# Pass 1 Retrospective — SensitiveDetector + detector volume

> Period: 2026-07-19. Sessions: 1.

## What worked

**The checkpoint caught the wrong-config run immediately.** The first 10k run
came back 0.5012 — exactly half. That "exactly half" was the tell: the phantom
was still in the world (default `"pipe"`), so the test was measuring
transmission, not the detector in isolation. The number diagnosed itself before
any code was suspected. A checkpoint that produces a *specific* expected value
(≈1.0) makes a wrong setup obvious; a vaguer "some photons detected" would have
passed silently and hidden the mistake.

**Pull over push for the count chain.** Having EventAction resolve the SD by
name via `G4SDManager` and read its count — rather than the SD holding a
back-pointer into the action classes — kept the wiring MT-clean with no
cross-thread pointer risk. Fewer moving parts than the push design originally
sketched.

**Reusing v1's dose-chain shape for counts.** The accumulate-per-event ->
merge-per-run structure was already proven in v1 (for dose). Swapping the
quantity from `G4double` energy to `G4int` count meant the wiring was familiar,
not invented — the only real change was what gets added.

## What was hard

**Nothing structural.** The vault's SensitiveDetector note carried the pattern-1
skeleton, the `fGeomBoundary` count-once guard, and the "count crossings not
energy" warning, so the class wrote itself against a known design.

**Procedural: the empty-world checkpoint needs a code change.** The 1d checkpoint
wants `fActivePhantom = "none"`, but the messenger that sets it at runtime is
Pass 4. So it was run by editing the default, rebuilding, verifying, and
reverting. Minor, but it's a pattern that repeats every pass whose checkpoint
needs a config the messenger can't yet provide — until Pass 4 closes it. The
risk each time is forgetting to revert; the mitigation is a `git status` /
grep-for-`"none"` check before commit.

## What to do differently next time

**Set the checkpoint config explicitly, don't assume the default.** The 0.50
detour cost one rebuild. When a checkpoint needs a specific phantom/geometry,
change it *first*, before the first assertion — same lesson the ranger simulator
retrospective reached about zeroing data before count checks.

**Nothing else.** Spec -> GO -> checkpointed build -> verify -> revert -> commit
ran clean.

## Numbers

- 4 checkpoints (build, detector+SD, count chain, empty-world 10k)
- 1 code commit: `feat:` (10 files)
- Empty world: 0.9978 detected · Pipe: 0.5012 detected (both reproducible)
- 0 troubleshooting entries — no time-costing errors (two `make` typos were
  immediate self-corrections, not worth an entry)