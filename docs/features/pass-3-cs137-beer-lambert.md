# Pass 3 — Cs-137 formalised, flat slab, Beer–Lambert validation

> Date: 2026-08-13. Commit `16c8006`. Parent: `cd3b868`.
> Checkpoint: **2% Beer–Lambert agreement at 5/10/20/40 mm — met, worst case +0.746%.**
> Numbers: [validation/pass-3-beer-lambert.md](../validation/pass-3-beer-lambert.md).
> Decisions: [ADR 0004](../decisions/0004-beer-lambert-reference-and-acceptance.md).

The pass that turns a simulator into a *validated* simulator. Everything
downstream — sinograms, reconstruction, synthetic training data — assumes the
transport physics is right. This is where that assumption stops being an
assumption.

## What was built

**Cs-137 formalised.** The 662 keV literal in `PrimaryGeneratorAction.cc`
became `Physics::kCs137GammaEnergy = 661.657 keV` in `Constants.hh`, the sole
definition in the codebase, read by the gun, the detector's unscattered gate
and the run summary. The exact line energy is used rather than the rounded one
so the simulation energy and the reference μ/ρ are evaluated at the same point.

**Flat slab phantom.** `"slab"` joins `pipe` / `bars` / `none`. A `G4Box` of
carbon steel centred at the origin, thickness along the beam axis, 100 × 100 mm
laterally so no ray reaching the 50.8 mm detector face can have travelled
around an edge. Thickness is the only thing that varies; the lateral size is
fixed deliberately so that changing t changes exactly one thing.

The pipe wall was rejected for this test: the chord through a curved wall
depends on where the ray enters, which smears the exponential. The slab gives
one exactly-known path length.

**Minimal messenger, pulled forward from Pass 4.** `/cttwin/phantom` and
`/cttwin/slabThickness`, state-setting only, both required before
`/run/initialize`. A command issued after the geometry is closed is **rejected
with a loud error**, not accepted and ignored — silently running the wrong
thickness would produce a completely plausible-looking wrong validation table.

**Dual counting in the SensitiveDetector.** Alongside the Pass 1 arrival count,
a second count of unscattered primaries: `ParentID == 0`, arriving with launch
energy (within 1 eV) *and* launch direction (within 1e-9 of +x). Both gates are
needed because Rayleigh scattering is elastic and would pass an energy-only
test. `GetCount()` is untouched, so the Pass 1/2 anchors still mean what they
meant.

**Machine-readable output.** `RunAction` emits a single `CTTWIN_RESULT` line of
stable key=value fields, plus a human block that states which phantom and
thickness were actually built.

**Two Python deliverables.** `xcom_reference.py` derives the reference
coefficient from NIST SRD 126 Table 3 rather than storing a literal.
`validate_beer_lambert.py` drives all five configurations, applies the
air-path correction, and reports both per-thickness deviation and a fitted μ.

## Files

| Path | New/Mod | |
| --- | --- | --- |
| `python/xcom_reference.py` | New | reference derivation |
| `python/validate_beer_lambert.py` | Rewritten | was a `NotImplementedError` stub |
| `include/Constants.hh` | Mod | Cs-137 energy, slab, unscattered tolerances |
| `include/DetectorMessenger.hh` | New | |
| `src/DetectorMessenger.cc` | New | |
| `include/DetectorConstruction.hh` | Mod | slab, getters, messenger, real destructor |
| `src/DetectorConstruction.cc` | Mod | `"slab"` branch, `BuildSlabPhantom()`, SD lookup guard |
| `include/SensitiveDetector.hh` / `src/SensitiveDetector.cc` | Mod | unscattered counter |
| `include/RunAction.hh` / `src/RunAction.cc` | Mod | second accumulable, `CTTWIN_RESULT` |
| `include/EventAction.hh` / `src/EventAction.cc` | Mod | one extra pull |
| `src/PrimaryGeneratorAction.cc` | Mod | energy from `Constants.hh` |

`DetectorMessenger.cc` is a new `.cc`, so the CMake glob was stale and a full
`rm -rf build` reconfigure was mandatory.

## The three things that decided whether this pass could pass

Ordered by how much of the 2% budget each would have consumed:

**1. Photon statistics.** Every pass through Pass 2 used 10 000 primaries. At
40 mm that gives 2.94% statistical spread — the test cannot demonstrate 2%
agreement even with perfect physics, and a run that happened to land inside 2%
would have been unreproducible luck. 10⁶ brings it to 0.30%. This was a
precondition, not a refinement.

**2. The reference coefficient.** The inherited 0.07375 cm²/g is not
reproducible from the NIST table by either obvious interpolation (log-log
0.07346, linear 0.07392). That 0.65% spread is ~1.5% of transmission at 40 mm.
See ADR 0004.

**3. Which count is compared.** Scatter contamination reaches 2.858% at 20 mm
and 6.218% at 40 mm. Judging the total count against exp(−μt) would have failed
two thicknesses for reasons unrelated to transport accuracy.

## Result

All four thicknesses pass; worst case +0.746% at 40 mm. The fit of −ln(N/N₀)
against t gives μ = 0.5761682 ± 0.0004231 /cm with reduced χ² = 1.33, against
the NIST value of 0.5769780 /cm — a difference of −0.140% ± 0.073%.

The low χ² is what carries the physics. It says attenuation is exponential in
thickness, which is the transport validation and is internal to the simulation.
The slope offset is then a difference between Geant4's photon cross-section
library and the NIST tabulation, which is a finding to report rather than a bug
to hunt. Separating those two was the reason the fit exists at all.

## Gotchas for anyone extending this

- **The unscattered gate assumes a single-energy, on-axis source.** Co-60 (two
  lines), beam divergence, or source translation each break it. The comment in
  `SensitiveDetector.cc` says so; this is the first thing to revisit in Pass 4
  when the source starts moving.
- **Messenger commands must precede `/run/initialize`.** The rejection path is
  tested (checkpoint 3.5) and loud, but the constraint is real: geometry is
  built once.
- **Every `cttwin` process starts from the same default seed.** Runs are
  bit-reproducible, which is why the anchors reproduce exactly — but repeated
  runs at different N are *nested samples*, not independent ones. See §8 of the
  validation doc.
- **`CTTWIN_RESULT` is a parsed contract.** Reformatting it breaks
  `validate_beer_lambert.py`.
- **Changing the composition of `CarbonSteel` changes the reference μ/ρ.**
  `xcom_reference.py` re-derives it, but the ADR's quoted numbers would need
  re-measuring.
