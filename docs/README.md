# cttwin — Documentation

This is the working knowledge base for the cttwin build — the Kenya
contribution to IAEA CRP F22086, a Geant4 first-generation gamma CT simulator.

The goal: anyone (including future-me) should be able to read this and
understand **what was built, why it was built that way, whether it was
physically verified, and how to recover if it breaks**.

Adapted from the R.A.N.G.E.R. V3 docs. Same discipline, mapped from *features*
to *passes*, with one addition — `validation/` — because a cttwin pass isn't
done when it compiles, it's done when its checkpoint number comes out right,
and those numbers are what the NUTECH 2026 paper claims.

## docs/ vs the vault — read this first

There are two knowledge stores. They are not duplicates.

- **The vault** (Obsidian, private) is *forward-looking scaffolding*: what to
  build next, the live locked-decisions list, current carry-forward lessons.
  It gets rewritten as reality changes.
- **`docs/`** (in-repo, git-tracked) is the *backward-looking record*: what was
  built and why, frozen at the moment it shipped. Entries are dated and are
  not rewritten — a Pass 1 retrospective stays true even after Pass 4 changes
  everything.

They hand off at the ADR. The vault's `Architecture Lockdown` is the *live
index* of decisions (current truth, rewritten when a decision changes). A
`decisions/NNNN-*.md` ADR is the *frozen entry* for one decision, with full
context — problem, options, consequences — as it stood when made.

---

## Structure

### `features/`
One pair of files per pass, documenting the pass end to end.

- `pass-N-<name>.md` — what the pass built: files created/modified, geometry,
  physics, data flow, the checkpoint, gotchas.
- `pass-N-<name>-retrospective.md` — what worked, what was hard, what to do
  differently, numbers. Records the *surprise*, not a recap. A clean pass earns
  a short retrospective; padding it is worse than brevity.

### `validation/`
Physics verification records — the cttwin-specific addition. One file per
measured checkpoint: what was tested, the method, the expected value, the
actual number. This is the evidence base the September paper cites directly.

- `pass-N-<test>.md` — e.g. `pass-3-beer-lambert.md`.

### `decisions/`
Architecture Decision Records (ADRs). Short, dated, numbered. Format:
context → decision → options considered → consequences. The frozen companion
to the vault's live Architecture Lockdown.

### `troubleshooting/`
Error logs and fixes. Each entry: symptom → diagnosis → fix → lesson. Written
when an error costs real time, so a fresh chat (or fresh machine) doesn't
rediscover it.

### `architecture/`
Higher-level design docs — the *why* behind structural choices: the three-tier
decoupled architecture, translate–rotate geometry, the idealised detector.

- `00-overview.md` — three-tier system overview.

---

## Index

### Passes
- [`pass-0-port.md`](./features/pass-0-port.md) — port v1 phantom geometry into
  the CTTwin namespace, set the EM physics list, one phantom at the origin
- [`pass-0-port-retrospective.md`](./features/pass-0-port-retrospective.md) —
  the duplicate-file mess, the QBBC catch, carry-forwards
- [`pass-1-sensitive-detector.md`](./features/pass-1-sensitive-detector.md) —
  the idealised photon counter, detector volume, and the SD→EventAction→RunAction count chain
- [`pass-1-sensitive-detector-retrospective.md`](./features/pass-1-sensitive-detector-retrospective.md) —
  the exactly-half diagnostic, pull-over-push, the empty-world config wrinkle
- [`pass-2-pencil-beam.md`](./features/pass-2-pencil-beam.md) —
  the collimated zero-width pencil beam replacing the placeholder, aimed at the in-line pixel
- [`pass-2-pencil-beam-retrospective.md`](./features/pass-2-pencil-beam-retrospective.md) —
  the almost-no-code surprise, deciding idealisations before GO, trusting a small diff
- [`pass-3-cs137-beer-lambert.md`](./features/pass-3-cs137-beer-lambert.md) —
  Cs-137 formalised at 661.657 keV, the flat slab phantom, the minimal messenger
  pulled forward from Pass 4, and unscattered-primary counting
- [`pass-3-cs137-beer-lambert-retrospective.md`](./features/pass-3-cs137-beer-lambert-retrospective.md) —
  the error budget that nearly lost the pass, the known-answer checkpoint, why
  fitting mu beat checking four endpoints, the nested-sample artefact
- [`pass-4-scan-motion.md`](./features/pass-4-scan-motion.md) —
  the scan transform carried by the phantom, runtime geometry motion, the full
  `/cttwin/scan/` + `/cttwin/output/` messenger, and the per-projection CSV
- [`pass-4-scan-motion-retrospective.md`](./features/pass-4-scan-motion-retrospective.md) —
  the landmine that dissolved instead of being fixed, the theta = 360 code-path
  discriminator, two ~2.8 sigma flags chased to ground, the seed scar made visible
- [`pass-5-python-tier.md`](./features/pass-5-python-tier.md) —
  the scan driver, the assembled sinogram, the reconstruction, and the analytic
  forward model that made the checkpoint a number; the two scan grids; why the
  baseplate is never illuminated
