# Pass 5 — the Python tier: scan driver, sinogram, reconstruction

**Date:** 2026-08-20
**Code commit:** `80d07a2`
**Status:** ✅ complete — all seven checkpoints passed
**Claim:** NUTECH 2026 abstract claim 2 (a reconstructed cross-section of the
multi-material phantom). **Banked.** Phase 1 is complete.

---

## What this pass built

Tier 2 and tier 3 of the three-tier architecture. The Geant4 tier was **not
touched** — no C++ file changed in this pass, which is why the Pass 1–4
regression anchors carry forward untouched and there was no rebuild in the
critical path.

| File | Status | Role |
|---|---|---|
| `python/run_scan.py` | filled (was a stub) | renders the macro template per angle, drives `cttwin`, collects CSVs |
| `python/assemble_sinogram.py` | filled (was a stub) | stacks CSVs into a verified sinogram |
| `python/reconstruct.py` | filled (was a stub) | line integrals → FBP + iterative → mu in /cm |
| `python/phantom_model.py` | **new** | analytic forward model of the Phase 1 phantoms |
| `python/xcom_reference.py` | modified | Al and polyethylene added alongside steel |
| `macros/full_scan_template.mac` | modified | `/run/numberOfThreads 1` pinned |

### `phantom_model.py` — why a fourth file

The pass plan called for three scripts. The fourth exists because the vault's
Pass 5 checkpoint was *"the Option B reconstruction is recognisable"*, and
every other checkpoint in this project is a number.

The Phase 1 phantoms are circles, the beam is a zero-width pencil, and the
chord of a ray through a circle is one line of trigonometry — so the entire
sinogram is predictable in closed form. That buys three things:

1. **The sinogram is validated before any reconstruction runs.** If it matches
   the model, the Geant4 tier and the driver are both correct and any remaining
   problem is in reconstruction. Two failures that would otherwise look
   identical are separated, in different files.
2. **The (theta, t) sign convention is determined, not assumed** — scored
   against the model under all four sign hypotheses.
3. **The reconstruction gets a ground truth**, turning "steel looks brighter
   than poly" into a per-material comparison against NIST.

---

## Data flow

```
run_scan.py
  ├─ one open-beam run  (phantom none, 1e7 primaries)      → open_beam.csv
  └─ one cttwin PROCESS per angle, N_t beamOn inside it    → angle_NNNN.csv
                                                            + manifest.json
assemble_sinogram.py
  └─ p = -ln( (N/n_events) / (N0/n0_events) )              → <scan>.npz
     verified against phantom_model before it is written
reconstruct.py
  └─ TomoPy gridrec + SIRT, scaled to /cm, oriented        → .npy + .png
```

### One process per angle

`/cttwin/scan/...` is legal after `/run/initialize` (ADR 0005), so a whole
translation sweep runs inside one process: **181 launches, not 14,661**.
`/cttwin/phantom` stays above `/run/initialize` because it rebuilds solids.

The vault's `04 - Code Architecture/Python Tier.md` still describes one process
per measurement. That is pre-Pass-4 and is superseded by this.

### Normalisation

Both counts are divided by their own `n_events` before the ratio. This is why
`n_events` is in the CSV at all: the open beam runs at 100x the statistics of a
projection, and a ratio of raw counts across different N is meaningless.

### Two sinograms, one scan

`n_counts` and `n_unscattered` are both in every row, so both are assembled and
either can be reconstructed (`--count`). The unscattered one is the primary
claim — it is the physics Pass 3 validated. The total one is the
scatter-contaminated realistic comparison. Zero extra simulation.

### `.npz`, not `.npy`

`03 - Scan & Detection/Output Format.md` specifies one `.npy`. That predates
two counts per row, an open-beam normalisation that has to travel with the
data, and axes with physical units the reconstruction needs to report mu in
/cm. A bare array carrying none of that is a file whose correct interpretation
lives in someone's memory — the exact failure mode ADR 0004 was opened to fix.
One self-describing `.npz` instead. Deliberate deviation.

---

## The scan grids

Two packages, both run in full. Angular sampling comfortably exceeds
(pi/2) x (samples across the object) in both, so angular sampling is never the
limit.

| | pitch | N_t | N_angles | measurements | primaries | wall |
|---|---|---|---|---|---|---|
| **A** | 2.0 mm | 81 | 180 @ 1° | 14,661 | 1.47e9 | 2.7 h (pipe, uncontended) |
| **B** | 1.0 mm | 161 | 240 @ 0.75° | 38,801 | 3.88e9 | 8.3 h (contended) |

`t` spans ±80 mm for both, covering the pipe (support radius 70.65 mm) and the
bars (75.00 mm) on one grid. An **odd** N_t symmetric about zero puts the
rotation axis exactly on a sample, so the geometric centre is
`(N_t - 1) / 2` exactly — confirmed by `find_center` to 0.00 px on package B.

Photons: **1e5 per measurement, 1e7 for the single open-beam run.** The open
beam gets three extra orders of magnitude because its error is *common-mode* —
it shifts every line integral by the same amount rather than averaging out, so
it is the cheapest place to spend photons. It costs the same as 100
measurements out of 14,661.

**One open-beam run, not a sweep.** Under ADR 0005 nothing in the world changes
with theta or t when the phantom is `none`, so every open-beam measurement
would be literally the same simulation.

