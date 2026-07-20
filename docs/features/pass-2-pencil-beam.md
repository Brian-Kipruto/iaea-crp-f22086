# Pass 2 — Collimated pencil beam

> Status: Complete 2026-07-20. Commit `b297975` on `main`. Geant4 11.2.1.

## What it does

Replaces the Pass 0 placeholder beam with a deliberate, documented **collimated
pencil beam**: a single, zero-width 662 keV gamma launched from the source plane
at `x = -kSourceToIso` and aimed along +x straight at the in-line detector pixel.
This is what makes each projection a clean **line integral** — every photon
travels one defined path from source to detector, so the transmitted count is a
Beer–Lambert measurement rather than an incidental arrival. It is the
foundation Pass 3's validation and Pass 5's sinogram both stand on.

Still not a scan: energy is fixed at 662 keV but not yet formalised as the
locked Cs-137 source (Pass 3), there is no runtime source translation (Pass 4),
and no rotation. What changed in Pass 2 is the *nature and intent* of the beam,
not the machinery around it.

## Files created / modified

New: none.

Modified:
```
src/PrimaryGeneratorAction.cc    Pass 0 placeholder block replaced wholesale with
                                 Pass 2 pencil-beam block (marker-wrapped)
include/PrimaryGeneratorAction.hh  class doc comment updated: pencil beam is now
                                 current, not future; idealisations documented
```

Deliberately untouched: `include/Constants.hh` (the beam aims *relative* to the
existing `kSourceToIso` / `kIsoToDetector` constants — no new constant needed),
and the placeholder scan geometry (Lockdown #10) — that stays flagged and
awaits Dr. Kairu's hardware SDD before Pass 3.

## The beam

```cpp
fParticleGun->SetParticleEnergy(662.0 * keV);
fParticleGun->SetParticlePosition(G4ThreeVector(-kSourceToIso, 0.0, 0.0));
fParticleGun->SetParticleMomentumDirection(G4ThreeVector(1.0, 0.0, 0.0));
```

- **Source** on the source plane at `x = -kSourceToIso` (150 mm), on-axis
  (`y = z = 0`).
- **Aim** +x, straight at the detector centre at `x = +kIsoToDetector`. The ray
  lands on the in-line pixel (pixel 0, the whole 50.8 mm face today).
- One primary per event, `G4ParticleGun`.

## Idealisations (locked for Phase 1)

Two realism knobs are deliberately turned off, each with a dated home for later:

- **Zero angular spread.** A real collimated source diverges <1°. Beam
  divergence is deferred to a later realism mode (Pass 6+, alongside NaI
  detector response), not Phase 1. See vault `Source - Cs-137`.
- **Zero beam-spot width — a point ray.** One path length per projection. A
  finite spot would sample a *spread* of chord lengths through the phantom and
  blur the Beer–Lambert line integral, defeating the whole reason the detector
  and beam are idealised. Confirmed as a decision this pass (Q2 of the Pass 2
  spec): keep zero-width now, defer real divergence.

Neither idealisation is a new locked decision — both follow directly from
Lockdown #1 (single unambiguous line) and #2 (idealised counter). No ADR was
opened.

## What changed from Pass 0 / v1

| v1 (reference) | Pass 0 placeholder | Pass 2 |
| --- | --- | --- |
| Ir-192, 3 made-up lines | 662 keV single line | 662 keV single line (Cs-137 formalised Pass 3) |
| 30° cone, >99% photons wasted | single +x ray (incidental) | single +x ray (deliberate pencil beam) |
| global namespace | `CTTwin` | `CTTwin` |

The subtle point: the Pass 0 placeholder was *already* a single `(1,0,0)` ray —
the v1 cone was never ported. So Pass 2's code delta is small. The pass's real
content is (a) making the ray a documented idealisation aimed at the in-line
pixel, and (b) the verification below proving it behaves as a line-integral
sampler.

## Checkpoint

Qualitative (quantitative 2% agreement is Pass 3's job with a flat slab — the
curved pipe wall gives a position-dependent chord and is not a clean
Beer–Lambert test). Two runs, 10k photons each:

| Config | Result | Reading |
| --- | --- | --- |
| empty world (`"none"`) | **0.9978 detected** | identical to Pass 1 — same single ray, unchanged, in-line pixel |
| pipe phantom (`"pipe"`) | **0.5012 detected** | count drops: the beam now samples a real line integral through steel |

The *drop* between the two is the signal. Empty-world staying at 0.9978
(≡ Pass 1) proves the pencil beam still hits the in-line pixel exactly; the
pipe drop proves it attenuates along a defined path. See
[validation/pass-2-pencil-beam.md](../validation/pass-2-pencil-beam.md).

## Gotcha — the empty-world checkpoint still needs a code change

Same wrinkle as Pass 1, and it recurs until Pass 4. The empty-world run needs
`fActivePhantom = "none"`, but the messenger that sets it at runtime is Pass 4.
So it was run by temporarily editing the default in `DetectorConstruction.hh` to
`"none"`, rebuilding, verifying, then reverting to `"pipe"` before commit. The
`"none"` default must never ship; committed default is `"pipe"`. Guard before
commit: `git status` shows only `PrimaryGeneratorAction.*`, and
`grep 'fActivePhantom = ' include/DetectorConstruction.hh` reads `"pipe"`.

## Related

- [features/pass-2-pencil-beam-retrospective.md](./pass-2-pencil-beam-retrospective.md)
- [validation/pass-2-pencil-beam.md](../validation/pass-2-pencil-beam.md)
- [features/pass-1-sensitive-detector.md](./pass-1-sensitive-detector.md) — the detector the beam aims at