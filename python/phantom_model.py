"""
phantom_model.py — Pass 5. The analytic forward model of the Phase 1 phantoms.

WHY THIS FILE EXISTS
--------------------
"The reconstruction is recognisable" is a judgement. Every other checkpoint in
this project is a number, and this one can be too: the Phase 1 phantoms are
circles, the beam is a zero-width pencil, and the chord of a ray through a
circle is one line of trigonometry. So the whole sinogram is predictable in
closed form, before TomoPy is ever imported.

That buys three things Pass 5 needs:

  1. The sinogram can be validated on its own, ahead of any reconstruction.
     If the measured sinogram matches the model, the Geant4 tier and the
     driver are both correct and any remaining problem is in reconstruction.
     If it does not, the bug is upstream and there is no point reconstructing.
     Two failures that would otherwise look identical are separated here.

  2. The (theta, t) sign convention is DETERMINED rather than assumed.
     `best_sign_convention()` scores the measured sinogram against the model
     under all four sign hypotheses. The correct one wins by orders of
     magnitude, and the answer is a table rather than an opinion.

  3. The reconstruction gets a ground truth. `mu_map()` renders the phantom on
     the reconstruction grid, which turns "steel looks brighter than poly"
     into a per-material comparison against NIST.

THE Z = 0 SLICE — AND THE BASEPLATE THAT ISN'T IN IT
----------------------------------------------------
The gun sits at (-250, 0, 0) firing along +x, a zero-width pencil beam that
lives permanently in the plane z = 0. In DetectorConstruction the Option B
baseplate spans z in [-85, -75] mm while the bars span z in [-74.99, +75.01]
mm, and the scan transform only ever moves the phantom in x and y. So the
beam plane NEVER intersects the baseplate, at any (theta, t).

This is not a reading of the code, it is measured. Pass 4 recorded
theta = 30 deg, t = 0 -> unscattered 0.44469. Four centimetres of aluminium
alone predicts 0.44477. If the baseplate were in the beam it would add ~200 mm
of aluminium chord and multiply the result by 0.018.

CONSEQUENCE: the Option B reconstruction shows SEVEN BARS AND NO RING. The
"faint baseplate ring" in the vault ([[Option B - Multi-Material Array]],
[[Phase 2 - End-to-End Pipeline]]) and in the Pass 4 handoff predates the
pencil beam and is wrong for this geometry. Left uncorrected it costs an
evening hunting for a feature that was never illuminated.

Everything below therefore models the z = 0 slice only, and the baseplate is
deliberately absent.

GEOMETRY
--------
Under ADR 0005 a phantom point p maps to the world as

    p_world = R_z(theta) * p  -  t * y_hat

and the ray is the world x-axis. So a cylinder whose axis sits at phantom
polar coordinates (R_c, phi) has, in the world, a signed perpendicular offset
from the ray of

    y_w(theta, t) = R_c * sin(theta + phi) - t

and the ray's chord through it is 2 * sqrt(r^2 - y_w^2) when |y_w| < r, else
zero. That single expression is the whole forward model, and read as a curve
in the (theta, t) plane it is the sinusoid the bar traces in the sinogram.
"""

from __future__ import annotations

import math
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from xcom_reference import (  # noqa: E402
    aluminium_reference,
    carbon_steel_reference,
    polyethylene_reference,
)


# ---------------------------------------------------------------------------
# Geometry, mirroring src/DetectorConstruction.cc exactly.
#
# These are duplicated from C++ rather than parsed out of it. That is a real
# risk of drift, and the mitigation is checkpoint 3/4: if the geometry here and
# the geometry in the simulator disagree, the measured sinogram stops matching
# the model and the verification fails loudly. A silent copy is dangerous; a
# copy with a test against the original is a cross-check.

PIPE_OD_MM = 141.3          # 5" NPS SCH 40
PIPE_WALL_MM = 6.55

BARS_RING_RADIUS_MM = 60.0
BARS_CENTRE_RADIUS_MM = 20.0   # central Al bar, 40 mm dia
BARS_STEEL_RADIUS_MM = 10.0    # 20 mm dia, at phi = 0, 120, 240 deg
BARS_POLY_RADIUS_MM = 15.0     # 30 mm dia, at phi = 60, 180, 300 deg


def _erode(mask: np.ndarray, n: int) -> np.ndarray:
    """Peel `n` layers of boundary pixels off a boolean mask (4-connected).

    Written out rather than pulled from scipy.ndimage: phantom_model is
    imported by assemble_sinogram.py, which runs under system python where
    scipy may not be installed, and a hard dependency there would break the
    sinogram stage over a reconstruction-only convenience.
    """
    m = np.asarray(mask, dtype=bool)
    for _ in range(max(0, int(n))):
        e = m.copy()
        e[1:, :] &= m[:-1, :]
        e[:-1, :] &= m[1:, :]
        e[:, 1:] &= m[:, :-1]
        e[:, :-1] &= m[:, 1:]
        e[0, :] = e[-1, :] = False
        e[:, 0] = e[:, -1] = False
        m = e
    return m