- [`pass-5-python-tier-retrospective.md`](./features/pass-5-python-tier-retrospective.md) —
  verifying the sinogram instead of the reconstruction, a redundancy test that
  tested nothing, an ROI that measured the wrong material, and three
  explanations of one bias with two of them wrong

### Validation
- [`pass-0-port.md`](./validation/pass-0-port.md) — build + run + overlap
  verification for the ported skeleton
- [`pass-1-detector.md`](./validation/pass-1-detector.md) — 10k photons empty
  world → 0.9978 detected; pipe cross-check 0.5012
- [`pass-2-pencil-beam.md`](./validation/pass-2-pencil-beam.md) — pencil beam
  samples a line integral: empty world 0.9978 (≡ Pass 1), pipe 0.5012 drop
- [`geometry-update-500mm-sdd.md`](./validation/geometry-update-500mm-sdd.md) —
  anchors re-baselined after the SDD change; **current** regression figures
- [`pass-3-beer-lambert.md`](./validation/pass-3-beer-lambert.md) —
  **the headline Phase 1 result.** 2% agreement met at 5/10/20/40 mm (worst
  +0.746%); fitted mu = 0.5761682 /cm, reduced chi2 = 1.33; measured scatter
  contamination; photons-per-projection budget. The table the NUTECH 2026 paper
  cites, and the basis of the TRL 3 claim
- [`pass-4-scan-motion.md`](./validation/pass-4-scan-motion.md) —
  the seven-rung checkpoint ladder: anchors reproduced **exactly**, pipe
  rotational invariance, the factor-4 Option B rotation, parallel-beam
  redundancy at 0.26 sigma, the sign table, bit-for-bit proof against stale
  geometry, and the CSV contract. Six hand-computed chords agreeing to 0.2-1.5%
- [`pass-5-reconstruction.md`](./validation/pass-5-reconstruction.md) —
  **the Phase 1 closing result.** Seven checkpoints across two phantoms and two
  sampling grids: anchors exact at threads = 1, sign convention determined at
  161x separation, redundancy at 0.75 vs 209.8 sigma, RMS pull 1.04 and 1.06
  against a parameter-free forward model, and reconstructed mu within **1%** of
  NIST for steel, aluminium and polyethylene. The basis of NUTECH abstract
  claim 2, and the FBP-vs-iterative finding

### Decisions
- [`0001-physics-list-emstandard-option4.md`](./decisions/0001-physics-list-emstandard-option4.md)
  — why `G4EmStandardPhysics_option4` replaces v1's `QBBC`
- [`0002-placeholder-scan-geometry.md`](./decisions/0002-placeholder-scan-geometry.md)
  — the symmetric 150/150 mm source–iso–detector placeholder, and what it blocks
  (**superseded by 0003**)
- [`0003-real-scan-geometry-500mm-sdd.md`](./decisions/0003-real-scan-geometry-500mm-sdd.md)
  — confirmed hardware geometry: symmetric 250/250 mm, 500 mm SDD; supersedes 0002
- [`0004-beer-lambert-reference-and-acceptance.md`](./decisions/0004-beer-lambert-reference-and-acceptance.md)
  — the reference mu/rho derived from NIST rather than remembered, the source
  energy fixed at 661.657 keV in one place, and the 2% criterion applied to the
  unscattered count against a measured open-beam N0
- [`0005-phantom-carries-the-scan-transform.md`](./decisions/0005-phantom-carries-the-scan-transform.md)
  — the phantom carries the scan transform and the rig never moves; scan motion
  is legal after `/run/initialize`; the detector stays one pixel; and why the
  equivalence is exact rather than approximate (and when it expires)
- [`0006-scan-grids-seeding-and-thread-pinning.md`](./decisions/0006-scan-grids-seeding-and-thread-pinning.md)
  — the two matched scan grids and why N_t is odd, 1e5 per measurement against
  1e7 for the common-mode open beam, per-measurement seeds recorded as a
  formula, and threads pinned to remove an unmeasured variable from the anchors

### Troubleshooting
- [`001-multiple-definition-of-main.md`](./troubleshooting/001-multiple-definition-of-main.md)
  — duplicate `cttwin.cc` in root and `src/` → linker error, and the CMake
  glob re-configure gotcha behind it

- [`002-delivered-files-land-at-a-flattened-path.md`](./troubleshooting/002-delivered-files-land-at-a-flattened-path.md)
  — delivered files arriving at a flattened path rather than not arriving; the
  revised check (verify the path before the run, not the file after the
  failure), and stale IntelliSense masquerading as real errors

- [`003-template-substituted-its-own-documentation.md`](./troubleshooting/003-template-substituted-its-own-documentation.md)
  — a macro template whose header comment explained its own placeholders, so a
  whole-file substitution injected the entire scan block into a comment and ran
  it above `/run/initialize`; and why a rejected Geant4 command is not a failed
  run