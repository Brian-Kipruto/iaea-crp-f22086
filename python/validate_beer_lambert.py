"""
validate_beer_lambert.py — Pass 3, the Phase 1 physics validation.

Drives cttwin through an open-beam reference and a set of flat carbon-steel
slabs, and compares the measured unscattered transmission against the
analytical Beer-Lambert prediction from python/xcom_reference.py.

Run it:
    python3 python/validate_beer_lambert.py --exe build/cttwin

WHAT IT REPORTS, AND WHY THERE ARE TWO ANSWERS
----------------------------------------------
1. PER-THICKNESS DEVIATION vs NIST XCOM.
   This is the abstract's stated claim: within 2% at 5/10/20/40 mm. It is a
   comparison between the simulation and an EXTERNAL data standard, so it
   folds together two different things: whether cttwin transports photons
   correctly, and whether Geant4's photon cross-section library agrees with
   NIST's. A failure here does not say which of the two moved.

2. FITTED mu FROM THE SIMULATION ITSELF.
   -ln(N/N0) is fitted against t across all four thicknesses. Two separate
   results come out, and they answer separate questions:
     * The RESIDUALS test whether attenuation is exponential in t at all.
       That is the actual transport validation, and it is internal to the
       simulation — no external data can make it pass or fail.
     * The SLOPE is Geant4's effective mu. Comparing it to the NIST value
       quantifies the library difference as one number, once, instead of
       letting it masquerade as four unrelated per-thickness errors.

   The distinction matters because the two failure modes need opposite
   responses. Non-exponential behaviour is a bug in cttwin. A clean
   exponential with an offset slope is a cross-section data difference, which
   is a finding to report, not a defect to fix.

WHAT COUNT IS COMPARED
----------------------
The UNSCATTERED count, not the total. exp(-mu*t) describes primary
transmission; the total count includes photons that scattered forward in the
phantom and still landed on the detector face. At one half-value layer that
contamination is already worth ~2.4%, so comparing totals against
Beer-Lambert compares two different physical quantities and fails for a
reason that has nothing to do with transport accuracy. Both are reported; the
criterion is applied to the unscattered one. See ADR 0004.

N0 IS THE MEASURED OPEN BEAM, NOT THE NUMBER OF PRIMARIES FIRED
---------------------------------------------------------------
About 0.42% of primaries are lost to the 500 mm air path before reaching the
detector. Dividing by the events fired would carry that straight into the
result as a systematic. Dividing by the measured empty-world count cancels it.
A small correction remains, because a slab of thickness t displaces t of air,
so the slab runs traverse slightly less air than the reference does; that is
applied explicitly below (worth +0.03% at 40 mm — negligible against the
statistics, included because the whole point of this script is not waving
small terms away).
"""

from __future__ import annotations

import argparse
import math
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from xcom_reference import (  # noqa: E402
    VALIDATION_THICKNESSES_MM,
    carbon_steel_reference,
)

# Total air path from source to detector, mm. Must match kSourceToDetector
# in include/Constants.hh.
SOURCE_TO_DETECTOR_MM = 500.0

ACCEPTANCE_PERCENT = 2.0

RESULT_RE = re.compile(r"^CTTWIN_RESULT\s+(.*)$", re.MULTILINE)


# --------------------------------------------------------------------------
# Running cttwin


@dataclass
class RunResult:
    phantom: str
    slab_mm: float
    events: int
    total: int
    unscattered: int

    @property
    def frac_total(self) -> float:
        return self.total / self.events

    @property
    def frac_unscattered(self) -> float:
        return self.unscattered / self.events

    def binomial_sigma(self, unscattered: bool = True) -> float:
        """Relative 1-sigma on the fraction. Binomial, not Poisson: the number
        of primaries is fixed, so sqrt(counts) would overstate the spread."""
        p = self.frac_unscattered if unscattered else self.frac_total
        if p <= 0.0 or p >= 1.0:
            return float("inf")
        return math.sqrt(p * (1.0 - p) / self.events) / p


def parse_result(stdout: str) -> RunResult:
    matches = RESULT_RE.findall(stdout)
    if not matches:
        raise RuntimeError(
            "No CTTWIN_RESULT line in cttwin output. Either the run failed, or "
            "RunAction::EndOfRunAction has been changed and this parser is stale."
        )
    fields = dict(kv.split("=", 1) for kv in matches[-1].split())
    return RunResult(
        phantom=fields["phantom"],
        slab_mm=float(fields["slab_mm"]),
        events=int(fields["events"]),
        total=int(fields["total"]),
        unscattered=int(fields["unscattered"]),
    )


