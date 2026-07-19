# 0002 — Placeholder scan geometry: symmetric 150/150 mm source–iso–detector

**Status:** Accepted as a placeholder, 2026-07-19. **Must be revisited before
Pass 3** — Pass 3 quotes validation numbers against this geometry.

## Context

The translate–rotate scan needs three distances: source to rotation axis
(isocentre), isocentre to detector, and their sum (source-to-detector, SDD).
Pass 1 places the physical detector volume against these numbers; Pass 3 quotes
Beer–Lambert results against them.

The real values depend on what the Kenya portable clampable CT rig actually
uses — an open question owned by Dr. Kairu that has not yet been answered. v1
placed its source at x = −150 mm from the world centre; there was no detector,
so no detector distance existed to inherit.

The project cannot stall waiting for the hardware number. It needs *a* geometry
to keep moving, clearly marked as provisional.

## Decision

Adopt a documented placeholder in `include/Constants.hh`, `namespace
CTTwin::Geometry`, as named `constexpr` values with a warning block:

```cpp
constexpr G4double kSourceToIso      = 150.0 * mm;   // carried from v1
constexpr G4double kIsoToDetector    = 150.0 * mm;   // ASSUMED symmetric
constexpr G4double kSourceToDetector = kSourceToIso + kIsoToDetector;  // 300 mm
constexpr G4double kWorldHalfSize    = 500.0 * mm;
```

`kSourceToIso` is carried from v1. `kIsoToDetector` is **assumed symmetric with
no hardware basis**. `kSourceToDetector` is derived.

## Options considered

**Option A — symmetric placeholder in named constants (chosen).**
Source and detector equidistant from the axis, values named and warning-blocked.

- Unblocks Pass 1 immediately; nothing waits on Dr. Kairu.
- Single source of truth — when the real number lands, one file changes.
- Symmetry is convenient and simple to reason about while validating transport.

**Option B — block Pass 1 until the real SDD is known (rejected).**
Wait for the hardware answer before placing the detector.

- Physically most correct, but stalls the critical path indefinitely on an
  external dependency, against a September deadline.

**Option C — bare literals at point of use (rejected).**
Hard-code 150/300 mm where needed.

- No single source of truth; the placeholder becomes invisible and un-findable,
  and updating it later means hunting scattered magic numbers. This is exactly
  the trap the named-constant block avoids.

## Consequences

**Good:**
- Pass 1 proceeds now.
- The assumption is visible in code (warning block) and in the vault
  (Architecture Lockdown #10), not buried.
- One-line change when the real number arrives.

**Costs / risks:**
- **The symmetric assumption is not physical.** Real portable clamp rigs are
  often source-close / detector-far; the clamp geometry constrains it.
- The assumption gets *more* expensive to change with each pass built on it:
  Pass 1 places the detector at it, Pass 3 quotes numbers against it. Changing
  it after Pass 3 invalidates the numbers in the NUTECH abstract claim.
- **Hard deadline for resolving this: before Pass 3.** The action is an email to
  Dr. Kairu asking the real SDD, sent while it is still just a constant and not
  a constant with three passes of numbers layered on top.

## Related

- Vault: Architecture Lockdown #10; Open Questions (source-to-detector distance)
- [features/pass-0-port.md](../features/pass-0-port.md)
- `include/Constants.hh`