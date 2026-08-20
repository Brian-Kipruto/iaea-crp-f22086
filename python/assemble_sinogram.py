"""
assemble_sinogram.py — Pass 5, tier 2. Per-angle CSVs -> a verified sinogram.

    python3 python/assemble_sinogram.py --scan data/raw/bars_pkgA

Stacks every row into (n_angles, n_translations), normalises against the open
beam, converts to line integrals, verifies the result against the analytic
model in phantom_model.py, and saves one .npz to data/sinograms/.

THE SECOND AXIS IS TRANSLATION, NOT PIXELS
------------------------------------------
[[Output Format]] describes the sinogram as (n_angles, n_pixels). That wording
predates the single-pixel detector. `pixel_index` is 0 in every row, so
assembling on it gives shape (n_angles, 1) — a sinogram of one column, which
reconstructs to nothing and looks like a data problem rather than an indexing
one. The spatial coordinate within a projection is `translation_mm`.

WHY .npz AND NOT .npy
---------------------
The vault specifies "one .npy". That was written when there was one count per
row and no open beam. There are now two counts (total and unscattered, ADR
0004), the normalisation has to travel with the data (`n_events`), and the
axes have physical units that the reconstruction needs in order to report mu
in /cm rather than /pixel. A bare array carrying none of that is a file whose
correct interpretation lives in someone's memory — which is the exact failure
mode ADR 0004 was opened to fix. One self-describing .npz instead. Deliberate
deviation, recorded here and in the Pass 5 docs.

LINE INTEGRALS
--------------
    p = -ln( (N / n_events) / (N0 / n0_events) )

Both counts are normalised by their own event totals before the ratio. This is
why `n_events` is in the CSV at all: the open beam runs at 100x the statistics
of a projection, and a ratio of raw counts across different N is meaningless.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import phantom_model  # noqa: E402


def load_rows(path: Path) -> list[dict]:
    with open(path) as fh:
        return list(csv.DictReader(fh))


def assemble(scan_dir: Path):
    manifest = json.loads((scan_dir / "manifest.json").read_text())
    angles = np.array(manifest["angles_deg"], dtype=float)
    translations = np.array(manifest["translations_mm"], dtype=float)
    n_a, n_t = len(angles), len(translations)

    total = np.full((n_a, n_t), np.nan)
    unscattered = np.full((n_a, n_t), np.nan)
    events = np.full((n_a, n_t), np.nan)

    t_index = {round(t, 6): j for j, t in enumerate(translations)}

    for i in range(n_a):
        csv_path = scan_dir / f"angle_{i:04d}.csv"
        if not csv_path.exists():
            raise FileNotFoundError(f"missing {csv_path} — re-run that angle")
        for row in load_rows(csv_path):
            # Index on the row's OWN recorded coordinates, never on file order.
            # The CSV records what was actually built (RunAction reads it back
            # from DetectorConstruction), so a row that landed at the wrong
            # angle shows up here as a mismatch rather than being silently
            # filed under the angle we meant to run.
            if abs(float(row["angle_deg"]) - angles[i]) > 1e-6:
                raise ValueError(
                    f"{csv_path.name}: row claims angle "
                    f"{row['angle_deg']} deg, manifest says {angles[i]}. The "
                    "scan and the manifest disagree — do not reconstruct this.")
            j = t_index.get(round(float(row["translation_mm"]), 6))
            if j is None:
                raise ValueError(f"{csv_path.name}: translation "
                                 f"{row['translation_mm']} mm is not on the grid")
            if not np.isnan(total[i, j]):
                raise ValueError(
                    f"{csv_path.name}: duplicate row at theta={angles[i]}, "
                    f"t={translations[j]}. The CSV appends, so this is an "
                    "angle that was run twice into the same file. Delete it "
                    "and re-run with --force.")
            total[i, j] = float(row["n_counts"])
            unscattered[i, j] = float(row["n_unscattered"])
            events[i, j] = float(row["n_events"])

    missing = int(np.isnan(total).sum())
    if missing:
        raise ValueError(f"{missing} measurements missing from the grid")

    n0_rows = load_rows(scan_dir / manifest["open_beam_csv"])
    if len(n0_rows) != 1:
        raise ValueError(f"open beam has {len(n0_rows)} rows, expected 1")
    n0 = n0_rows[0]
    n0_events = float(n0["n_events"])
    n0_total = float(n0["n_counts"]) / n0_events
    n0_unsc = float(n0["n_unscattered"]) / n0_events

    with np.errstate(divide="ignore", invalid="ignore"):
        p_total = -np.log((total / events) / n0_total)
        p_unsc = -np.log((unscattered / events) / n0_unsc)

    return dict(manifest=manifest, angles_deg=angles,
                translations_mm=translations,
                counts_total=total, counts_unscattered=unscattered,
                events=events, n0_events=n0_events,
                n0_frac_total=n0_total, n0_frac_unscattered=n0_unsc,
                sino_total=p_total, sino_unscattered=p_unsc)


# ---------------------------------------------------------------------------
# Verification — checkpoints 3 and 4

def verify(data: dict, phantom_name: str) -> bool:
    angles = data["angles_deg"]
    translations = data["translations_mm"]
    sino = data["sino_unscattered"]
    n_recon = len(angles) - 1                # last angle is the 180 deg check
    ok = True

    print("Sinogram verification")
    print("=" * 74)
    print(f"  shape {sino.shape}  "
          f"(n_angles x n_translations — the second axis is TRANSLATION)")
    print(f"  open beam: {data['n0_frac_unscattered']:.6f} unscattered "
          f"at {data['n0_events']:,.0f} primaries "
          f"(1 sigma {100 / np.sqrt(data['n0_frac_unscattered'] * data['n0_events']):.3f}%)")

    # --- statistical floor, for judging everything below
    frac = data["counts_unscattered"] / data["events"]
    sig = np.sqrt(frac * (1 - frac) / data["events"]) / frac
    print(f"  ray noise on p: median {np.median(sig):.4f}, "
          f"worst {np.nanmax(sig):.4f} (line integral units)")

    # --- redundancy: p(theta, t) == p(theta + 180, -t)
    #
    # The partner angle is chosen by run_scan (30 deg, not 0 deg) because the
    # Option B projection at theta = 0 is EVEN in t, and against an even row
    # the correct relation and the wrong one p(theta+180,+t) are
    # indistinguishable. The wrong-relation figure below is printed precisely
    # so that a degenerate test announces itself instead of passing quietly.
    partner = data["manifest"].get("redundancy_partner_deg", 0.0)
    idx = int(np.argmin(np.abs(angles[:n_recon] - partner)))
    a, b = sino[idx], sino[-1][::-1]
    s = np.hypot(sig[idx], sig[-1][::-1])
    dev = np.abs(a - b) / s
    wrong = np.abs(a - sino[-1]) / s
    print(f"\n  Redundancy p({angles[idx]:g},t) = p({angles[-1]:g},-t): "
          f"mean {dev.mean():.2f} sigma, max {dev.max():.2f} sigma")
    print(f"    the WRONG relation p(theta+180,+t) gives "
          f"{wrong.mean():.1f} sigma")
    if wrong.mean() < 3.0:
        print("    NOTE: the two relations are not separated here, so this "
              "test\n    discriminates nothing. Expected for the pipe (every "
              "projection is\n    even in t). On the BARS it is a real "
              "warning — pick a partner\n    angle whose projection is "
              "asymmetric.")
    if dev.mean() > 3.0:
        print("    FAIL: the redundancy relation does not hold.")
        ok = False

    # --- against the analytic model
    try:
        model = phantom_model.get(phantom_name)
    except KeyError:
        print(f"\n  No analytic model for '{phantom_name}' — skipping.")
        return ok

    best, scores = phantom_model.best_sign_convention(
        sino[:n_recon], angles[:n_recon], translations, model)
    print("\n  Sign convention, scored against the analytic model (RMS):")
    for label, rms in sorted(scores.items(), key=lambda kv: kv[1]):
        print(f"    {label:<32} {rms:.5f}{'   <-- best' if label == best else ''}")
    spread = max(scores.values()) - min(scores.values())
    if spread < 1e-6:
        print("    All four tie. Expected for the pipe: it is rotationally"
              "\n    symmetric, so no sign convention is observable from it at "
              "all.\n    This test only has teeth on the bars.")
    elif not best.startswith("as-acquired") and not best.startswith("both"):
        print("    WARNING: the as-acquired data does NOT match the model. "
              "The\n    relative sign of theta and t is wrong somewhere in the "
              "driver.")
        ok = False
    else:
        print("    As-acquired matches. (It ties with 'both flipped' by "
              "construction:\n    every Phase 1 phantom is mirror-symmetric, "
              "so the ABSOLUTE convention\n    is unobservable here — the "
              "RELATIVE sign, the one that mirrors a\n    reconstruction, is "
              "what this pins down.)")

    pred = model.sinogram(angles[:n_recon], translations)
    resid = sino[:n_recon] - pred
    pull = resid / sig[:n_recon]
    print(f"\n  Residual vs model: RMS {np.sqrt((resid ** 2).mean()):.5f}, "
          f"mean pull {pull.mean():+.2f} sigma, RMS pull {np.sqrt((pull ** 2).mean()):.2f}")
    print("    Scatter makes the measured line integral SMALLER than the "
          "model\n    (forward-scattered photons still land on the face), so a "
          "small\n    negative mean pull on the total sinogram is physics, not "
          "error.")

    if phantom_name == "pipe":
        # Rotational symmetry: every column must be flat in theta. A strong
        # test of the driver that needs no reconstruction, and one the bars
        # cannot provide.
        #
        # UNITS: `sig` is the RELATIVE sigma on the count fraction, and since
        # p = -ln(N/N0), d p = -dN/N, that relative sigma IS the ABSOLUTE sigma
        # on p. So it is compared to the column spread directly. Scaling it by
        # |p| would be wrong twice over: wrong dimensionally, and catastrophic
        # outside the phantom where p -> 0 and the ratio blows up on a column
        # that is in fact perfectly well behaved.
        col_sd = sino[:n_recon].std(axis=0)
        col_exp = sig[:n_recon].mean(axis=0)
        ratio = col_sd / col_exp
        worst = float(np.nanmax(ratio))
        print(f"\n  Pipe rotational invariance: column scatter / expected "
              f"statistical\n    spread — median {np.nanmedian(ratio):.2f}, "
              f"worst {worst:.2f} (expect ~1).")
        if worst > 3.0:
            bad = int(np.nanargmax(ratio))
            print(f"    FAIL at t = {translations[bad]:g} mm. The pipe is "
                  "rotationally\n    symmetric, so a column that varies with "
                  "theta is a driver bug —\n    most likely the scan transform "
                  "not being applied, or rows filed\n    under the wrong angle.")
            ok = False

    if phantom_name == "bars":
        print("\n  Option B fingerprint: each bar traces t = 60*sin(theta+phi).")
        for label, trace in phantom_model.bar_traces(model, angles[:n_recon]):
            amp = np.abs(trace).max()
            print(f"    {label:<14} amplitude {amp:>5.1f} mm")
        print("    Six sinusoids of amplitude 60.0 mm at 60 deg phase spacing,")
        print("    plus the central bar's straight line at t = 0. NO baseplate")
        print("    trace: it sits at z in [-85,-75] mm and the beam is at z=0.")

    return ok


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--scan", type=Path, required=True,
                    help="a scan directory under data/raw/")
    ap.add_argument("--out", type=Path, default=REPO / "data" / "sinograms")
    ap.add_argument("--no-verify", action="store_true")
    args = ap.parse_args()

    data = assemble(args.scan.resolve())
    phantom = data["manifest"]["phantom"]

    ok = True if args.no_verify else verify(data, phantom)

    args.out.mkdir(parents=True, exist_ok=True)
    dest = args.out / f"{args.scan.name}.npz"
    np.savez_compressed(
        dest,
        sino_unscattered=data["sino_unscattered"],
        sino_total=data["sino_total"],
        counts_unscattered=data["counts_unscattered"],
        counts_total=data["counts_total"],
        events=data["events"],
        angles_deg=data["angles_deg"],
        translations_mm=data["translations_mm"],
        n_reconstruction_angles=len(data["angles_deg"]) - 1,
        n0_frac_unscattered=data["n0_frac_unscattered"],
        n0_frac_total=data["n0_frac_total"],
        n0_events=data["n0_events"],
        phantom=phantom,
    )
    print(f"\n  -> {dest}")
    print(f"  next: python3 python/reconstruct.py --sinogram {dest}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())