def run_cttwin(exe: Path, macro_lines: list[str], workdir: Path,
               label: str, verbose: bool) -> RunResult:
    macro = workdir / f"{label}.mac"
    macro.write_text("\n".join(macro_lines) + "\n")

    print(f"  running {label} ...", end="", flush=True)
    proc = subprocess.run(
        [str(exe), str(macro)],
        capture_output=True, text=True, cwd=exe.parent,
    )
    if proc.returncode != 0:
        print(" FAILED")
        sys.stderr.write(proc.stdout[-4000:])
        sys.stderr.write(proc.stderr[-4000:])
        raise RuntimeError(f"cttwin exited {proc.returncode} on {label}")

    if "*** CTTwin ERROR" in proc.stdout:
        print(" FAILED")
        raise RuntimeError(
            f"{label}: a /cttwin command was rejected. The run did NOT use the "
            "requested configuration — refusing to report its numbers."
        )

    result = parse_result(proc.stdout)
    print(f" {result.unscattered}/{result.events} unscattered")
    if verbose:
        print(proc.stdout[-1500:])
    return result


# --------------------------------------------------------------------------
# Analysis


def air_correction(t_mm: float, open_beam_fraction: float) -> float:
    """Air survival over (SDD - t) relative to survival over the full SDD.

    The open-beam run measures survival across the whole air path. A slab run
    replaces t mm of that air with steel, so it sees slightly less air.
    """
    return open_beam_fraction ** ((SOURCE_TO_DETECTOR_MM - t_mm)
                                  / SOURCE_TO_DETECTOR_MM) / open_beam_fraction


