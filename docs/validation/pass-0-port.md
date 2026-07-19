# Validation — Pass 0 (port)

> Date: 2026-07-19. Geant4 11.2.1 (MT). Commit `a3a8e2f`.

Pass 0 has no physics number to validate (no detector yet). Its checkpoint is
structural: the ported skeleton builds, runs, fires events, and is geometrically
sound. Recorded here so the trail starts with the same discipline the later
physics passes will need.

## Checkpoints and results

| # | Check | Method | Expected | Actual |
| --- | --- | --- | --- | --- |
| build | Compiles + links clean | `rm -rf build && cmake .. && make -j` | `Built target cttwin` | ✅ built, no errors |
| 0b | One phantom at origin, no overlaps | overlap check during `/run/beamOn` | `... OK!` | ✅ `Checking overlaps for volume PhysPipe:0 (G4Tubs) ... OK!` |
| 0c | Events fire, action chain runs | `printf '/run/initialize\n/run/beamOn 10\n' > /tmp/p0.mac; ./cttwin /tmp/p0.mac` | run summary, 10 events | ✅ `--- CTTwin run summary: 10 events processed. ---` |
| 0d | EM physics active, QBBC gone | init dump + tree grep | EM params block present, 0 `QBBC` | ✅ "Electromagnetic Physics Parameters" printed; `grep -rn QBBC` → only the removal comment |
| 0e | Namespace clean | `grep -rn "\bB1\b"` / `grep -rn "namespace CTTwin"` | 0 `B1`, all units `CTTwin` | ✅ 0 `B1`; `CTTwin` in every unit |

## Method notes

- `single_projection.mac` could not be used to fire events — it is a Pass-4 stub
  (all comments, no `/run/beamOn`). A throwaway macro was used instead.
- `G4EmStandardPhysics_option4` does **not** print its class name in the init
  dump. Runtime confirmation is the "Electromagnetic Physics Parameters" block
  plus the source registration (`RegisterPhysics(new G4EmStandardPhysics_option4())`
  in `cttwin.cc`), not a log grep for "option4". See decision 0001.
- The overlap check runs automatically because placements pass
  `checkOverlaps = true` (the trailing `true` arg on `G4PVPlacement`).

## Verdict

Pass 0 checkpoint met on all counts. Foundation is sound for Pass 1
(SensitiveDetector + detector volume).

## Related

- [features/pass-0-port.md](../features/pass-0-port.md)
- [decisions/0001-physics-list-emstandard-option4.md](../decisions/0001-physics-list-emstandard-option4.md)