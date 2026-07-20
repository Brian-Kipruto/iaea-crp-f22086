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

### Validation
- [`pass-0-port.md`](./validation/pass-0-port.md) — build + run + overlap
  verification for the ported skeleton
- [`pass-1-detector.md`](./validation/pass-1-detector.md) — 10k photons empty
  world → 0.9978 detected; pipe cross-check 0.5012
- [`pass-2-pencil-beam.md`](./validation/pass-2-pencil-beam.md) — pencil beam
  samples a line integral: empty world 0.9978 (≡ Pass 1), pipe 0.5012 drop

### Decisions
- [`0001-physics-list-emstandard-option4.md`](./decisions/0001-physics-list-emstandard-option4.md)
  — why `G4EmStandardPhysics_option4` replaces v1's `QBBC`
- [`0002-placeholder-scan-geometry.md`](./decisions/0002-placeholder-scan-geometry.md)
  — the symmetric 150/150 mm source–iso–detector placeholder, and what it blocks

### Troubleshooting
- [`001-multiple-definition-of-main.md`](./troubleshooting/001-multiple-definition-of-main.md)
  — duplicate `cttwin.cc` in root and `src/` → linker error, and the CMake
  glob re-configure gotcha behind it