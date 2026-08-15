# 0005 — The phantom carries the scan transform; the rig never moves

- **Status:** Accepted
- **Date:** 2026-08-15
- **Pass:** 4
- **Commit:** `e240918`
- **Supersedes:** nothing. **Superseded by:** nothing.
- **Related:** [0003](./0003-real-scan-geometry-500mm-sdd.md) (scan geometry),
  [0004](./0004-beer-lambert-reference-and-acceptance.md) (unscattered counting),
  [validation/pass-4-scan-motion.md](../validation/pass-4-scan-motion.md)

## Context

Pass 4 had to make the scan coordinates (θ, t) settable at runtime. A
first-generation translate–rotate scanner moves the source–detector pair across
the object in y (translation, inner loop) and steps the angle between sweeps
(rotation, outer loop). The vault leaned toward rotating the phantom and keeping
the rig fixed, but had never settled it, and had never addressed translation at
all.

Four questions were entangled and had to be answered together, because the
answer to each constrains the others:

1. What physically moves — the phantom, or the source–detector pair?
2. When may it move — before `/run/initialize` only, or between runs?
3. Does the detector need to become wide and pixelated?
4. What happens to the unscattered gate in `SensitiveDetector.cc`, which was
   hard-coded to test the arriving direction against +x and would collapse to
   zero the moment the beam was steered?

## Decision

**The phantom carries the entire scan transform. The source and detector are
fixed in world coordinates for the life of the process.**

For a phantom point `p` expressed in the phantom's own frame (its position at
θ = 0):

```
p_world = R_z(θ) · p  −  t · ŷ
```

The first term is the canonical rotation of `[[Coordinate Conventions]]` — θ
about +z, counterclockwise viewed from +z. The second is the whole system
rigidly shifted by −t so the rig can stay at y = 0 instead of moving to y = +t.
Relative geometry, which is all a line integral sees, is identical to the
canonical picture.

Consequent decisions, all adopted:

- **Scan motion is legal after `/run/initialize`.** `/cttwin/scan/angle` and
  `/cttwin/scan/translation` mutate existing placements via `SetTranslation` /
  `SetRotation` followed by `G4RunManager::GeometryHasBeenModified()`. Phantom
  selection and slab thickness remain pre-init only and are still rejected
  loudly, because they rebuild solids.
- **The detector stays one pixel, 50.8 mm.** No widening, no pixelation.
- **The unscattered gate reads the launch state from the event's own primary
  vertex** rather than comparing against a hard-coded axis and a constant.
- **Output is one CSV row per `beamOn`, appended**, with `n_unscattered` and
  `n_events` added to the column set from `[[Output Format]]`.

## Why moving the phantom is exact, not approximate

The argument that decided this is stronger than "physically equivalent."

The detector volume is **`G4_AIR` inside a `G4_AIR` world**. It is invisible to
photon transport — it attenuates nothing and scatters nothing; it exists only as
a surface for the `SensitiveDetector` to count crossings on. So "a small
detector that translates in y" and "a detector that sits still while the phantom
slides past it" are not approximately the same problem, they are the *same
transport problem*. Nothing is lost by choosing either.

Given that, moving the phantom wins on every secondary count:

- The gun stays at `(−kSourceToIso, 0, 0)` aimed +x permanently, so the
  unscattered gate's hard-coded axis was never actually going to break. The
  Pass 4 "landmine" is defused rather than fixed.
- The detector placement never changes, so the beam always lands on the in-line
  pixel and the single-pixel model of `[[Detector Model]]` survives untouched.
- One transform to reason about instead of two moving bodies.
- The Pass 1–3 regression anchors are unaffected at θ = 0, t = 0 — and this was
  *verified exactly*, not assumed (see consequences).

This equivalence has a lifetime. It holds precisely because the detector is
idealised air. When NaI realism arrives in Pass 6+ the detector becomes a real
attenuating body, the equivalence breaks, and a physically translating detector
must be modelled. **This ADR is scoped to Phase 1–2 and must be revisited with
the NaI model.**

## Options considered

**Rotate/translate the source–detector pair, phantom fixed.** The literal
hardware picture. Rejected: it requires moving two bodies (the gun, an action
object, and the detector, a geometry object), it steers the beam off +x and so
genuinely does break the unscattered gate, and it buys nothing physical over
moving the phantom.

