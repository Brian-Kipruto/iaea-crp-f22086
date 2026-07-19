# Pass 0 — Port v1 phantom geometry to CTTwin

> Status: Complete 2026-07-19. Commit `a3a8e2f` on `main`. Geant4 11.2.1.

## What it does

Pass 0 turns two things into one working Geant4 application:

1. the **v1 phantom model** (a separate repo, `crpf22086-phantom`) — which had
   correct geometry but was not a CT simulator; and
2. the **cttwin skeleton** (`iaea-crp-f22086`) — which had the right project
   scaffold but a placeholder `main` and no physics.

The result builds, runs, fires events, and passes an overlap check with a
single phantom centred on the rotation axis. It is *still not* a CT simulator —
there is no detector, no scoring, no scan — that begins in Pass 1. Pass 0 is
the foundation everything downstream is built on.

The original handoff scoped Pass 0 as a namespace refactor (`B1` → `CTTwin`).
Inspection showed the reality was different: the two repos were out of sync, the
v1 code lived elsewhere, and there was no `B1` to rename in the target. Pass 0
became a **port**, not a refactor.

## What was carried, what was discarded

The v1 model was a geometry proof, not a simulator skeleton. Only the geometry
was worth keeping.

| v1 element | Disposition |
| --- | --- |
| Materials (NIST air/Al/poly + custom CarbonSteel 7.85 g/cm³, Fe 99% / C 1%) | **Kept verbatim** — the material the validation numbers will be quoted against |
| Phantom solids (pipe; baseplate + central bar + hex ring) | **Kept**, recentred |
| Phantom *placement* (both phantoms side by side at x = ±160 mm) | **Rewritten** — one active phantom at the origin |
| `fScoringVolume = logicPipe` | **Deleted** — scored dose *in the object*; wrong quantity, wrong place |
| Ir-192 three-line spectrum + 30° cone (`PrimaryGeneratorAction`) | **Discarded** — placeholder mono-directional 662 keV gamma instead |
| Dose chain (`RunAction`/`EventAction`/`SteppingAction`, `G4Accumulable<G4double> fEdep`) | **Discarded** — rewritten as count stubs; `SteppingAction` not ported at all |
| `QBBC` physics list (`phantom_design.cc`) | **Replaced** with `G4EmStandardPhysics_option4` |
| `G4ScoringManager` (enabled but unused in v1 main) | **Dropped** |
| v1 `build/` directory (committed binaries + `.o`) | **Not carried** — `.gitignore` catches it |

## Files created / modified

New in `iaea-crp-f22086`:

```
include/Constants.hh                 placeholder geometry constants (single source of truth)
include/DetectorConstruction.hh      namespaced; SetActivePhantom() stub for Pass 4
include/PrimaryGeneratorAction.hh    gamma-gun shell
include/ActionInitialization.hh      namespaced
include/RunAction.hh                 count stub (was dose)
include/EventAction.hh               count stub (header-only)
src/DetectorConstruction.cc          geometry salvaged, one phantom at origin
src/PrimaryGeneratorAction.cc        placeholder beam, Ir-192 + cone removed
src/ActionInitialization.cc
src/RunAction.cc                     prints run summary
```

Modified: `cttwin.cc` (placeholder `main` → real: `G4RunManagerFactory`,
explicit `CTTwinPhysicsList` wrapping `G4EmStandardPhysics_option4`,
DetectorConstruction, ActionInitialization).

`EventAction` is header-only (inline empty bodies) — correct for Pass 0, so no
`src/EventAction.cc`.

## Geometry

One active phantom, selected by `fActivePhantom` (default `"pipe"`), centred on
the origin — which is the rotation axis for the translate–rotate scan in Pass 4.
v1 placed *both* phantoms off-axis at ±160 mm, which would have rotated the
wrong thing. Option B (bars) keeps the deliberate 0.01 mm z-gap between bars and
baseplate (the navigator needs distinct boundaries).

Source-to-detector geometry is a documented **placeholder** — see
[decision 0002](../decisions/0002-placeholder-scan-geometry.md).

## Physics

`G4EmStandardPhysics_option4`, set explicitly in `cttwin.cc` via a small
`G4VModularPhysicsList` subclass. This replaces v1's `QBBC`. See
[decision 0001](../decisions/0001-physics-list-emstandard-option4.md) for the
full reasoning — in short, QBBC is a hadronic/space-radiation list and would
have silently skewed Beer–Lambert in Pass 3.

## Checkpoint

Build clean · one phantom at origin, overlap-clean · 10 events run · EM physics
active · zero `B1` · zero `QBBC`. All met — see
[validation/pass-0-port.md](../validation/pass-0-port.md) for the numbers.

## Gotchas encountered

- **Duplicate `cttwin.cc`** in root and `src/` → `multiple definition of main`.
  See [troubleshooting 001](../troubleshooting/001-multiple-definition-of-main.md).
- **CMake globbing** doesn't see added/removed files until `cmake ..` re-runs.
- **`macros/single_projection.mac` is a Pass-4 stub** — all comments, no
  `/run/beamOn`. Firing events pre-messenger needs a throwaway macro.
- **The physics list class name doesn't print** in the init dump — confirm via
  source + the "Electromagnetic Physics Parameters" block, not by grepping the
  log for "option4".