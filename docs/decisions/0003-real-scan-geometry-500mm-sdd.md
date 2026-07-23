# 0003 — Real scan geometry: symmetric 250/250 mm, 500 mm SDD

**Status:** Accepted, 2026-07-20. **Supersedes [ADR 0002](./0002-placeholder-scan-geometry.md)**
(symmetric 150/150 mm placeholder), which is now superseded and closed.

## Context

ADR 0002 adopted a symmetric 150/150 mm source–isocentre–detector placeholder,
carried from v1, with `kIsoToDetector` assumed symmetric on no hardware basis.
It carried a hard deadline: resolve before Pass 3, because Pass 3 quotes
Beer–Lambert validation numbers against this geometry and the NUTECH abstract
claims those numbers.

The open question ("what does the rig actually use?") was owned by Dr. Kairu and
asked on 2026-07-20. Two questions were put to him: the source-to-detector
distance of the NDT-lab gamma column scanner, and whether source and detector
are symmetric or offset about the object.

**His answer, 2026-07-20:** SDD is approximately **0.5 m**, source and detector
are **symmetric** about the object, and the geometry is **adjustable** if we want
to change it.

This arrived *before* Pass 3 began — the deadline in ADR 0002 was met, and the
constants are still just constants with no validated numbers layered on them.
This is the cheapest possible moment to change them.

## Decision

Adopt the hardware geometry exactly as reported, in `include/Constants.hh`,
`namespace CTTwin::Geometry`:

```cpp
constexpr G4double kSourceToIso      = 250.0 * mm;   // source -> rotation axis
constexpr G4double kIsoToDetector    = 250.0 * mm;   // rotation axis -> detector (symmetric, confirmed)
constexpr G4double kSourceToDetector = kSourceToIso + kIsoToDetector;  // 500 mm SDD
constexpr G4double kWorldHalfSize    = 500.0 * mm;   // unchanged — detector at +250 mm sits well inside
```

The symmetric split of the 500 mm SDD puts source and detector each 250 mm from
the isocentre, which is what "symmetric about the object" means.

## Options considered

**Option A — mirror the hardware exactly: 250/250 mm, symmetric (chosen).**

- The stated purpose of cttwin is a *digital twin* of the Kenya rig. A twin that
  doesn't match the rig's geometry is not a twin; the CRP contribution rests on
  correspondence with real hardware.
- Removes the last unfounded assumption from the geometry. ADR 0002's headline
  risk — "the symmetric assumption is not physical" — is retired by the
  hardware answer confirming symmetry.
- One change, one file, before any validated number depends on it.

**Option B — exploit "adjustable" and pick a convenient geometry (rejected).**

Dr. Kairu noted the rig can be adjusted however we like, which invites choosing
a distance that is tidier or that flatters the simulation (e.g. shorter air path
for a cleaner open-beam count).

- Rejected on principle: it would quietly undo the thing the placeholder was
  flagged for two passes running. The value of the answer is *correspondence
  with hardware*, and optimising away from it for convenience discards exactly
  that value while keeping the appearance of it.
- If a future experiment genuinely needs a different SDD (e.g. a
  magnification study), that is a deliberate, documented variation from the
  hardware baseline — a new ADR, not a silent default.

**Option C — keep 150/150 and note the discrepancy (rejected).**

- Cheaper today, but knowingly ships a geometry that contradicts the rig it
  claims to model, and the cost only rises: Pass 3 numbers, then Pass 4/5 scan
  motion, would all be built on a known-wrong figure.

## Consequences

**Good:**
- Geometry is hardware-derived; Lockdown #10 stops being a placeholder row.
- Pass 3 validation numbers will be quoted against real geometry from the
  outset — no re-run, no caveat in the paper's setup description.
- The last open question blocking the critical path is closed.

**Costs / risks:**
- **The stored regression anchors moved** — measured, not predicted: empty world
  0.9978 → **0.9960** (−0.18%), pipe 0.5012 → **0.4797** (−4.29%). The two did
  not move proportionally, and the asymmetry identifies the mechanism: the
  dominant loss is small-angle Compton scatter *from the steel* falling outside
  the detector's reduced solid angle at the longer distance, not air attenuation.
  This is beneficial for Pass 3 — less scatter contamination means the measured
  count is closer to pure primary transmission, which is what Beer–Lambert
  predicts. Full analysis in
  [validation/geometry-update-500mm-sdd.md](../validation/geometry-update-500mm-sdd.md).
  The Pass 1/2 validation docs keep their original numbers as a frozen record of
  the placeholder geometry and gain a dated note pointing here.
- "Approximately 0.5 m" is a round figure, not a measured one. If a precise
  survey later gives e.g. 495 mm, that is a small amendment to this ADR, not a
  new decision — and by then the constants are still the single source of truth.
- The detector remains well inside `kWorldHalfSize` (250 mm vs 500 mm
  half-extent), so no world resize is needed. Worth re-checking if SDD ever
  grows past ~900 mm.

## Related

- [decisions/0002-placeholder-scan-geometry.md](./0002-placeholder-scan-geometry.md) — superseded by this ADR
- [validation/geometry-update-500mm-sdd.md](../validation/geometry-update-500mm-sdd.md) — re-baselined anchors
- Vault: Architecture Lockdown #10 (amended 2026-07-20)
- `include/Constants.hh`