# 0004 — Beer–Lambert reference value and acceptance definition

- **Status:** Accepted
- **Date:** 2026-08-13
- **Pass:** 3
- **Supersedes:** nothing. **Superseded by:** nothing.
- **Related:** [0001](./0001-physics-list-emstandard-option4.md) (physics list), [0003](./0003-real-scan-geometry-500mm-sdd.md) (scan geometry), [validation/pass-3-beer-lambert.md](../validation/pass-3-beer-lambert.md)

## Context

Pass 3 states that simulated transmission agrees with Beer–Lambert to within
2%. That sentence contains three quantities which were never pinned down, and
each of them can consume a large fraction of the 2% budget on its own:

1. **Which μ/ρ.** The project vault carried 0.07375 cm²/g for iron at 662 keV
   as a remembered number with no derivation. It is not reproducible from NIST
   SRD 126 Table 3 by either obvious interpolation: log-log interpolation of
   the 0.6 and 0.8 MeV grid points gives 0.07346, linear gives 0.07392. That
   0.65% spread becomes ~1.5% in predicted transmission at 40 mm — most of the
   budget, spent before any physics is tested. The material is also carbon
   steel, not iron.

2. **Which energy.** Pass 2 wrote 662 keV as a literal in
   `PrimaryGeneratorAction.cc`. The Cs-137 line is 661.657 keV. A rounded
   simulation energy paired with a coefficient evaluated at a different energy
   is precisely the "keep energy ↔ μ/ρ in sync" failure on the project's
   carry-forward list, and it fails looking like a physics bug.

3. **Which count.** The detector registers every gamma that arrives, including
   photons that Compton-scattered forward in the phantom and still landed on
   the face. exp(−μt) describes primary transmission only. This was left open
   by `validation/geometry-update-500mm-sdd.md`, which measured the scatter
   sensitivity but deferred the decision to this pass.

## Decision

**1. The reference coefficient is derived in code, not stored as a literal.**
`python/xcom_reference.py` computes it from NIST SRD 126 Table 3 elemental
data: log-log interpolation in energy (the interpolation the tables are built
to be read with; μ/ρ is near a power law in this Compton-dominated region with
no edges), mass-weighted over the actual `CarbonSteel` composition by Bragg
additivity.

| Quantity | Value |
| --- | --- |
| μ/ρ, Fe alone, 661.657 keV | 0.0734641 cm²/g |
| μ/ρ, 99% Fe + 1% C | **0.0735004 cm²/g** |
| μ (× 7.85 g/cm³) | **0.5769780 /cm** |
| Half-value layer | 12.0134 mm |

The compound correction is +0.049% — small, but computed rather than waived,
because quoting a steel measurement against an iron coefficient invites exactly
one reviewer question and there should be an answer.

**2. The source energy is 661.657 keV, defined once.**
`Physics::kCs137GammaEnergy` in `include/Constants.hh` is the sole definition;
`PrimaryGeneratorAction`, `SensitiveDetector` and `RunAction` all read it.
`xcom_reference.py` evaluates μ/ρ at the same value. Prose and publications may
continue to say "662 keV", which is standard.

**3. Acceptance is judged on the UNSCATTERED count.** A photon counts as
unscattered when it is a primary (`ParentID == 0`) arriving with both its
launch energy (within 1 eV) and its launch direction (within 1e-9 of the +x
axis). Both gates are required: Compton scattering changes energy and
direction, but **Rayleigh scattering is elastic** and would pass an
energy-only gate. The total count is reported alongside, and the difference
between them is published as the measured scatter contribution.

**4. N₀ is the measured open beam at matched statistics**, not the number of
primaries fired. About 0.47% of primaries are lost to the 500 mm air path; the
ratio cancels it. The residual term from a slab of thickness t displacing t of
air is applied explicitly (+0.034% at 40 mm).

## Options considered

**On the reference coefficient.** Keeping the vault's 0.07375 was rejected: it
is unattributable, and its provenance cannot be reconstructed for a reviewer.
Using pure iron was rejected as described above. Taking the value from the
XCOM web form by hand was rejected because a number typed into a file is not
reproducible and drifts silently if the composition changes; the derivation in
code re-derives itself.

**On the acceptance count.** Judging the total count was the status quo and was
rejected on measurement: contamination reaches 2.858% at 20 mm and 6.218% at
40 mm, so the criterion would have failed at two thicknesses for a reason
unconnected to transport accuracy. Applying a scatter *correction factor* to
the total was rejected as less honest — an exact discriminator exists in
simulation, so estimating what could be measured would be a choice to know
less. An energy-window count, which is what a real NaI photopeak delivers, is
a detector-response question and belongs with the NaI model in Pass 6+.

**On the direction gate.** An energy-only gate was considered and rejected
because it silently admits Rayleigh-scattered photons. A track-length
comparison (path travelled vs straight-line distance) was considered as an
exact alternative, but `G4Track::GetTrackLength()` has ambiguous semantics
about whether the current step is included at the moment `ProcessHits` runs;
the energy-plus-direction test has no such ambiguity.

## Consequences

**Good.**
- The validation table is reproducible end to end from source, with the
  reference derivation auditable in ~30 lines.
- The published result separates two claims that were previously conflated:
  attenuation is exponential in t (χ² = 1.33 — this validates cttwin), and the
  effective μ differs from NIST by −0.140% ± 0.073% (this quantifies a
  cross-section library difference, not a defect).
- Scatter contamination becomes a published measurement rather than an
  assumption, closing the open item from `geometry-update-500mm-sdd.md`.

**Costs.**
- Changing the source energy from 662.0 to 661.657 keV broke bit-exact
  reproduction of the Pass 1/2 anchors. In practice the empty-world anchor
  reproduced exactly and the pipe anchor moved 2 counts against a 1σ of 69, so
  regression checks against these anchors are now statistical rather than
  equality tests.
- The unscattered gate hard-codes the assumption of a single-energy on-axis
  source. Adding Co-60 (two lines), beam divergence, or source translation
  will each require the gate to be generalised. `SensitiveDetector.cc` carries
  a comment saying so.
- If Geant4's photon data library is ever changed, the −0.140% figure moves and
  this ADR's numbers need re-measuring, not just re-reading.

**Neutral.**
- The 2% criterion itself is unchanged and remains as inherited from the Phase
  1 plan and the submitted abstract. This ADR defines what it is applied to,
  not what it is.
