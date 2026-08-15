# Pass 4 — Scan motion, the full messenger, and per-projection output

- **Date:** 2026-08-15
- **Commit:** `e240918` (cleanup `dbd7547`)
- **Checkpoint:** ✅ seven rungs, all passed —
  [validation/pass-4-scan-motion.md](../validation/pass-4-scan-motion.md)
- **Decisions:** [ADR 0005](../decisions/0005-phantom-carries-the-scan-transform.md)

## What this pass built

Pass 3 left `cttwin` able to compute one attenuation measurement per process,
with a two-command messenger pulled forward from this pass. Pass 4 turns it into
something a driver can loop over: the scan coordinates (θ, t) are settable at
runtime, they may change *between runs inside one process*, and each `beamOn`
appends a row to a CSV. This is the last piece before the Python tier.

The pass split into five sub-steps, verified in two deliveries: the transform,
runtime motion, the messenger and the unscattered gate first (checkpoints 1–6),
then output (checkpoint 7).

## The central decision

**The phantom carries the scan transform. The source and detector never move.**

For a phantom point `p` in the phantom's own frame:

```
p_world = R_z(θ) · p  −  t · ŷ
```

— the canonical rotation, with the whole system rigidly shifted by −t so the rig
can stay at y = 0 rather than translating to y = +t.

The justification is that the detector is `G4_AIR` inside a `G4_AIR` world and
therefore invisible to transport, which makes "a small detector that translates"
and "a detector that sits still while the phantom slides past" the *same
transport problem* rather than merely similar ones. Full reasoning, options
rejected, and the lifetime of that equivalence are in ADR 0005.

Three consequences shaped everything else in the pass: the beam stays on +x
forever, so the unscattered gate was never actually going to break; the detector
stays a single 50.8 mm pixel with the ray always on its centre; and the Pass 1–3
anchors are untouched at θ = 0, t = 0.

## Files

**Modified.** No new `.cc` files, so no CMake re-configure was needed.

| File | Change |
|---|---|
| `include/Constants.hh` | scan defaults, `kMaxScanTranslation` guard, the transform note |
| `include/DetectorConstruction.hh` | scan state, placement registry, output config |
| `src/DetectorConstruction.cc` | `PlacePhantomVolume`, `WorldPositionOf`, `ApplyScanTransform`, `UpdateFrameRotation`, `RotationForPlacement` |
| `include/DetectorMessenger.hh` / `src/DetectorMessenger.cc` | `/cttwin/scan/…`, `/cttwin/output/…`, split of the pre-init rule |
| `src/SensitiveDetector.cc` | unscattered gate generalised off the +x axis |
| `include/RunAction.hh` / `src/RunAction.cc` | scan echo, two new `CTTWIN_RESULT` fields, `WriteProjectionRow` |
| `macros/single_projection.mac` | stub filled in |
| `macros/full_scan_template.mac` | stub filled in as a per-angle template |

**New.** `macros/checkpoints/c1,c2-c4,c3,c5×2,c6×3,c7` — nine macros, each
carrying its own expected numbers in its header.

## The command vocabulary

```
GEOMETRY REBUILD — pre-init only, rejected loudly afterwards
  /cttwin/phantom          pipe | bars | slab | none
  /cttwin/slabThickness    <value> <unit>

SCAN MOTION — legal at any time, including between runs
  /cttwin/scan/angle        <value> <unit>
  /cttwin/scan/translation  <value> <unit>

OUTPUT — legal at any time
  /cttwin/output/file          <path>
  /cttwin/output/projectionId  <int>
```

The split is the point of the class. The first pair changes which *solids*
exist and can only be read when the geometry is built, so issuing one after
`/run/initialize` is an error worth shouting about. The second and third only
mutate objects that already exist.

Named `/cttwin/scan/…` rather than the vault's planned `/cttwin/source setAngle`
because under ADR 0005 the source is the one thing that does *not* move; the
command names the scan coordinate, which is what the driver cares about.

## How the motion works

Every phantom sub-volume is placed through `PlacePhantomVolume()`, which records
its position in the phantom's own frame in `fPhantomPlacements` and applies the
transform on the way out. On a scan-coordinate change, `ApplyScanTransform()`
recomputes the transform, pushes it onto the existing physical volumes with
`SetTranslation` / `SetRotation`, and calls
`G4RunManager::GeometryHasBeenModified()`. Nothing is rebuilt.

