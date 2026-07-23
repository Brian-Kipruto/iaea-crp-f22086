# Validation — Pass 2 (Collimated pencil beam)

> Date: 2026-07-20. Geant4 11.2.1 (MT). Commit `b297975`.

> **Geometry note (added 2026-07-20).** The numbers on this page were measured
> at the ADR 0002 placeholder geometry: 150/150 mm, 300 mm SDD. That geometry
> was superseded on 2026-07-20 by the confirmed hardware value (500 mm SDD,
> symmetric — see [ADR 0003](../decisions/0003-real-scan-geometry-500mm-sdd.md)).
> These figures stay as the frozen record of what was true then; the *current*
> regression anchors are in
> [validation/geometry-update-500mm-sdd.md](./geometry-update-500mm-sdd.md).


Qualitative checkpoint. Pass 2 proves the pencil beam samples a line integral —
unchanged through vacuum, attenuated through material. The *quantitative* 2%
Beer–Lambert agreement is deferred to Pass 3, deliberately, because it needs a
flat slab (the curved pipe wall gives a position-dependent chord length and is
not a clean test).

## Checkpoints and results

| # | Check | Method | Expected | Actual |
| --- | --- | --- | --- | --- |
| build | Compiles + links clean | `rm -rf build && cmake .. && make -j` | `Built target cttwin` | ✅ |
| 2a | Empty world → beam still lands in-line, unchanged from Pass 1 | set `fActivePhantom="none"`, `beamOn 10000` | ≈ 0.9978 (≡ Pass 1) | ✅ **9978 / 10000 = 0.9978** |
| 2b | **Phantom in → count drops (line integral sampled)** | `fActivePhantom="pipe"` (default), `beamOn 10000` | clear, reproducible drop | ✅ **5012 / 10000 = 0.5012** |

The pass condition is the *relationship* between 2a and 2b: empty-world
identical to Pass 1 (beam geometry untouched by the rewrite) AND a clear drop
with the phantom in (beam attenuates along a defined path). Both hold.

## Why empty-world ≡ Pass 1 is the key result

The Pass 0 placeholder was already a single +x ray, so a correct Pass 2 must
leave the empty-world number *exactly* where Pass 1 left it. 0.9978 → 0.9978 is
the regression check: the pencil-beam rewrite changed framing and documentation,
not trajectory. Had this moved, the beam position/aim would have been wrong.

## Method notes

- Empty world required temporarily setting the `fActivePhantom` default to
  `"none"` (no messenger until Pass 4). Reverted to `"pipe"` before commit;
  verified with `git status` (only `PrimaryGeneratorAction.*` staged) and
  `grep 'fActivePhantom = '` reading `"pipe"`.
- Overlap check on the empty-world run printed only `PhysDetector:0 (G4Box)
  ... OK!` (no `PhysPipe`), confirming the phantom was genuinely absent.
- The ~0.998 (not 1.0) is physical: Compton scatter in the 300 mm air gap
  deflects a small fraction off the 50.8 mm face — same as Pass 1.
- Pipe 0.5012 is a geometry-dependent value (curved wall, two chords at y=0),
  not a physics constant. It is stable and reused as a regression anchor, but it
  is **not** the Beer–Lambert number — that is Pass 3 on a flat slab.

## Verdict

Pass 2 checkpoint met. The beam is a documented, zero-width collimated pencil
beam that samples a clean line integral. Foundation sound for Pass 3 (Cs-137
source formalised + explicit Beer–Lambert validation on a flat slab).

## Related

- [features/pass-2-pencil-beam.md](../features/pass-2-pencil-beam.md)
- [validation/pass-1-detector.md](./pass-1-detector.md) — the 0.9978 / 0.5012 anchors this pass reproduces
- [decisions/0002-placeholder-scan-geometry.md](../decisions/0002-placeholder-scan-geometry.md) — the geometry the beam aims across; revisit before Pass 3