@dataclass(frozen=True)
class Cylinder:
    """A circular cross-section in the phantom frame (its pose at theta=0, t=0).

    `delta_mu_per_mm` is a DIFFERENCE in attenuation coefficient, not an
    absolute one, so that nested volumes compose by addition: the pipe is a
    steel disc of the outer radius plus a bore disc of the inner radius
    carrying -mu_steel. Chord arithmetic then needs no special cases.
    """

    label: str
    centre_r_mm: float       # R_c — phantom-frame polar radius of the axis
    centre_phi_deg: float    # phi  — phantom-frame polar angle of the axis
    radius_mm: float
    delta_mu_per_mm: float

    def offset_mm(self, theta_deg, t_mm):
        """Signed perpendicular distance from the ray to this axis, in mm.

        Vectorised: theta_deg and t_mm may be arrays, and broadcast.
        """
        phase = np.deg2rad(np.asarray(theta_deg, dtype=float)
                           + self.centre_phi_deg)
        return self.centre_r_mm * np.sin(phase) - np.asarray(t_mm, dtype=float)

    def chord_mm(self, theta_deg, t_mm):
        """Path length of the ray through this circle, in mm. Zero if missed."""
        y = self.offset_mm(theta_deg, t_mm)
        inside = np.abs(y) < self.radius_mm
        return np.where(inside,
                        2.0 * np.sqrt(np.clip(self.radius_mm ** 2 - y ** 2,
                                              0.0, None)),
                        0.0)


@dataclass(frozen=True)
class Phantom:
    name: str
    cylinders: tuple = field(default_factory=tuple)

    @property
    def support_radius_mm(self) -> float:
        """Smallest radius containing the whole slice — sets the scan range."""
        return max(c.centre_r_mm + c.radius_mm for c in self.cylinders)

    def line_integral(self, theta_deg, t_mm):
        """Predicted -ln(N/N0) for the ray at (theta, t). Dimensionless.

        Air is deliberately not modelled. The measurement is normalised by an
        open-beam run over the full 500 mm air path, so what survives is
        +mu_air * (chord through the phantom): a -mu_air ~ -1e-5 /mm offset
        inside each material, against mu_Al = 2.0e-2 /mm. That is 0.05%, an
        order below the counting statistics of any single ray. Neglected
        deliberately, not overlooked — the same term is applied explicitly in
        validate_beer_lambert.py, where at 40 mm it was worth +0.03%.
        """
        theta = np.asarray(theta_deg, dtype=float)
        t = np.asarray(t_mm, dtype=float)
        total = np.zeros(np.broadcast(theta, t).shape)
        for c in self.cylinders:
            total = total + c.delta_mu_per_mm * c.chord_mm(theta, t)
        return total

    def sinogram(self, angles_deg, translations_mm):
        """Predicted line-integral sinogram, shape (n_angles, n_translations)."""
        th = np.asarray(angles_deg, dtype=float)[:, None]
        tt = np.asarray(translations_mm, dtype=float)[None, :]
        return self.line_integral(th, tt)

    def mu_map(self, translations_mm):
        """Render the phantom as an absolute-mu image on the reconstruction grid.

        The grid is n_t x n_t, pixel pitch equal to the translation step,
        centred on the rotation axis — which is what a parallel-beam FBP over
        this sinogram produces. Returned in /mm, indexed [row, col] with row
        increasing in +y and col increasing in +x, i.e. the phantom frame at
        theta = 0. Whether the reconstruction shares that handedness is not
        assumed anywhere; see reconstruct.py, which measures it.
        """
        t = np.asarray(translations_mm, dtype=float)
        yy, xx = np.meshgrid(t, t, indexing="ij")
        out = np.zeros_like(xx)
        for c in self.cylinders:
            cx = c.centre_r_mm * math.cos(math.radians(c.centre_phi_deg))
            cy = c.centre_r_mm * math.sin(math.radians(c.centre_phi_deg))
            inside = ((xx - cx) ** 2 + (yy - cy) ** 2) < c.radius_mm ** 2
            out = out + np.where(inside, c.delta_mu_per_mm, 0.0)
        return out

    def roi_masks(self, translations_mm, erode_mm=2.0):
        """Interior masks per labelled region, for quantitative mu recovery.

        BUILT FROM THE NET mu MAP, NOT FROM EACH CYLINDER SEPARATELY.
        An earlier version masked each positive-mu cylinder by its own radius,
        shrunk by a factor. That is correct for the bars and WRONG for the
        pipe: the pipe's steel is a solid outer disc plus a NEGATIVE bore disc,
        so shrinking the outer disc lands the ROI entirely inside the air
        cavity. It reported the bore's mu (~0) as the steel's and produced a
        -103% "error" that was purely a masking bug.

        Selecting on the net map instead means the ROI is, by construction,
        wherever that material actually ends up — annulus, disc, or anything
        else — so nesting can never mislead it again.

        `erode_mm` then peels back a fixed PHYSICAL margin, because FBP rings
        at a sharp boundary and an ROI drawn to the true edge samples the
        overshoot rather than the interior.

        In millimetres, not pixels, and that distinction is not cosmetic. An
        earlier version eroded a fixed pixel count, which meant a 2 mm margin
        at a 2 mm pitch but only 1 mm at a 1 mm pitch — so a finer scan drew
        its ROI physically CLOSER to the ringing edge, cancelling out the
        resolution it had just paid for. Package A and package B were being
        judged on different criteria, and any comparison of the two was
        measuring the mask as much as the reconstruction. A fixed physical
        margin makes them commensurable.
        """
        net = self.mu_map(translations_mm)
        masks = {}
        for c in self.cylinders:
            if c.delta_mu_per_mm <= 0.0:
                continue
            m = np.isclose(net, c.delta_mu_per_mm, rtol=1e-6, atol=0.0)
            if c.label in masks:
                masks[c.label] |= m
            else:
                masks[c.label] = m
        step = abs(float(translations_mm[1] - translations_mm[0]))
        n = int(round(erode_mm / step))
        return {k: _erode(v, n) for k, v in masks.items()}

    def feature_sizes_px(self, translations_mm):
        """Smallest resolved dimension per material, in pixels.

        Reported next to the recovered mu because it is the single number that
        says how much of any shortfall is partial-volume blurring rather than
        physics. The pipe wall is 6.55 mm: 3.3 px at a 2 mm pitch, 6.6 px at
        1 mm. Below ~4 px an ROI mean is dominated by the edge and cannot be
        quoted as a measurement of mu.
        """
        step = abs(float(translations_mm[1] - translations_mm[0]))
        out = {}
        for c in self.cylinders:
            if c.delta_mu_per_mm <= 0.0:
                continue
            if self.name == "pipe" and c.label == "steel":
                size = PIPE_WALL_MM              # annulus: the wall, not the OD
            else:
                size = 2.0 * c.radius_mm
            out[c.label] = min(out.get(c.label, 1e9), size / step)
        return out