**Widen the detector and pixelate it.** Attractive at first: with a wide static
detector, translation becomes a pure source-y offset needing no geometry change
at all, and the pixel index carries the translation coordinate for free.
Rejected for Phase 1 on two grounds. It changes the total-count regression
anchors, since a wider face accepts scatter over a wider solid angle; and
`[[Detector Model]]` locks one active pixel for Phase 1. Under the decision
actually taken, translation needs no geometry rebuild anyway, so the main
benefit evaporates. Revisit alongside NaI.

**Keep all commands pre-init (one process per measurement).** The conservative
option, consistent with Pass 3, and it makes stale geometry structurally
impossible. Rejected on cost: a 180 × 128 scan becomes ~23,000 process launches,
and the remaining schedule risk is already concentrated in Pass 5. It also has a
statistical cost — see consequences.

## Consequences

**Good.**

- A full scan is ~180 process launches (one per angle) instead of ~23,000, with
  the inner translation sweep running inside one process.
- One CSV file per angle instead of one per measurement — 180 files rather than
  23,040 — which retires the file-count blow-up warning in `[[Output Format]]`
  without moving to HDF5.
- The open-beam N₀ needs **one run, not a scan**. With `/cttwin/phantom none`
  there is no phantom, and the rig does not move, so nothing in the world
  changes with θ or t: every open-beam measurement is literally the same
  simulation.
- The Pass 1–3 anchors reproduced **exactly** — 0.99600 / 0.99580 empty world,
  0.47950 / 0.47060 pipe — which makes the generalised unscattered gate a
  *proven* no-op rather than an argued one.

**Costs and hazards.**

- **Stale geometry** is now possible in principle: a scan could transport
  photons through a world that no longer matches the reported (θ, t). This is
  the hazard `[[DetectorMessenger]]` warned about. It is mitigated by
  `GeometryHasBeenModified()` and *tested* by checkpoint 6, which requires a
  batched run to be bit-for-bit identical to the same measurement taken in its
  own process. It was, under MT.
- **Every phantom volume must be placed through `PlacePhantomVolume()`.** A
  volume placed directly with `G4PVPlacement` will not be registered, will not
  follow the scan, and will silently stay behind when the phantom rotates —
  producing a projection that looks plausible and is wrong. This is the single
  most likely way to break the geometry in a later pass.
- **Two Geant4 sign conventions coexist and look contradictory.**
  `WorldPositionOf()` uses +θ; `UpdateFrameRotation()` builds the placement
  matrix with `rotateZ(−θ)`. Both are correct: a `G4PVPlacement` rotation
  argument is the rotation of the *mother* frame with respect to the daughter,
  i.e. the inverse of the object's own rotation.
- **The θ = 0 case deliberately keeps a `nullptr` placement rotation** rather
  than an identity matrix, so the unrotated geometry stays on the exact
  navigator code path the Pass 1–3 anchors were measured on. Verified harmless:
  θ = 0 and θ = 360° agree to 1.03σ.
- **Seeds must be set per measurement in Pass 5.** Every `cttwin` process starts
  from the same default seed and runs within a process draw sequentially from
  one stream. Without an explicit reseed, the same noise realisation repeats at
  the same position in every angle's sweep — correlated noise down a sinogram
  column, which reconstructs as structure rather than grain.

## The limit on the sign convention

The relative sign of θ and t is measurable and was measured (checkpoint 4: four
configurations isolating a single bar each, poly/steel/steel/poly as predicted,
a factor 2.5 apart).

**A simultaneous flip of both signs is not measurable with the current phantom
set.** Pipe, bars and slab are all mirror-symmetric about the beam axis — for
the bars phantom, φ → −φ maps steel@0° → steel@0°, poly@60° → poly@300°,
steel@120° → steel@240°, poly@180° → poly@180° — so (θ, t) → (−θ, −t) is an
exact symmetry of the object and no measurement can distinguish it. Nine
independent samples of the mirrored pair agree (χ² = 4.3/7 dof after removing
one 2.6σ outlier).

The absolute convention is therefore fixed **by this ADR and
`[[Coordinate Conventions]]`, not by evidence.** It has no observable
consequence in Phase 1 or 2. It becomes observable in Phase 3 (laminography) and
Phase 4 (synthetic datasets), where asymmetric objects appear. Anyone adding an
asymmetric phantom should re-derive the convention against it as a first act.
