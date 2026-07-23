# Validation — Pass 1 (SensitiveDetector + detector volume)

> Date: 2026-07-19. Geant4 11.2.1 (MT). Commit `<fill after push>`.

> **Geometry note (added 2026-07-20).** The numbers on this page were measured
> at the ADR 0002 placeholder geometry: 150/150 mm, 300 mm SDD. That geometry
> was superseded on 2026-07-20 by the confirmed hardware value (500 mm SDD,
> symmetric — see [ADR 0003](../decisions/0003-real-scan-geometry-500mm-sdd.md)).
> These figures stay as the frozen record of what was true then; the *current*
> regression anchors are in
> [validation/geometry-update-500mm-sdd.md](./geometry-update-500mm-sdd.md).


First pass with a real measured number. The detector is verified in isolation
(empty world) before any transmission physics is trusted.

## Checkpoints and results

| # | Check | Method | Expected | Actual |
| --- | --- | --- | --- | --- |
| build | Compiles + links clean | `rm -rf build && cmake .. && make -j` | `Built target cttwin` | ✅ |
| 1a/1b | Detector placed, overlap-clean, SD attaches | `beamOn 10`, grep overlaps | `PhysDetector ... OK!` | ✅ `PhysDetector:0 (G4Box) ... OK!` (and PhysPipe OK) |
| 1c | Count chain prints a run total | `beamOn 10` | run summary with detector count | ✅ prints Events / Detector counts / Fraction |
| 1d | **10k photons, empty world → ~all in detector** | set `fActivePhantom="none"`, `beamOn 10000` | fraction ≈ 1.0 | ✅ **9978 / 10000 = 0.9978** |

## Cross-check (not a checkpoint, but recorded)

| Config | Beam | Result | Reading |
| --- | --- | --- | --- |
| empty world | 10k @ 662 keV +x | 0.9978 detected | detector + geometry sound; ~22 lost to air scatter over 300 mm |
| pipe phantom | 10k @ 662 keV +x | 0.5012 detected | ~half attenuated by the carbon-steel pipe wall — reproducible |

The pipe number is a ready sanity anchor for Pass 3's Beer–Lambert (it is a
geometry-dependent coincidence at ~0.50, not a physics constant, but it is
stable and useful as a regression check).

## Method notes

- Empty world required temporarily changing the `fActivePhantom` default to
  `"none"` (no messenger until Pass 4). Reverted to `"pipe"` before commit.
- `ProcessHits` counts gamma entries at `fGeomBoundary` — one count per photon,
  not per step, verified by the empty-world fraction landing at ~1.0 rather than
  >1.0 (which multi-counting would produce).
- The ~0.998 (not exactly 1.0) is physical: Compton scatter in the 300 mm air
  gap deflects a small fraction off the 50.8 mm face.

## Verdict

Pass 1 checkpoint met. Detector counts correctly and in isolation. Foundation
sound for Pass 2 (pencil beam replacing the placeholder).

## Related

- [features/pass-1-sensitive-detector.md](../features/pass-1-sensitive-detector.md)
- [decisions/0002-placeholder-scan-geometry.md](../decisions/0002-placeholder-scan-geometry.md) — the +150 mm the detector sits at