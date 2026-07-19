# 0001 — Physics list = G4EmStandardPhysics_option4, replacing v1's QBBC

**Status:** Accepted, 2026-07-19. Revisit only if a physics regime beyond
photon transport in 50 keV–1.5 MeV is introduced.

## Context

The simulator validates gamma transmission through carbon steel via the
Beer–Lambert law (Pass 3 checkpoint: 2% agreement at 5/10/20/40 mm, Cs-137
662 keV). The physics list governs which processes and cross-sections model
that transmission. If it is wrong, Beer–Lambert can fail — or worse, *appear*
to pass while being subtly off — for reasons unrelated to the geometry or
materials under test.

The v1 model (`crpf22086-phantom/phantom_design.cc`) set the physics list
explicitly to `QBBC`. The original Pass 0 handoff assumed the list was an
inherited `FTFP_BERT` default; reading v1's main showed it was a deliberate
`QBBC`. Either way, both are hadronic/high-energy-oriented lists carrying
physics irrelevant to a 662 keV photon problem, and neither is the standard
choice for precise low-to-mid-energy electromagnetic transport.

## Decision

Use `G4EmStandardPhysics_option4`, set explicitly in `cttwin.cc` through a
minimal `G4VModularPhysicsList` subclass:

```cpp
class CTTwinPhysicsList : public G4VModularPhysicsList {
 public:
  CTTwinPhysicsList() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
```

## Options considered

**Option A — `G4EmStandardPhysics_option4` (chosen).**
The most accurate of the standard EM option sets, recommended for applications
needing high-precision electromagnetic physics in the keV–MeV range. It is the
right tool for photon attenuation and dosimetry-grade transport.

- Correct process set for photoelectric / Compton / pair-production at 662 keV.
- "Standard for photon transport in 50 keV–1.5 MeV range" — matches the vault's
  Physics List Choice note and Architecture Lockdown #8.
- Slightly heavier than base `option0`, but the run sizes here are small and
  accuracy dominates.

**Option B — keep `QBBC` (rejected).**
The v1 inheritance. A hadronic/space-radiation list.

- Carries hadronic and high-energy physics irrelevant to this problem.
- Not tuned for precise low-energy EM transport; risks a silent few-percent
  skew in transmission that would surface as a Beer–Lambert failure Pass 3
  would waste time hunting.

**Option C — base `G4EmStandardPhysics` / `option0` (rejected).**
Adequate for many EM applications, lighter than option4.

- Fine for coarse work, but when the whole deliverable is a 2%-agreement
  validation claim, the most accurate standard EM list is the defensible
  choice to quote in a paper.

## Consequences

**Good:**
- The physics list is set explicitly and named in source — no silent inheritance.
- Correct EM process set for the Beer–Lambert validation the project stands on.
- Verified active at runtime (the "Electromagnetic Physics Parameters" block
  prints), and QBBC is provably gone (zero grep hits in the tree).

**Costs / notes:**
- `G4EmStandardPhysics_option4` does not print its own class name in the init
  dump, so runtime confirmation is via the EM-parameters block + source grep,
  not a log string match. Documented so no one re-hunts this.
- If the project ever models something outside photon EM transport, this
  decision must be revisited rather than extended blindly.

## Related

- Vault: Architecture Lockdown #8, #11; Physics List Choice note
- [features/pass-0-port.md](../features/pass-0-port.md)
- [validation/pass-0-port.md](../validation/pass-0-port.md)