# Pass 1 — SensitiveDetector + detector volume

> Status: Complete 2026-07-19. Commit `<fill after push>` on `main`. Geant4 11.2.1.

## What it does

Turns cttwin from a geometry into a measuring instrument. Adds a detector
volume on the far side of the phantom and a `G4VSensitiveDetector` that counts
photons arriving in it, then reports the run total. This is the first pass that
produces a *number* — the count that Pass 3's Beer–Lambert validation and
Pass 5's sinogram are both built on.

Still not a scan: the beam is the Pass 0 placeholder (mono-directional 662 keV
gamma along +x). The collimated pencil beam is Pass 2; Cs-137 and validation are
Pass 3; rotation is Pass 4.

## Files created / modified

New:
```
include/SensitiveDetector.hh    pattern-1 SD, counts gamma entries, one pixel
src/SensitiveDetector.cc
src/EventAction.cc              EventAction was header-only in Pass 0; now real
```

Modified:
```
include/Constants.hh            + detector geometry block (face, thickness)
include/DetectorConstruction.hh + detector LV member, ConstructSDandField(), "none"
src/DetectorConstruction.cc     + BuildDetector(), "none" branch, SD attach
include/EventAction.hh          + RunAction* ctor, SD count pull
include/RunAction.hh            dose accumulable -> G4Accumulable<G4int> count
src/RunAction.cc                prints count + fraction detected
src/ActionInitialization.cc     constructs RunAction, passes it to EventAction
```

## The detector

A thin `G4_AIR` box, 50.8 mm x 50.8 mm face (2", matching the eventual NaI so
the geometry carries over), 1 mm along the beam. Air, not scintillator: an
idealised counter registers arrival without attenuating. Placed at
`+kIsoToDetector` (150 mm) on the beam axis, opposite the source. Real NaI
response is deferred to Pass 6+ (Lockdown #2). Dimensions and distance are named
constants in `Constants.hh`, placeholder-flagged like the scan geometry.

## The count chain

```
SensitiveDetector.ProcessHits   count gamma at fGeomBoundary (one per photon), one pixel
        |  (EventAction pulls via G4SDManager::FindSensitiveDetector)
EventAction.EndOfEventAction     read SD count, hand to RunAction
        |
RunAction (G4Accumulable<G4int>) accumulate across run, merge threads, print total
```

Design notes:

- **Pull, not push.** EventAction resolves the SD by name and reads its count,
  rather than the SD holding a back-pointer into the action classes. Cleaner in
  MT mode — no cross-object pointer that can dangle across threads.
- **Count crossings, not energy.** `ProcessHits` counts gamma tracks entering at
  `fGeomBoundary` — one count per photon regardless of how many steps it takes
  in the volume. It does NOT accumulate `GetTotalEnergyDeposit()` (the v1 dose
  mistake in a new costume).
- **SD attached in `ConstructSDandField()`**, not `Construct()` — the MT-safe
  registration hook.
- **One pixel** (whole face) for Pass 1, but `ProcessHits` already captures hit
  position, so subdivision later needs no re-plumbing.

## Checkpoint

10k photons, empty world (`fActivePhantom = "none"`), placeholder beam:
**0.9978 detected** — essentially all photons land. See
[validation/pass-1-detector.md](../validation/pass-1-detector.md).

## Gotcha — the empty-world checkpoint needs a code change

The checkpoint requires an empty world, but the phantom defaults to `"pipe"` and
the messenger that would set `"none"` at runtime is Pass 4. So the checkpoint was
run by temporarily changing the default in `DetectorConstruction.hh` to `"none"`,
verifying, then reverting to `"pipe"` before commit. This pattern — checkpoint
needs a config the messenger can't yet provide — repeats until Pass 4. The
`"none"` default must never ship; committed default is `"pipe"`.

## Free cross-check

Same 10k beam through the pipe phantom gives **0.5012 detected** — reproducible.
Two anchored data points (0.998 empty / 0.501 pipe wall) that cross-check each
other. The pipe number is a ready sanity anchor for Pass 3's real Beer–Lambert.