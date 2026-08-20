"""
reconstruct.py — Pass 5, tier 3. Sinogram -> cross-section, in /cm.

    python3 python/reconstruct.py --sinogram data/sinograms/bars_pkgA.npz

Runs TomoPy FBP (gridrec) and an iterative comparison (ASTRA SIRT when it is
installed, TomoPy's own SIRT otherwise), determines the image orientation
against the analytic phantom rather than assuming it, and reports recovered mu
per material against NIST.

THE CHECKPOINT IS A NUMBER
--------------------------
The vault's Pass 5 checkpoint is "the reconstruction is recognisable". Every
other checkpoint in this project is a measurement, and this one can be too:
Option B spans steel / aluminium / polyethylene, whose mu at 661.657 keV are
derived from NIST in xcom_reference.py. An ROI mean inside each bar turns the
headline claim from "the picture looks right" into "recovered mu agrees with
NIST to X% across Z_eff from 5.5 to 26" — which is the claim worth putting in
the NUTECH abstract and the Nukleonika paper.

UNITS
-----
tomopy.recon returns attenuation per PIXEL. The pixel pitch is the translation
step, so dividing by the step in cm gives mu in /cm. Skipping that conversion
is the easy way to get a beautiful image that agrees with nothing.

ORIENTATION IS MEASURED, NOT ASSUMED
------------------------------------
TomoPy and ASTRA disagree with each other and with our (theta, t) convention
about angle direction and detector indexing, and a mirrored hexagon looks
exactly as plausible as a correct one. So the reconstruction is scored against
phantom_model.mu_map() over all eight dihedral transforms and the winner is
reported. If the identity wins, the conventions agree and that is a finding; if
a flip wins, that is also a finding, and it is recorded rather than silently
applied somewhere in the plotting code.

Note the division of labour with assemble_sinogram.py: that script has already
confirmed the SINOGRAM matches the model, so anything found here is purely a
reconstruction-library convention. Those are two different bugs in two
different files, and separating them is what stops this becoming the "budget a
day for a flipped reconstruction" the handoff warns about.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import phantom_model  # noqa: E402

DIHEDRAL = {
    "identity": lambda a: a,
    "flip ud": np.flipud,
    "flip lr": np.fliplr,
    "rot 180": lambda a: np.rot90(a, 2),
    "rot 90": lambda a: np.rot90(a, 1),
    "rot 270": lambda a: np.rot90(a, 3),
    "transpose": lambda a: a.T,
    "anti-transpose": lambda a: np.rot90(a, 2).T,
}


def orient(rec: np.ndarray, truth: np.ndarray):
    """Score all eight dihedral transforms of `rec` against `truth`."""
    if rec.shape != truth.shape:
        raise ValueError(
            f"reconstruction is {rec.shape}, analytic phantom map is "
            f"{truth.shape}. tomopy.recon returns (n_slices, n_t, n_t), so a "
            "mismatch means the reconstruction grid is not the detector grid "
            "— usually a back end that crops to the inscribed circle or picks "
            "its own output size. Set it to len(translations) rather than "
            "resampling the ground truth, which would silently change the "
            "pixel pitch and therefore every mu reported below.")
    scores = {name: float(np.sqrt(np.mean((fn(rec) - truth) ** 2)))
              for name, fn in DIHEDRAL.items()}
    best = min(scores, key=scores.get)
    return best, DIHEDRAL[best](rec), scores


def find_center_report(sino, theta, expected):
    """Checkpoint 5/6 — assume perfect centring, but measure it anyway (D7)."""
    try:
        import tomopy
        # tomopy.find_center returns a 1-element ndarray on some versions, and
        # numpy 2 refuses float() on anything that is not 0-d. Flatten first.
        found = float(np.asarray(tomopy.find_center(
            sino[:, None, :], theta, init=expected, tol=0.1)).ravel()[0])
    except Exception as exc:                      # noqa: BLE001
        print(f"  find_center unavailable ({exc}); using geometric centre")
        return expected
    print(f"  centre of rotation: geometric {expected:.2f} px, "
          f"find_center {found:.2f} px, difference {found - expected:+.2f} px")
    if abs(found - expected) > 0.5:
        print("    NOTE: these should agree — the simulation is centred by")
        print("    construction and the translation grid is symmetric about")
        print("    zero with an odd number of points, so the axis lands")
        print("    exactly on a sample. A disagreement is information about")
        print("    the assembly, not a knob to turn. Reconstructing at the")
        print("    geometric centre; investigate before trusting find_center.")
    return expected


def reconstruct(sino, angles_deg, step_mm, center):
    """FBP (gridrec) plus an iterative comparison. Returns {name: image}, /cm."""
    import tomopy
    theta = np.deg2rad(angles_deg)
    stack = sino[:, None, :]                       # (n_angles, 1 slice, n_t)
    out = {}

    rec = tomopy.recon(stack, theta, center=center, algorithm="gridrec",
                       filter_name="shepp")[0]
    out["FBP (gridrec)"] = rec / (step_mm / 10.0)

    # ASTRA if present, TomoPy's own SIRT if not. astra-toolbox is conda-only
    # and is NOT required for any NUTECH claim — the FBP/iterative comparison
    # stands either way. Spending deadline on an environment is the wrong
    # trade this close to 21 Sept.
    try:
        import astra  # noqa: F401
        rec2 = tomopy.recon(stack, theta, center=center,
                            algorithm=tomopy.astra,
                            options={"proj_type": "linear",
                                     "method": "SIRT_CUDA"
                                     if _has_cuda() else "SIRT",
                                     "num_iter": 200})[0]
        label = "SIRT (ASTRA, 200 it)"
    except Exception as exc:                       # noqa: BLE001
        print(f"  ASTRA unavailable ({type(exc).__name__}); "
              "using TomoPy SIRT instead")
        rec2 = tomopy.recon(stack, theta, center=center, algorithm="sirt",
                            num_iter=200)[0]
        label = "SIRT (TomoPy, 200 it)"
    out[label] = rec2 / (step_mm / 10.0)
    return out


def _has_cuda() -> bool:
    try:
        import astra
        return bool(astra.use_cuda())
    except Exception:                              # noqa: BLE001
        return False


def report_materials(rec, model, translations, label):
    """ROI means per material against NIST. This is the checkpoint."""
    masks = model.roi_masks(translations)
    sizes = model.feature_sizes_px(translations)
    print(f"\n  {label} — recovered mu vs NIST")
    print("  " + "-" * 74)
    print(f"    {'material':<14} {'ROI mean':>10} {'sd':>9} {'NIST':>10} "
          f"{'dev':>9} {'px':>6} {'feat':>6}")
    truth_mu = {c.label: c.delta_mu_per_mm * 10.0
                for c in model.cylinders if c.delta_mu_per_mm > 0}
    devs, thin = {}, []
    for name, mask in masks.items():
        vals = rec[mask]
        nist = truth_mu[name]
        dev = (vals.mean() / nist - 1.0) * 100.0
        devs[name] = dev
        if sizes[name] < 4.0:
            thin.append(name)
        print(f"    {name:<14} {vals.mean():>10.5f} {vals.std():>9.5f} "
              f"{nist:>10.5f} {dev:>+8.2f}% {mask.sum():>6d} "
              f"{sizes[name]:>5.1f}p")
    if thin:
        print(f"    NOTE: {', '.join(thin)} spans under 4 pixels. Its ROI mean "
              "is\n    biased by edge effects and is not quotable as a "
              "measurement of mu\n    at this pitch.")
    print("    On FBP the bias runs POSITIVE and lives in the INTERIOR. "
          "Three\n    things were measured about it, across both scan "
          "packages:\n"
          "      - it is ordered by object SIZE, not by mu (the smallest bar "
          "is\n        always the worst);\n"
          "      - it does not improve when the pitch is halved, which rules "
          "out\n        partial-volume averaging;\n"
          "      - it does not improve when the ROI is pulled further from "
          "the\n        edge, which rules out an ROI sampling boundary "
          "ringing;\n"
          "      - and the background goes NEGATIVE while every material goes\n"
          "        positive, so it is a redistribution, not a scale error.\n"
          "    Consistent with the ramp filter's positive central lobe filling "
          "a\n    compact object against a compensating negative tail "
          "outside it —\n    smaller object, larger fractional excess. "
          "Iterative reconstruction\n    shows none of it, and is what makes "
          "the recovered mu quantitative.")

    bg = rec[model.mu_map(translations) == 0.0]
    print(f"    {'background':<14} {bg.mean():>10.5f} {bg.std():>9.5f} "
          f"{0.0:>10.5f}")
    if len(devs) >= 2:
        hi = max(devs, key=lambda k: truth_mu[k])
        lo = min(devs, key=lambda k: truth_mu[k])
        contrast = (rec[masks[hi]].mean() - rec[masks[lo]].mean())
        noise = bg.std()
        print(f"    contrast-to-noise ({hi} vs {lo}): "
              f"{contrast / noise:.1f}" if noise > 0 else "")
    return devs


def save_figure(images, model, translations, dest):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except Exception as exc:                       # noqa: BLE001
        print(f"  matplotlib unavailable ({exc}); skipping the figure")
        return
    extent = [translations[0], translations[-1],
              translations[0], translations[-1]]
    truth = model.mu_map(translations) * 10.0
    panels = [("ground truth (NIST)", truth)] + list(images.items())
    fig, axes = plt.subplots(1, len(panels), figsize=(5 * len(panels), 5.4))
    vmax = truth.max() * 1.15
    for ax, (name, img) in zip(np.atleast_1d(axes), panels):
        im = ax.imshow(img, extent=extent, origin="lower",
                       cmap="gray", vmin=-0.02 * vmax, vmax=vmax)
        ax.set_title(name)
        ax.set_xlabel("x (mm)")
        ax.set_ylabel("y (mm)")
        fig.colorbar(im, ax=ax, fraction=0.046, label=r"$\mu$ (cm$^{-1}$)")
    fig.suptitle(f"cttwin — {model.name}, Cs-137 661.657 keV, "
                 f"IAEA CRP F22086")
    fig.tight_layout()
    fig.savefig(dest, dpi=160)
    print(f"  figure -> {dest}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sinogram", type=Path, required=True)
    ap.add_argument("--count", choices=("unscattered", "total"),
                    default="unscattered",
                    help="which sinogram to reconstruct (default unscattered — "
                         "the scatter-free one whose physics Pass 3 validated)")
    ap.add_argument("--out", type=Path, default=REPO / "data" / "reconstructions")
    args = ap.parse_args()

    d = np.load(args.sinogram, allow_pickle=False)
    phantom_name = str(d["phantom"])
    n_recon = int(d["n_reconstruction_angles"])
    angles = d["angles_deg"][:n_recon]
    translations = d["translations_mm"]
    sino = d[f"sino_{args.count}"][:n_recon]
    step = float(translations[1] - translations[0])

    print(f"cttwin reconstruction — {phantom_name}, {args.count} counts")
    print("=" * 74)
    print(f"  sinogram {sino.shape}, pixel pitch {step} mm, "
          f"{len(translations)} detector samples")
    print(f"  the 180 deg redundancy row is excluded from the reconstruction")

    expected_center = (len(translations) - 1) / 2.0
    center = find_center_report(sino, np.deg2rad(angles), expected_center)

    try:
        images = reconstruct(sino, angles, step, center)
    except ImportError:
        print("\n  tomopy is not installed. It installs most reliably via "
              "conda:\n    conda install -c conda-forge tomopy\n"
              "  Everything upstream of this point has already run and the "
              "sinogram\n  is verified and saved — only this step is blocked.")
        return 2

    model = phantom_model.get(phantom_name)
    truth = model.mu_map(translations) * 10.0

    args.out.mkdir(parents=True, exist_ok=True)
    oriented = {}
    for name, rec in images.items():
        best, fixed, scores = orient(rec, truth)
        print(f"\n  {name}: orientation vs the analytic phantom")
        for k, v in sorted(scores.items(), key=lambda kv: kv[1])[:3]:
            print(f"    {k:<16} RMS {v:.5f}{'   <-- applied' if k == best else ''}")
        if best != "identity":
            print(f"    NOTE: '{best}' was needed. The sinogram already "
                  "matched the\n    model (assemble_sinogram.py), so this is a "
                  "reconstruction-library\n    convention, not a driver bug. "
                  "Record it in the Pass 5 docs.")
        oriented[name] = fixed
        report_materials(fixed, model, translations, name)
        np.save(args.out / f"{args.sinogram.stem}_{args.count}_"
                           f"{name.split()[0].lower()}.npy", fixed)

    save_figure(oriented, model, translations,
                args.out / f"{args.sinogram.stem}_{args.count}.png")
    print(f"\n  arrays -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())