> **Any phantom volume placed directly with `G4PVPlacement` will not be
> registered, will not follow the scan, and will silently stay behind when the
> phantom rotates.** This is the most likely way to break the geometry in a
> later pass, and the failure produces a plausible-looking wrong projection.

Two details that look like bugs and are not:

- **`WorldPositionOf()` uses +θ while `UpdateFrameRotation()` builds the matrix
  with `rotateZ(−θ)`.** A `G4PVPlacement` rotation argument is the rotation of
  the *mother* frame with respect to the daughter — the inverse of the object's
  own rotation. Both expressions describe the same rotation in Geant4's two
  conventions.
- **At θ = 0 the placement rotation is `nullptr`, not an identity matrix**
  (`RotationForPlacement()`). An identity-rotation placement is physically the
  same but reaches the navigator by a different code path, and the boundary
  arithmetic need not round identically. The unrotated case keeps the exact path
  the Pass 1–3 anchors were measured on, which is what made the *exact* anchor
  reproduction in checkpoint 5 a meaningful test rather than a coincidence.

## The unscattered gate

Pass 3 tested the arriving direction against a hard-coded +x and the energy
against `Physics::kCs137GammaEnergy`. Both are now read from the event's own
primary vertex, so they cannot drift out of sync with `PrimaryGeneratorAction` —
there is no second definition of the beam to keep updated.
`1 − dir.x()` became `1 − dir · launchDir`, which reduces to exactly the old
expression while `launchDir` is `(1,0,0)`.

Under ADR 0005 the beam does not move, so this is a **provable no-op today** —
which is the point. Its correctness is testable now, by the anchors, rather than
in Pass 6 when the source finally does move.

## Output

`RunAction::WriteProjectionRow`, master thread only, one row per `beamOn`:

```
projection_id,angle_deg,translation_mm,pixel_index,n_counts,n_unscattered,n_events
```

`[[Output Format]]`'s columns with two appended. `n_unscattered` because there
are two counts now and the scatter-free sinogram is a paper figure that cannot
be reconstructed later; `n_events` because line integrals are −ln(N/N₀) and the
normalisation has to travel with the measurement.

The header is written **only if the file is new or empty**, so successive runs
accumulate into one file per angle — 180 files for a full scan rather than
23,040, which retires the file-count warning in `[[Output Format]]` without
moving to HDF5. An unset path means nothing is written, which is the default, so
every Pass 1–4 macro and `validate_beer_lambert.py` are unaffected. A failed
open is loud, because a scan that silently writes nothing looks exactly like a
scan that worked until the sinogram comes out empty hours later.

`pixel_index` is always 0 in Phase 1. **Note for Pass 5:** `[[Output Format]]`
describes the sinogram as (n_angles, n_pixels) — for a true first-generation
scanner that second axis is the *translation* axis. The column stays so the
format survives a multi-pixel detector in Pass 6+.

`CTTWIN_RESULT` gained `angle_deg=` and `translation_mm=`. Fields were
**appended**, never renamed — `validate_beer_lambert.py` builds a key→value dict
and looks fields up by name, so additions are transparent to it. Precision on
that line was raised to 9 significant figures so scan coordinates survive the
round trip; this changes `energy_keV=661.7` to `661.657`, a field nothing
parses.

## Gotchas for the next pass

- **Seeds are not optional in Pass 5.** Every `cttwin` process starts from the
  same default seed, and runs within a process draw sequentially from one
  stream. Without a reseed per measurement, the same noise realisation repeats
  at the same position in every angle's sweep — correlated noise down a sinogram
  column, which reconstructs as structure rather than grain.
  `full_scan_template.mac` generates `/random/setSeeds` per measurement.
- **The open beam needs one run, not a scan.** With `/cttwin/phantom none`
  nothing in the world changes with θ or t, so every open-beam measurement is
  the same simulation.
- **Option B has a 60° degeneracy at t = 0.** θ = 0 and θ = 60° give identical
  counts, because the ray cuts the same 30 mm of poly and 20 mm of steel in the
  opposite order and attenuation is order-independent. Correct physics; do not
  read it as a stuck geometry.
- **Parallel-beam redundancy is `p(θ,t) = p(θ+180°, −t)`**, not
  `p(θ+180°, t)`. The detector coordinate reverses. Getting this wrong is how a
  sinogram ends up mirrored.
- **`/cttwin/scan/translation` is guarded at ±150 mm.** Beyond that is almost
  certainly a unit slip; the command is refused rather than walking the phantom
  toward the world boundary.