# ---------------------------------------------------------------------------
# The phantoms

def _mu_per_mm():
    return (carbon_steel_reference().mu_per_mm,
            aluminium_reference().mu_per_mm,
            polyethylene_reference().mu_per_mm)


def pipe_phantom() -> Phantom:
    """Option A — 5" SCH 40 carbon-steel pipe, hollow, air-filled bore.

    Modelled as a solid steel disc of the outer radius plus a bore disc of the
    inner radius carrying -mu_steel. Rotationally symmetric, so its sinogram is
    independent of theta — which is precisely what makes it a strong test of
    the driver and a useless test of the angular convention.
    """
    mu_steel, _, _ = _mu_per_mm()
    r_out = PIPE_OD_MM / 2.0
    r_in = r_out - PIPE_WALL_MM
    return Phantom("pipe", (
        Cylinder("steel", 0.0, 0.0, r_out, +mu_steel),
        Cylinder("bore", 0.0, 0.0, r_in, -mu_steel),
    ))


def bars_phantom() -> Phantom:
    """Option B — central Al bar + hexagonal ring of alternating steel/poly.

    NO BASEPLATE: it sits at z in [-85, -75] mm and the beam plane is z = 0.
    See the module docstring.

    Bar i sits at phi = i*60 deg, steel for even i (per DetectorConstruction's
    `i % 2 == 0`), poly for odd. Each traces t = 60*sin(theta + i*60 deg) in
    the sinogram; the central bar traces the straight line t = 0.
    """
    mu_steel, mu_al, mu_poly = _mu_per_mm()
    cyls = [Cylinder("aluminium", 0.0, 0.0, BARS_CENTRE_RADIUS_MM, mu_al)]
    for i in range(6):
        if i % 2 == 0:
            cyls.append(Cylinder("steel", BARS_RING_RADIUS_MM, i * 60.0,
                                 BARS_STEEL_RADIUS_MM, mu_steel))
        else:
            cyls.append(Cylinder("polyethylene", BARS_RING_RADIUS_MM, i * 60.0,
                                 BARS_POLY_RADIUS_MM, mu_poly))
    return Phantom("bars", tuple(cyls))