---

## The z = 0 slice — and the baseplate that is not in it

The gun sits at (-250, 0, 0) firing along +x: a zero-width pencil beam
permanently in the plane z = 0. The Option B baseplate spans z in [-85, -75] mm
while the bars span z in [-74.99, +75.01] mm, and the scan transform only moves
the phantom in x and y.

**The beam plane never intersects the baseplate, at any (theta, t).**

Measured, not inferred: Pass 4 recorded theta = 30°, t = 0 → unscattered
0.44469, and 40 mm of aluminium *alone* predicts 0.44477 (0.02%, -0.05 sigma).
A baseplate in the beam would add ~200 mm of aluminium chord and multiply
everything by 0.018.

**The Option B reconstruction therefore shows seven bars and no ring.** The
"faint baseplate ring" in `Option B - Multi-Material Array.md`,
`Phase 2 - End-to-End Pipeline.md` and the Pass 4 handoff predates the pencil
beam and is wrong for this geometry.

---

## Reference coefficients

`xcom_reference.py` gained aluminium and polyethylene, derived the same way
steel is: NIST SRD 126 Table 3 elemental values, log-log interpolated to
661.657 keV, combined by Bragg additivity. Polyethylene's mass fractions come
from C2H4 stoichiometry rather than being typed in.

| material | mu/rho (cm²/g) | rho | mu (/cm) | HVL (mm) |
|---|---|---|---|---|
| CarbonSteel | 0.0735004 | 7.850 | 0.5769780 | 12.01 |
| G4_Al | 0.0746097 | 2.699 | 0.2013717 | 34.42 |
| G4_POLYETHYLENE | 0.0880085 | 0.940 | 0.0827280 | 83.79 |

Fe and C were re-checked against the live NIST tables at the same time and are
unchanged, so **the steel coefficient is bit-identical to Pass 3's** and
nothing downstream of ADR 0004 moves.

This closes the `Open Questions` entry *"derive mu/rho for aluminium and
polyethylene"*, and it re-reads Pass 4's Option B chord predictions: the
residuals recorded as "0.2–1.5%" with literature-grade values are **1.75 sigma
and 0.05 sigma** with the derived ones. They were statistical all along.

---

## Seeds

Every `cttwin` process starts from the same default seed, and runs within one
process draw sequentially from one stream. Without an explicit reseed per
measurement the same noise realisation lands at the same position in every
angle's sweep — **correlated noise down a sinogram column**, which reconstructs
as structure rather than grain.

The seed pair is a scrambled function of the global measurement index
`i = angle_index * n_translations + translation_index`, so no realisation
repeats anywhere in the scan and the whole scan replays exactly. The *scheme*
is recorded in `manifest.json`, not the seeds themselves — 39,000 integers is
an archive; a formula is reproducible.

---

## Operational behaviour

- **Rejected commands are fatal.** A rejected `/cttwin` command leaves the run
  using the *previous* configuration and still exits zero. `run_scan.py` scans
  stdout for `*** CTTwin ERROR`, `***** COMMAND NOT FOUND`, `command refused`
  and `illegal application state`, and refuses to keep that angle's numbers.
- **Rows are indexed on their own recorded coordinates**, never on file order.
  The CSV records what was actually built, so a row that landed at the wrong
  angle surfaces as a mismatch instead of being filed under the angle we meant.
- **Resume is safe; append is not.** Completed angle files are skipped;
  incomplete ones are deleted and redone. The CSV *appends*, so re-running an
  angle whose file exists would silently double its rows — the assembler
  detects duplicates and refuses.
- **Absolute output paths.** `cttwin` resolves relative paths against its own
  working directory (`build/`), which is why `c7-output-csv.mac` writes to
  `../data/raw/`. The driver passes absolute paths and sidesteps it.
- **Threads pinned to 1**, parallelism over angles. See ADR 0006.

---

## Checkpoint

**Reconstructed mu recovered to better than 1% for all three Option B
materials**, spanning Z_eff 5.5–26 and a factor of 7 in mu, by iterative
reconstruction on package B. Full ladder in
[`validation/pass-5-reconstruction.md`](../validation/pass-5-reconstruction.md).

---

## Gotchas for whoever comes next

- `pixel_index` is 0 in every row. The spatial coordinate within a projection
  is `translation_mm`. Assembling on `pixel_index` gives shape (n_angles, 1).
- TomoPy's `gridrec` needs **`rot 270`** to match our (x, y) convention.
  Confirmed on both packages, so it is a fixed library convention. The
  orientation is measured against the analytic phantom rather than hard-coded.
- The 180°-redundancy partner is **30°, not 0°**. At theta = 0 the Option B
  projection is *even* in t, and against an even row the correct redundancy
  relation and the wrong one are indistinguishable. See troubleshooting 003's
  sibling note in the retrospective.
- `roi_masks` erodes a fixed **physical** margin, not a pixel count. A pixel
  count makes a finer scan draw its ROI closer to the edge, so the two packages
  would not be comparable.
- Air is deliberately not modelled in `phantom_model`. Normalising by the open
  beam leaves +mu_air x (chord through the phantom): a -1e-5 /mm offset against
  mu_Al = 2.0e-2 /mm, i.e. 0.05%, an order below any single ray's statistics.
