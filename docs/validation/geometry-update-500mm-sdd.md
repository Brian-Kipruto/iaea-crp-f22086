# Validation — Geometry update to 500 mm SDD (re-baselined anchors)

> Date: 2026-07-20. Geant4 11.2.1 (MT). Commit `ab9e4ba`.
> Geometry: symmetric 250/250 mm, **500 mm SDD** ([ADR 0003](../decisions/0003-real-scan-geometry-500mm-sdd.md)),
> superseding the 150/150 mm placeholder.

Not a pass. This is the re-measurement of the two standing regression anchors
after the scan geometry changed from the placeholder to the confirmed hardware
value. It exists so Pass 3 inherits anchors that match the geometry it will
actually run on — otherwise the first Pass 3 mismatch looks like a physics bug
when it is really a stale baseline.

## Why the numbers moved — and why the two moved differently

The detector is unchanged (50.8 mm face, idealised air counter). The beam is
unchanged (zero-width pencil, 662 keV, on-axis). What changed is the
source-to-detector distance: 300 mm → 500 mm.

The two anchors did **not** move by the same relative amount, and that
difference is the physically informative result:

| | 300 mm SDD | 500 mm SDD | relative change |
| --- | --- | --- | --- |
| empty world | 0.9978 | 0.9960 | **−0.18%** |
| pipe | 0.5012 | 0.4797 | **−4.29%** |

A single mechanism — longer air path, more scatter out of the face — would have
moved both by roughly the same small amount. The pipe moved ~24× harder, so the
air path explains the empty-world change and **not** the pipe change.

**The mechanism is scatter from the steel, not from the air.**

- *Empty world (−0.18%):* almost nothing in the beam to scatter off. The tiny
  loss is the pure air-path effect over ~67% more air. Primaries all arrive.
- *Pipe (−4.29%):* roughly half the beam interacts in the wall, and a
  substantial fraction of those Compton-scatter forward at small angles. At
  300 mm those small-angle scattered photons still landed within the 50.8 mm
  face and were counted. At 500 mm the face subtends a smaller solid angle from
  the scattering point, so they miss it and are not counted.

What the longer SDD removed is **scatter contamination in the transmitted
count** — which was always the wrong quantity for a line-integral measurement.

### Consequence for Pass 3 (good news)

Beer–Lambert, exp(−μt), describes **primary transmission only**. At 500 mm the
counted signal is closer to pure primaries, so measured N/N₀ sits closer to
theory and the 2% agreement target gets *easier*, not harder. The hardware
geometry is better for validation than the placeholder was.

### Correction to an earlier claim

Before this measurement it was stated that Pass 3's validation numbers are
"SDD-invariant". That is too strong and is corrected here. The underlying
**attenuation physics** is SDD-invariant — exp(−μt) has no distance term. The
**measured count** is not, because scatter acceptance depends on the detector's
solid angle. This data quantifies the difference: 0.18% when there is no
scatterer in the beam, 4.29% when there is. Pass 3's validation doc should state
how much scatter remains in the measured number rather than assuming none
does.

## Checkpoints and results

| # | Check | Method | Expected | Actual |
| --- | --- | --- | --- | --- |
| g1 | Rebuild clean at new geometry | `rm -rf build && cmake .. && make -j` | `Built target cttwin` | ✅ |
| g2 | Detector placed at +250 mm, overlap-clean | overlap check on run | `PhysDetector ... OK!` | ✅ (pipe run also `PhysPipe ... OK!`) |
| g3 | **Empty-world anchor re-baselined** | `fActivePhantom="none"`, `beamOn 10000` | slightly below 0.9978 | ✅ **9960 / 10000 = 0.9960** |
| g4 | **Pipe anchor re-baselined** | `fActivePhantom="pipe"` (default), `beamOn 10000` | shifted from 0.5012 | ✅ **4797 / 10000 = 0.4797** |

## Anchors — before and after

| Config | At 300 mm SDD (ADR 0002) | At 500 mm SDD (ADR 0003) |
| --- | --- | --- |
| empty world, 10k @ 662 keV | 0.9978 | **0.9960** |
| pipe phantom, 10k @ 662 keV | 0.5012 | **0.4797** |

**These are the anchors Pass 3 onward should regress against.** The Pass 1 and
Pass 2 validation docs keep their original figures as a frozen record of what
was true at the placeholder geometry.

## Method notes

- Empty world required temporarily setting the `fActivePhantom` default to
  `"none"` (no messenger until Pass 4). Revert to `"pipe"` before commit; verify
  with `git status` and `grep 'fActivePhantom = ' include/DetectorConstruction.hh`.
- No source file was added or removed, but `Constants.hh` is a header included
  by the geometry — a full `rm -rf build` rebuild is used anyway to rule out any
  stale-object masking.
- `kWorldHalfSize` stays 500 mm; the detector at +250 mm sits well inside it.

## Verdict

Geometry update verified. Both anchors moved, in the expected direction, and the
*asymmetry* between them (−0.18% vs −4.29%) is itself diagnostic — it identifies
the dominant effect as loss of forward-scattered photons from the steel, not air
attenuation. Nothing here indicates a defect; the simulation is behaving as a
longer-baseline collimated setup should.

**0.9960 (empty) and 0.4797 (pipe) are the standing regression anchors from
Pass 3 onward.** The Pass 1/2 figures are superseded for regression purposes and
retained only as the frozen record of the placeholder geometry.

One open item this raises for Pass 3, not resolved here: the measured count
still contains *some* scatter. Quantifying that residual — e.g. by comparing
against a primaries-only count — would let the Beer–Lambert agreement be quoted
with a stated scatter contribution rather than an implicit assumption of none.
Worth deciding in the Pass 3 spec.

## Related

- [decisions/0003-real-scan-geometry-500mm-sdd.md](../decisions/0003-real-scan-geometry-500mm-sdd.md)
- [validation/pass-1-detector.md](./pass-1-detector.md) — original anchors at 300 mm SDD
- [validation/pass-2-pencil-beam.md](./pass-2-pencil-beam.md) — same anchors reproduced at 300 mm SDD