PHANTOMS = {"pipe": pipe_phantom, "bars": bars_phantom}


def get(name: str) -> Phantom:
    if name not in PHANTOMS:
        raise KeyError(f"No analytic model for phantom '{name}'. "
                       f"Known: {sorted(PHANTOMS)}")
    return PHANTOMS[name]()


# ---------------------------------------------------------------------------
# Sign-convention determination

SIGN_HYPOTHESES = {
    "as-acquired      (+theta, +t)": (+1, +1),
    "translation flip (+theta, -t)": (+1, -1),
    "angle flip       (-theta, +t)": (-1, +1),
    "both flipped     (-theta, -t)": (-1, -1),
}


def best_sign_convention(measured, angles_deg, translations_mm, phantom):
    """Score `measured` against the model under all four sign hypotheses.

    Returns (best_label, {label: rms}), rms in line-integral units.

    Why all four and not just a look at the picture: a mirrored reconstruction
    and a correct one are equally plausible-looking images of a hexagon, and
    the difference between them is one sign in one line of Python. Scoring
    settles it before any reconstruction runs.

    NOTE on what this can and cannot decide. Every Phase 1 phantom is
    mirror-symmetric about the beam axis, so a SIMULTANEOUS flip of both signs
    is an exact symmetry of the object and "as-acquired" and "both flipped"
    will score identically. That degeneracy is expected and is the vault's
    known limit on the absolute convention (it becomes observable in Phase 3).
    What this DOES decide is the RELATIVE sign — whether t must be reversed
    with respect to theta — and that is the one that mirrors a reconstruction.
    """
    measured = np.asarray(measured, dtype=float)
    scores = {}
    for label, (s_theta, s_t) in SIGN_HYPOTHESES.items():
        pred = phantom.sinogram(s_theta * np.asarray(angles_deg),
                                s_t * np.asarray(translations_mm))
        scores[label] = float(np.sqrt(np.mean((measured - pred) ** 2)))
    return min(scores, key=scores.get), scores


def bar_traces(phantom, angles_deg):
    """The t(theta) curve each cylinder axis traces in the sinogram, in mm.

    This is the Option B fingerprint: six sinusoids of amplitude exactly
    60.0 mm at 60 deg phase spacing, plus the central bar's straight line at
    t = 0. Overlay it on the measured sinogram and the convention is readable
    by eye in one glance.
    """
    th = np.asarray(angles_deg, dtype=float)
    out = []
    for c in phantom.cylinders:
        if c.delta_mu_per_mm <= 0.0:
            continue
        out.append((c.label, c.centre_r_mm
                    * np.sin(np.deg2rad(th + c.centre_phi_deg))))
    return out


def main() -> None:
    mu_steel, mu_al, mu_poly = _mu_per_mm()
    print("cttwin — analytic phantom model (z = 0 slice)")
    print("=" * 72)
    print(f"  mu steel : {mu_steel * 10:.7f} /cm")
    print(f"  mu Al    : {mu_al * 10:.7f} /cm")
    print(f"  mu poly  : {mu_poly * 10:.7f} /cm")
    print()
    for name in ("pipe", "bars"):
        p = get(name)
        print(f"  {name}: {len(p.cylinders)} cylinders, "
              f"support radius {p.support_radius_mm:.2f} mm")
    print()
    print("  Pass 4 Option B anchors, against this model")
    print("  " + "-" * 68)
    bars = get("bars")
    air = 0.995292          # measured open-beam unscattered fraction at 1e6
    for theta, measured in ((0.0, 0.11118), (30.0, 0.44469)):
        pred_frac = air * math.exp(-float(bars.line_integral(theta, 0.0)))
        sigma = math.sqrt(measured * (1 - measured) / 1e5)   # Pass 4 ran 1e5
        print(f"    theta={theta:>5.1f} deg, t=0 : predicted {pred_frac:.5f}, "
              f"measured {measured:.5f}, "
              f"{(measured / pred_frac - 1) * 100:+.2f}% "
              f"({(measured - pred_frac) / sigma:+.2f} sigma)")
    print()
    print("  Both are statistical. Pass 4 quoted 0.2-1.5% residuals here")
    print("  because it used literature-grade mu for Al and poly; with the")
    print("  NIST-derived values they are 1.75 and 0.05 sigma.")
    print()
    print("  NOTE: no baseplate. It sits at z in [-85, -75] mm and the pencil")
    print("  beam lives at z = 0, so it is never illuminated at any (theta, t).")


if __name__ == "__main__":
    main()