def weighted_linear_fit(xs: list[float], ys: list[float],
                        sigmas: list[float]) -> tuple[float, float, float]:
    """Weighted least squares of y = m*x through the origin, plus reduced chi2.

    Forced through the origin because zero thickness must mean zero
    attenuation — that is not a fitted degree of freedom, it is the physics.
    Letting the intercept float would absorb a genuine systematic into a
    meaningless constant and make a broken result look fine.
    """
    w = [1.0 / (s * s) for s in sigmas]
    sxx = sum(wi * x * x for wi, x in zip(w, xs))
    sxy = sum(wi * x * y for wi, x, y in zip(w, xs, ys))
    slope = sxy / sxx
    sigma_slope = math.sqrt(1.0 / sxx)

    dof = max(len(xs) - 1, 1)
    chi2 = sum(wi * (y - slope * x) ** 2 for wi, x, y in zip(w, xs, ys)) / dof
    return slope, sigma_slope, chi2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("build/cttwin"),
                        help="path to the cttwin executable")
    parser.add_argument("--photons", type=int, default=1_000_000,
                        help="primaries per configuration (default 1e6)")
    parser.add_argument("--thicknesses", type=float, nargs="+",
                        default=list(VALIDATION_THICKNESSES_MM))
    parser.add_argument("--variance-study", action="store_true",
                        help="also sweep N at the thickest slab to fix the "
                             "photons-per-projection budget for Pass 5")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    exe = args.exe.resolve()
    if not exe.exists():
        sys.stderr.write(f"cttwin not found at {exe}\n")
        return 2

    ref = carbon_steel_reference()
    print("cttwin — Beer-Lambert validation (Pass 3)")
    print("=" * 78)
    print(f"  mu (NIST, 99Fe+1C, {ref.energy_mev*1000:.3f} keV) : "
          f"{ref.mu_per_cm:.7f} /cm")
    print(f"  primaries per configuration              : {args.photons:,}")
    print(f"  acceptance                               : "
          f"{ACCEPTANCE_PERCENT}% on the unscattered count")
    print()

    with tempfile.TemporaryDirectory(prefix="cttwin_bl_") as tmp:
        workdir = Path(tmp)

        # --- open beam reference (N0) -------------------------------------
        print("Open-beam reference:")
        n0 = run_cttwin(exe, [
            "/cttwin/phantom none",
            "/run/initialize",
            f"/run/beamOn {args.photons}",
        ], workdir, "open_beam", args.verbose)
        print()

        # --- slabs ---------------------------------------------------------
        print("Slab runs:")
        runs = [
            run_cttwin(exe, [
                "/cttwin/phantom slab",
                f"/cttwin/slabThickness {t} mm",
                "/run/initialize",
                f"/run/beamOn {args.photons}",
            ], workdir, f"slab_{t:g}mm", args.verbose)
            for t in args.thicknesses
        ]
        print()

        # --- per-thickness comparison --------------------------------------
        print("Per-thickness comparison against NIST XCOM")
        print("-" * 78)
        print(f"{'t (mm)':>7} {'measured':>10} {'predicted':>10} "
              f"{'dev %':>8} {'stat %':>8} {'verdict':>9} {'scatter %':>10}")

        fit_x, fit_y, fit_s = [], [], []
        failures = []

        for t, run in zip(args.thicknesses, runs):
            corr = air_correction(t, n0.frac_unscattered)
            measured = run.frac_unscattered / n0.frac_unscattered / corr
            predicted = ref.transmission(t)
            dev = (measured / predicted - 1.0) * 100.0

            sigma = math.hypot(run.binomial_sigma(), n0.binomial_sigma())
            ok = abs(dev) <= ACCEPTANCE_PERCENT
            if not ok:
                failures.append((t, dev))

            scatter = (1.0 - run.unscattered / run.total) * 100.0 if run.total else 0.0

            print(f"{t:>7.1f} {measured:>10.6f} {predicted:>10.6f} "
                  f"{dev:>+8.3f} {sigma*100:>8.3f} "
                  f"{'PASS' if ok else 'FAIL':>9} {scatter:>10.3f}")

            fit_x.append(t)
            fit_y.append(-math.log(measured))
            fit_s.append(sigma)   # d(-ln m)/m == relative sigma on m

        # --- fit ------------------------------------------------------------
        slope, sigma_slope, chi2 = weighted_linear_fit(fit_x, fit_y, fit_s)
        mu_sim_per_cm = slope * 10.0
        offset = (mu_sim_per_cm / ref.mu_per_cm - 1.0) * 100.0
        offset_sigma = (sigma_slope * 10.0 / ref.mu_per_cm) * 100.0

        print()
        print("Fitted attenuation coefficient (-ln(N/N0) vs t, through origin)")
        print("-" * 78)
        print(f"  mu from simulation : {mu_sim_per_cm:.7f} +/- "
              f"{sigma_slope*10:.7f} /cm")
        print(f"  mu from NIST XCOM  : {ref.mu_per_cm:.7f} /cm")
        print(f"  difference         : {offset:+.3f} % +/- {offset_sigma:.3f} %")
        print(f"  reduced chi2       : {chi2:.2f}")
        print()
        if chi2 < 3.0:
            print("  Attenuation is exponential in t within the fitted "
                  "uncertainties: the transport itself is behaving correctly.")
            print("  Any offset above is therefore a difference between "
                  "Geant4's cross-section")
            print("  library and NIST XCOM, not a defect in cttwin.")
        else:
            print("  WARNING: reduced chi2 is high — the measurements do NOT sit "
                  "on a single")
            print("  exponential. That points at cttwin, not at the reference "
                  "data. Check the")
            print("  slab thickness actually built and the unscattered gate "
                  "before reading")
            print("  anything into the per-thickness numbers above.")

        # --- variance vs N --------------------------------------------------
        if args.variance_study:
            print()
            print("Variance vs N at the thickest slab "
                  "(fixes the Pass 5 photon budget)")
            print("-" * 78)
            t_worst = max(args.thicknesses)
            print(f"{'N':>12} {'unscattered':>12} {'frac':>10} {'stat %':>8}")
            for n in (10_000, 30_000, 100_000, 300_000, 1_000_000):
                r = run_cttwin(exe, [
                    "/cttwin/phantom slab",
                    f"/cttwin/slabThickness {t_worst} mm",
                    "/run/initialize",
                    f"/run/beamOn {n}",
                ], workdir, f"var_{n}", False)
                print(f"{n:>12,} {r.unscattered:>12,} "
                      f"{r.frac_unscattered:>10.6f} "
                      f"{r.binomial_sigma()*100:>8.3f}")

    # --- verdict -----------------------------------------------------------
    print()
    print("=" * 78)
    if not failures:
        print(f"PASS — all thicknesses within {ACCEPTANCE_PERCENT}% of NIST XCOM.")
        return 0

    print(f"FAIL — {len(failures)} thickness(es) outside "
          f"{ACCEPTANCE_PERCENT}%:")
    for t, dev in failures:
        print(f"    t = {t:g} mm : {dev:+.3f} %")
    print()
    print("Before treating this as a cttwin defect, read the reduced chi2 above.")
    print("A low chi2 with a non-zero fitted offset means the simulation is")
    print("exponential and self-consistent, and the disagreement is with the")
    print("reference data — a different claim to make, not a bug to hunt.")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())