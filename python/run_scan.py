"""
run_scan.py — Pass 5, tier 2. Drive cttwin through a full translate-rotate scan.

Renders macros/full_scan_template.mac once per ANGLE, launches cttwin once per
angle, and collects one CSV per angle into data/raw/<scan>/. Plus exactly one
open-beam run for N0.

    python3 python/run_scan.py --anchors                  # checkpoint 0
    python3 python/run_scan.py --smoke                    # checkpoint 1
    python3 python/run_scan.py --package A --phantom pipe # checkpoint 3
    python3 python/run_scan.py --package A --phantom bars # the deliverable

Subprocess handling and CTTWIN_RESULT parsing are lifted from
validate_beer_lambert.py, which already solved both in Pass 3. In particular
the "*** CTTwin ERROR" scan: a rejected /cttwin command leaves the run using
the PREVIOUS configuration and exiting zero, so a driver that only checks the
return code will cheerfully record 180 projections taken at the wrong angle.

ONE PROCESS PER ANGLE (ADR 0005 / D2)
-------------------------------------
/cttwin/scan/... is legal after /run/initialize, so a whole translation sweep
runs inside one process. 180 launches, not 23,000. /cttwin/phantom stays above
/run/initialize because it rebuilds solids.

SEEDS
-----
Every cttwin process starts from the same default seed and runs within one
process draw sequentially from one stream. Without an explicit reseed per
measurement the same noise realisation lands at the same position in every
angle's sweep — correlated noise down a sinogram COLUMN, which reconstructs as
structure rather than grain. The seed here is a scrambled function of the
global measurement index, so no realisation repeats anywhere in the scan and
the whole scan replays exactly. The scheme is recorded in manifest.json rather
than the seeds themselves: 39,000 integers in a JSON file is an archive, a
formula is reproducible.

THREADS
-------
Pinned to 1 in the template, and parallelism is over angles here instead.
Angles are independent processes, so a pool of single-threaded workers avoids
the per-run merge and start-up cost of G4 MT entirely. It also removes a
variable: whether Geant4 11.2's event seeding is thread-count independent
depends on seedOncePerCommunication, which this project has never measured.
Fixing the count sidesteps the question — and --anchors checks that the
Pass 1-4 regression anchors survive the pinning before any long scan starts.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import math
import os
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
TEMPLATE = REPO / "macros" / "full_scan_template.mac"

RESULT_RE = re.compile(r"^CTTWIN_RESULT\s+(.*)$", re.MULTILINE)

# A rejected G4 command does not fail the process either. These are the strings
# Geant4 and cttwin print when a command does not land.
REFUSAL_MARKERS = ("*** CTTwin ERROR",
                   "***** COMMAND NOT FOUND",
                   "command refused",
                   "illegal application state")


# ---------------------------------------------------------------------------
# Scan packages (D1)

@dataclass(frozen=True)
class ScanPackage:
    name: str
    t_step_mm: float
    t_half_range_mm: float
    angle_step_deg: float
    photons: int
    open_beam_photons: int

    @property
    def translations(self):
        n = int(round(2 * self.t_half_range_mm / self.t_step_mm)) + 1
        return [round(-self.t_half_range_mm + i * self.t_step_mm, 6)
                for i in range(n)]

    @property
    def angles(self):
        # [0, 180) for the reconstruction, plus ONE extra angle acquired purely
        # to re-test p(theta,t) = p(theta+180, -t) end to end. Pass 4 verified
        # that relation inside the Geant4 tier to 0.26 sigma; this re-tests it
        # through the driver and the assembler as well, for 1/180th of the cost.
        # Not fed to the reconstruction.
        #
        # THE PARTNER IS 30 deg, NOT 0 deg, AND THAT MATTERS.
        # The obvious choice is 180 deg paired with 0 deg. It does not work:
        # at theta = 0 the Option B bars sit at y = 60*sin(phi) for
        # phi = 0,60,...,300, i.e. at 0, +51.96, +51.96, 0, -51.96, -51.96 mm,
        # with steel at phi = 0,120,240 and poly at 60,180,300 — so each
        # material appears symmetrically at +y and -y and the whole projection
        # is EVEN in t. Against an even row, p(theta+180,-t) and the WRONG
        # p(theta+180,+t) are identical, and the test silently passes either
        # way. (Measured: both gave 0.8 sigma.)
        #
        # At theta = 30 deg the offsets are +30, +60, +30, -30, -60, -30 mm
        # with steel (r = 10) at +30 and poly (r = 15) at -30: genuinely
        # asymmetric, so the two relations differ and the test discriminates.
        # 30 deg is on the grid for every package here.
        n = int(round(180.0 / self.angle_step_deg))
        return ([round(i * self.angle_step_deg, 6) for i in range(n)]
                + [REDUNDANCY_PARTNER_DEG + 180.0])


# The reconstruction angle whose theta+180 partner is acquired as a redundancy
# check. See ScanPackage.angles for why this is 30 and not 0.
REDUNDANCY_PARTNER_DEG = 30.0

PACKAGES = {
    # Matched grids: N_angles comfortably exceeds (pi/2) * (samples across the
    # object), so angular sampling is never the limit on either.
    "A": ScanPackage("A", 2.0, 80.0, 1.00, 100_000, 10_000_000),
    "B": ScanPackage("B", 1.0, 80.0, 0.75, 100_000, 10_000_000),
}


def seeds_for(index: int) -> tuple[int, int]:
    """Two independent-looking 31-bit seeds from a global measurement index.

    Multiplicative scrambling rather than index+1: sequential seeds into
    HepJamesRandom are not obviously harmful, but adjacent streams are exactly
    the correlation this whole mechanism exists to avoid, and scrambling costs
    nothing. Deterministic, so the scan replays bit-for-bit.
    """
    s1 = (index * 2_654_435_761 + 12_345) % 2_147_483_647
    s2 = (index * 40_503 + 987_654_321) % 2_147_483_647
    return s1 + 1, s2 + 1


# ---------------------------------------------------------------------------
# Running cttwin

def parse_result(stdout: str) -> dict:
    matches = RESULT_RE.findall(stdout)
    if not matches:
        raise RuntimeError(
            "No CTTWIN_RESULT line in cttwin output. Either the run failed, or "
            "RunAction::EndOfRunAction changed and this parser is stale.")
    return dict(kv.split("=", 1) for kv in matches[-1].split())


def run_macro(exe: Path, lines: list[str], workdir: Path, label: str,
              verbose: bool = False) -> str:
    macro = workdir / f"{label}.mac"
    macro.write_text("\n".join(lines) + "\n")
    proc = subprocess.run([str(exe), str(macro)],
                          capture_output=True, text=True, cwd=exe.parent)
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout[-4000:] + proc.stderr[-4000:])
        raise RuntimeError(f"cttwin exited {proc.returncode} on {label}")
    for marker in REFUSAL_MARKERS:
        if marker in proc.stdout or marker in proc.stderr:
            sys.stderr.write(proc.stdout[-4000:])
            raise RuntimeError(
                f"{label}: a command was rejected ({marker!r}). The run did "
                "NOT use the requested configuration — refusing to keep its "
                "numbers.")
    if verbose:
        print(proc.stdout[-1500:])
    return proc.stdout


def render_angle_macro(package: ScanPackage, phantom: str, angle_deg: float,
                       angle_index: int, out_csv: Path) -> str:
    """Fill full_scan_template.mac for one angle's whole translation sweep.

    COMMENT LINES ARE NOT SUBSTITUTED, and that is not fussiness.
    full_scan_template.mac documents its own placeholders in its header
    comment — it literally contains the sentence "Pass 5 will substitute
    {{PHANTOM}}, {{OUTPUT_FILE}}, {{ANGLE}} and {{TRANSLATION_BLOCK}}." A
    whole-file str.replace hits those occurrences too, which injects the
    ENTIRE translation block into the middle of a comment. Only its first
    line stays commented; every line after it becomes a live command, running
    the whole sweep a second time ABOVE /run/initialize and therefore at the
    default phantom. The run still exits zero and still writes a plausible
    CSV.

    Substituting line by line, skipping comments, removes the failure mode
    rather than relying on the template never explaining itself.

    The unit on the angle is ALSO already in the template
    (`/cttwin/scan/angle {{ANGLE}} deg`), so only the number is substituted.
    "30.0 deg deg" would be rejected by G4UIcmdWithADoubleAndUnit, and a
    rejected scan/angle leaves the run at the PREVIOUS angle while exiting
    zero — caught by REFUSAL_MARKERS, but cheaper not to write.
    """
    n_t = len(package.translations)
    block = []
    for j, t in enumerate(package.translations):
        s1, s2 = seeds_for(angle_index * n_t + j)
        block += [f"/random/setSeeds {s1} {s2}",
                  f"/cttwin/output/projectionId {j}",
                  f"/cttwin/scan/translation {t} mm",
                  f"/run/beamOn {package.photons}"]

    fields = {"{{PHANTOM}}": phantom,
              "{{OUTPUT_FILE}}": str(out_csv),
              "{{ANGLE}}": f"{angle_deg}",
              "{{TRANSLATION_BLOCK}}": "\n".join(block)}

    out, seen = [], set()
    for line in TEMPLATE.read_text().splitlines():
        if line.lstrip().startswith("#"):
            out.append(line)
            continue
        for token, value in fields.items():
            if token in line:
                line = line.replace(token, value)
                seen.add(token)
        out.append(line)

    unfilled = set(fields) - seen
    if unfilled:
        raise RuntimeError(
            f"{TEMPLATE.name} has no substitutable line for {sorted(unfilled)}. "
            "The template and this driver have drifted apart.")
    return "\n".join(out) + "\n"


def csv_is_complete(path: Path, n_rows: int) -> bool:
    """True if this angle already ran to completion.

    Matters because the CSV APPENDS. Re-running an angle whose file already
    exists silently doubles its rows, and a sinogram assembled from a file with
    2*n_t rows is not obviously wrong until much later. So: complete files are
    skipped, incomplete ones are deleted and redone. Never appended to.
    """
    if not path.exists():
        return False
    lines = [ln for ln in path.read_text().splitlines() if ln.strip()]
    return len(lines) == n_rows + 1          # + header


def run_angle(exe: Path, package: ScanPackage, phantom: str, out_dir: Path,
              angle_index: int, angle_deg: float, force: bool) -> dict:
    csv = out_dir / f"angle_{angle_index:04d}.csv"
    n_t = len(package.translations)
    if not force and csv_is_complete(csv, n_t):
        return {"angle_index": angle_index, "angle_deg": angle_deg,
                "csv": csv.name, "skipped": True, "seconds": 0.0}
    csv.unlink(missing_ok=True)

    macro_dir = out_dir / "macros"
    macro_dir.mkdir(exist_ok=True)
    macro = macro_dir / f"angle_{angle_index:04d}.mac"
    macro.write_text(render_angle_macro(package, phantom, angle_deg,
                                        angle_index, csv))

    t0 = time.time()
    proc = subprocess.run([str(exe), str(macro)],
                          capture_output=True, text=True, cwd=exe.parent)
    elapsed = time.time() - t0
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout[-3000:] + proc.stderr[-3000:])
        raise RuntimeError(f"cttwin exited {proc.returncode} at "
                           f"theta = {angle_deg} deg")
    for marker in REFUSAL_MARKERS:
        if marker in proc.stdout or marker in proc.stderr:
            sys.stderr.write(proc.stdout[-3000:])
            raise RuntimeError(
                f"theta = {angle_deg} deg: a command was rejected ({marker!r}). "
                "Refusing to keep this angle.")
    if not csv_is_complete(csv, n_t):
        raise RuntimeError(
            f"theta = {angle_deg} deg: expected {n_t} rows in {csv}, "
            "did not get them. If the file is missing entirely, cttwin could "
            "not open the path — it resolves against build/, which is why this "
            "driver passes absolute paths.")
    return {"angle_index": angle_index, "angle_deg": angle_deg,
            "csv": csv.name, "skipped": False, "seconds": elapsed}


# ---------------------------------------------------------------------------
# Checkpoint modes

def checkpoint_anchors(exe: Path, workdir: Path) -> int:
    """Checkpoint 0 — do the Pass 1-4 regression anchors survive threads=1?

    NOTE: deliberately NO /random/setSeeds. The anchors were measured on the
    default seed, and the point is to reproduce their conditions exactly except
    for the thread count. Adding a reseed here would change the realisation and
    make a pass or a fail equally uninterpretable.
    """
    print("Checkpoint 0 — regression anchors at /run/numberOfThreads 1")
    print("=" * 74)
    expected = {"none": (0.99600, 0.99580), "pipe": (0.47950, 0.47060)}
    worst, ok = 0.0, True
    for phantom, (exp_total, exp_unsc) in expected.items():
        out = run_macro(exe, ["/run/numberOfThreads 1",
                              f"/cttwin/phantom {phantom}",
                              "/run/initialize",
                              "/run/beamOn 10000"], workdir, f"anchor_{phantom}")
        r = parse_result(out)
        n = int(r["events"])
        tot, unsc = int(r["total"]) / n, int(r["unscattered"]) / n
        sigma = math.sqrt(exp_total * (1 - exp_total) / n)
        dev = (tot - exp_total) / sigma
        worst = max(worst, abs(dev))
        exact = abs(tot - exp_total) < 1e-9
        print(f"  {phantom:<5} total {tot:.5f} (expect {exp_total:.5f})  "
              f"unscattered {unsc:.5f} (expect {exp_unsc:.5f})  "
              f"{'EXACT' if exact else f'{dev:+.2f} sigma'}")
        if not exact and abs(dev) > 3.0:
            ok = False
    print()
    if worst < 1e-9:
        print("  Anchors reproduce EXACTLY. Thread pinning is transparent: "
              "Geant4's\n  event seeding here is thread-count independent.")
    elif ok:
        print(f"  Anchors move but agree within {worst:.2f} sigma. That is a "
              "different\n  realisation, not a different geometry — expected if "
              "seeding is\n  thread-count dependent. Record the new values and "
              "carry on.")
    else:
        print("  ANCHORS FAILED beyond 3 sigma. Do NOT start a long scan. "
              "Something\n  structural changed; this is not a seeding artefact.")
        return 1
    return 0


def checkpoint_smoke(exe: Path, workdir: Path, out_dir: Path) -> int:
    """Checkpoint 1 — the driver, against numbers Pass 4 already banked."""
    print("Checkpoint 1 — smoke scan (bars, 3 angles x 5 translations)")
    print("=" * 74)
    out_dir.mkdir(parents=True, exist_ok=True)
    pkg = ScanPackage("smoke", 20.0, 40.0, 30.0, 100_000, 0)
    for i, theta in enumerate((0.0, 30.0, 60.0)):
        csv = out_dir / f"angle_{i:04d}.csv"
        csv.unlink(missing_ok=True)
        macro = workdir / f"smoke_{i}.mac"
        macro.write_text(render_angle_macro(pkg, "bars", theta, i, csv))
        run_macro(exe, macro.read_text().splitlines(), workdir, f"smoke_run_{i}")

    import csv as csvmod
    rows = []
    for i in range(3):
        with open(out_dir / f"angle_{i:04d}.csv") as fh:
            rows += list(csvmod.DictReader(fh))
    print(f"  rows written: {len(rows)} (expect 15)")

    ok = True
    for theta, expected in ((0.0, 0.11118), (30.0, 0.44469)):
        hit = [r for r in rows
               if abs(float(r["angle_deg"]) - theta) < 1e-6
               and abs(float(r["translation_mm"])) < 1e-6]
        if not hit:
            print(f"  theta={theta}, t=0 : ROW MISSING")
            ok = False
            continue
        r = hit[0]
        frac = int(r["n_unscattered"]) / int(r["n_events"])
        sigma = math.sqrt(frac * (1 - frac) / int(r["n_events"]))
        dev = (frac - expected) / sigma
        print(f"  theta={theta:>5.1f}, t=0 : {frac:.5f} vs Pass 4 {expected:.5f}"
              f"  ({dev:+.2f} sigma)")
        if abs(dev) > 4.0:
            ok = False

    # theta = 60 deg must equal theta = 0 deg: Option B's known 60 deg
    # degeneracy at t = 0 (same materials on the ray, opposite order, and
    # attenuation is order-independent). Correct physics that looks like stuck
    # geometry — so it is worth asserting rather than being alarmed by.
    a0 = [r for r in rows if float(r["angle_deg"]) == 0.0
          and float(r["translation_mm"]) == 0.0][0]
    a60 = [r for r in rows if float(r["angle_deg"]) == 60.0
           and float(r["translation_mm"]) == 0.0][0]
    f0 = int(a0["n_unscattered"]) / int(a0["n_events"])
    f60 = int(a60["n_unscattered"]) / int(a60["n_events"])
    s = math.hypot(math.sqrt(f0 * (1 - f0) / int(a0["n_events"])),
                   math.sqrt(f60 * (1 - f60) / int(a60["n_events"])))
    print(f"  60 deg degeneracy   : {f0:.5f} vs {f60:.5f}  "
          f"({(f0 - f60) / s:+.2f} sigma, expect ~0)")

    print()
    print("  PASS" if ok else "  FAIL — do not start the full scan")
    return 0 if ok else 1


# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", type=Path, default=REPO / "build" / "cttwin")
    ap.add_argument("--package", choices=sorted(PACKAGES), default="A")
    ap.add_argument("--phantom", choices=("pipe", "bars"), default="bars")
    ap.add_argument("--photons", type=int, default=None,
                    help="override the package's photons per measurement")
    ap.add_argument("--workers", type=int, default=os.cpu_count())
    ap.add_argument("--out", type=Path, default=None)
    ap.add_argument("--force", action="store_true",
                    help="re-run angles that already have complete CSVs")
    ap.add_argument("--anchors", action="store_true", help="checkpoint 0 only")
    ap.add_argument("--smoke", action="store_true", help="checkpoint 1 only")
    args = ap.parse_args()

    exe = args.exe.resolve()
    if not exe.exists():
        sys.stderr.write(f"cttwin not found at {exe}\n")
        return 2

    workdir = REPO / "data" / "raw" / "_macros"
    workdir.mkdir(parents=True, exist_ok=True)

    if args.anchors:
        return checkpoint_anchors(exe, workdir)
    if args.smoke:
        return checkpoint_smoke(exe, workdir,
                                REPO / "data" / "raw" / "smoke_bars")

    pkg = PACKAGES[args.package]
    if args.photons:
        pkg = ScanPackage(pkg.name, pkg.t_step_mm, pkg.t_half_range_mm,
                          pkg.angle_step_deg, args.photons,
                          pkg.open_beam_photons)

    scan = args.out or (REPO / "data" / "raw"
                        / f"{args.phantom}_pkg{pkg.name}")
    scan.mkdir(parents=True, exist_ok=True)

    angles, translations = pkg.angles, pkg.translations
    n_meas = len(angles) * len(translations)
    print(f"cttwin scan — phantom '{args.phantom}', package {pkg.name}")
    print("=" * 74)
    print(f"  angles       : {len(angles)} "
          f"(0 to {angles[-2]} deg step {pkg.angle_step_deg}, "
          f"+ 180 deg redundancy check)")
    print(f"  translations : {len(translations)} "
          f"({translations[0]} to {translations[-1]} mm "
          f"step {pkg.t_step_mm})")
    print(f"  photons      : {pkg.photons:,} per measurement "
          f"({n_meas:,} measurements, {n_meas * pkg.photons:.3g} primaries)")
    print(f"  workers      : {args.workers}")
    print(f"  output       : {scan}")
    print()

    # --- open beam. ONE run, not a sweep: under ADR 0005 nothing in the world
    # changes with theta or t when the phantom is 'none', so every open-beam
    # measurement is literally the same simulation. High statistics because its
    # error is common-mode — it shifts every line integral by the same amount
    # rather than averaging out, so it is the cheapest place to spend photons.
    n0_csv = scan / "open_beam.csv"
    if args.force or not csv_is_complete(n0_csv, 1):
        n0_csv.unlink(missing_ok=True)
        print(f"  open beam ({pkg.open_beam_photons:,} primaries) ...",
              end="", flush=True)
        t0 = time.time()
        run_macro(exe, ["/run/verbose 0", "/event/verbose 0",
                        "/tracking/verbose 0",
                        "/run/numberOfThreads 1",
                        "/cttwin/phantom none",
                        "/run/initialize",
                        f"/cttwin/output/file {n0_csv}",
                        "/cttwin/output/projectionId 0",
                        "/random/setSeeds 20260819 8675309",
                        f"/run/beamOn {pkg.open_beam_photons}"],
                  workdir, "open_beam")
        print(f" done in {time.time() - t0:.1f} s")
    else:
        print("  open beam already present, skipping")

    # --- the sweep
    t_start = time.time()
    done, records = 0, []
    with concurrent.futures.ThreadPoolExecutor(args.workers) as pool:
        futures = {pool.submit(run_angle, exe, pkg, args.phantom, scan,
                               i, th, args.force): (i, th)
                   for i, th in enumerate(angles)}
        for fut in concurrent.futures.as_completed(futures):
            rec = fut.result()
            records.append(rec)
            done += 1
            elapsed = time.time() - t_start
            rate = done / elapsed if elapsed else 0
            eta = (len(angles) - done) / rate if rate else 0
            print(f"\r  angles {done}/{len(angles)}  "
                  f"elapsed {elapsed/60:.1f} min  "
                  f"ETA {eta/60:.1f} min      ", end="", flush=True)
    print()

    ran = [r for r in records if not r["skipped"]]
    total_s = time.time() - t_start
    if ran:
        primaries = len(ran) * len(translations) * pkg.photons
        wall = sum(r["seconds"] for r in ran)
        print(f"\n  Checkpoint 2 — throughput")
        print(f"    {primaries:,} primaries in {wall/3600:.2f} core-hours "
              f"({total_s/60:.1f} min wall on {args.workers} workers)")
        print(f"    {primaries / wall:,.0f} primaries/s/core")
        for other in sorted(PACKAGES):
            p = PACKAGES[other]
            n = len(p.angles) * len(p.translations) * p.photons
            print(f"    package {other} would take "
                  f"{n / (primaries / wall) / 3600 / args.workers:.1f} h wall "
                  f"at this rate")

    manifest = {
        "scan": scan.name,
        "phantom": args.phantom,
        "package": asdict(pkg),
        "angles_deg": angles,
        "translations_mm": translations,
        "reconstruction_angles_deg": angles[:-1],
        "redundancy_check_angle_deg": angles[-1],
        "redundancy_partner_deg": REDUNDANCY_PARTNER_DEG,
        "open_beam_csv": n0_csv.name,
        "threads_per_process": 1,
        "seed_scheme": ("s1 = (i*2654435761 + 12345) % 2147483647 + 1, "
                        "s2 = (i*40503 + 987654321) % 2147483647 + 1, "
                        "i = angle_index * n_translations + translation_index"),
        "exe": str(exe),
        "git_commit": _git_commit(),
        "generated": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "wall_seconds": total_s,
        "angles_run": len(ran),
        "angles_skipped": len(records) - len(ran),
    }
    (scan / "manifest.json").write_text(json.dumps(manifest, indent=2))
    print(f"\n  manifest -> {scan / 'manifest.json'}")
    print(f"  next: python3 python/assemble_sinogram.py --scan {scan}")
    return 0


def _git_commit() -> str:
    try:
        return subprocess.run(["git", "rev-parse", "--short", "HEAD"],
                              cwd=REPO, capture_output=True, text=True,
                              check=True).stdout.strip()
    except Exception:
        return "unknown"


if __name__ == "__main__":
    raise SystemExit(main())