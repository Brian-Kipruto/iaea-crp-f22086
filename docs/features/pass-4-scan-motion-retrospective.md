# Pass 4 retrospective — the landmine that wasn't, and two flags chased to ground

- **Date:** 2026-08-15
- **Commit:** `e240918`
- **Companion:** [pass-4-scan-motion.md](./pass-4-scan-motion.md)

## The surprise: the landmine defused itself

The Pass 3 handoff opened and closed on one warning — the unscattered gate in
`SensitiveDetector.cc` tests the arriving direction against a hard-coded +x, and
"the moment Pass 4 translates or rotates the source, that test starts failing
for every photon and the unscattered count drops to zero." It was flagged as the
one thing that would bite.

It never fired, because the design decision taken at the top of the pass removed
the condition that triggers it. Moving the phantom instead of the rig leaves the
gun at `(−250, 0, 0)` aimed +x permanently. The gate was generalised anyway —
it now reads the launch state from the event's primary vertex — but as
future-proofing, not as a fix.

The lesson isn't "the warning was wrong." The warning was correct about the code
and correct that the pass had to address it. It's that **a well-chosen
architectural decision can dissolve a problem instead of solving it**, and it is
worth spending the first hour of a pass looking for that decision before writing
the fix. The alternative design — rotate the source–detector pair — would have
required exactly the gate work the handoff anticipated, plus moving a second
body.

There's a second-order benefit that only became visible afterwards. Because the
gate change is a *provable no-op* under this design, its correctness was
testable immediately, by the regression anchors, rather than in Pass 6 when the
source finally moves. A change that can't break anything today but is verified
today is strictly better than one deferred until it matters.

## Making the checkpoint quantitative cost nothing

The planned checkpoint was "360° rotation visibly changes the Option B
projection" — qualitative, and satisfiable by almost any implementation that
does *something*. Replacing it with a seven-rung ladder took about twenty
minutes of arithmetic and produced six hand-computed chord predictions, all
matching to 0.2–1.5%.

Two rungs earned their place beyond the headline:

- **Pipe rotational invariance.** A cylinder about +z is invariant under
  rotation about +z. No reference data needed to interpret, and it fails loudly
  if the rotation axis is wrong or if rotation drags a spurious translation.
  This is the Pass 3 "run the known-answer case first" scar applied to geometry
  rather than physics.
- **The t = 40 mm pipe chord**, 16.30 mm from
  `2√(r_o²−t²) − 2√(r_i²−t²)`. It depends on translation being implemented
  correctly, so it validates translation quantitatively rather than merely
  confirming that something moved.

The design trap found while doing that arithmetic was worth more than the
checkpoint itself: **at t = 0, θ = 0 and θ = 60° give identical counts**, because
the ray cuts the same 30 mm of poly and 20 mm of steel in the opposite order and
exponential attenuation is order-independent. Sampling 0/60/120 — the obvious
choice for "sweep the angle" — would have looked exactly like a stuck geometry.
Related: parallel-beam redundancy is `p(θ,t) = p(θ+180°, −t)`, not
`p(θ+180°, t)`; the detector coordinate reverses, and assuming otherwise is
precisely how Pass 5 ends up with a mirrored sinogram.

## Two flags, both chased, both cleared

The ladder produced two ~2.8σ anomalies. Neither was a failure by the stated
criteria, and both could have been waved through. Both were chased, and the
chasing was cheap.

**Flag 1: θ = 0 sat +2.11σ above the other pipe angles** (χ² 7.6/3, p = 0.055).
This one had a *candidate mechanism*, which is what made it worth chasing rather
than dismissing: θ = 0 is the one case deliberately kept on the `nullptr`
placement-rotation path, and the others go through the rotation matrix. The test
was to add **θ = 360°** — physically identical to θ = 0, but forced through the
matrix path. They agreed to 1.03σ. The code-path split was exonerated directly
and the flag became ordinary fluctuation.

That trick generalises: **to test whether two code paths through the same
physics agree, find an input that is physically degenerate but routes
differently.** Cheaper and more decisive than reasoning about floating-point.

**Flag 2: the mirror pair differed by 2.86σ.** p(+30°,+60) and p(−30°,−60) are
exact mirror images and should agree; they came out 532 counts apart. Three
fresh seeds on the isolated pair gave differences of −143, −84, −24 — **the sign
flipped**. A real geometric asymmetry cannot reverse sign with the RNG seed.
Pooling all nine measurements of what is physically one configuration gave
χ² = 11.0/8, dropping to 4.3/7 without the single −2.59σ point. One 2.6σ outlier
in nine samples is expected roughly once.

The general lesson, and it's the same shape as Pass 3's "agreement that's too
good is a finding": **when a discrepancy might be systematic, the discriminating
variable is usually the seed, not more statistics at the same seed.** Sign
reversal across seeds is conclusive in a way that a smaller error bar is not.

## The seed scar, demonstrated

Adding one `/run/beamOn` to the middle of a checkpoint macro shifted every later
run's counts by one position in a fixed sequence — the t = 40 mm group came back
as the previous values displaced by one, plus one new value. That is the
carry-forward "every `cttwin` process starts from the same default seed" scar
made visible.

It is also the concrete argument for per-measurement seeding in Pass 5. Without
a reseed, the same noise realisation lands at the same position in every angle's
sweep: correlated noise down a sinogram column, which reconstructs as structure
rather than grain. That artefact would be very hard to diagnose from the
reconstruction alone. `full_scan_template.mac` now generates
`/random/setSeeds` per measurement.

## What actually cost time: file paths, not physics

Five separate rounds were lost to delivered files not being where the run
command expected them. The eventual diagnosis was not the assumed one.

Files were **landing, at a flattened path** — the download panel dropped
`macros/checkpoints/c1-pipe-invariance.mac` into `macros/`, while the run command
referenced `macros/checkpoints/`. Both copies ended up committed in `e240918`
and had to be cleaned up in `dbd7547`. The existing scar ("check that a
delivered file actually landed") was the right instinct pointed at the wrong
question: the file existed, so an existence check would have passed while the
run still failed.

Full write-up and the corrected check in
[troubleshooting/002](../troubleshooting/002-delivered-files-land-at-a-flattened-path.md).
The behavioural change: **verify the path, not the file, and do it before the
run, not after the failure.** For a multi-file delivery into a new directory,
generating the files with a terminal heredoc is more reliable than downloading
them — which is the same conclusion the Pass 0 scar reached about editors, one
layer out.

## Scope: what was deliberately not done

- **Detector pixelation.** Tempting, because a wide static detector makes
  translation a pure source-offset needing no geometry change. Rejected: it
  moves the total-count anchors, `[[Detector Model]]` locks one pixel for
  Phase 1, and under the chosen design translation needs no rebuild anyway.
- **Deriving μ/ρ for aluminium and polyethylene.** The Option B predictions use
  literature-grade values, so their 0.2–1.5% residuals are not quotable as
  physics. Extending `xcom_reference.py` would fix that in ~20 minutes but is
  not required for any NUTECH claim, and the schedule risk is in Pass 5.
- **The off-axis chord observation.** Pooled t = 40 mm sits 0.46% below
  prediction at 2.46σ, while on-axis sits at 0.13%. It does not scale with path
  length, so it is not a μ/ρ error. Left open, to be settled in Pass 5 with one
  high-statistics run when that data is being generated anyway. Recorded in the
  validation doc so it isn't rediscovered.

## Carry-forward

- Every phantom volume must be placed through `PlacePhantomVolume()`. A direct
  `G4PVPlacement` will not follow the scan and fails silently.
- `WorldPositionOf()` uses +θ, the placement matrix uses −θ. Both correct;
  Geant4's placement rotation is the mother frame's, i.e. the inverse.
- The absolute (θ, t) sign convention is **fixed by ADR 0005, not by evidence** —
  all Phase 1 phantoms are mirror-symmetric about the beam axis, so a
  simultaneous flip of both signs is unobservable. It becomes observable in
  Phase 3. Re-derive against the first asymmetric phantom.
- Adding fields to `CTTWIN_RESULT` is safe; renaming or removing is not. The
  parser builds a dict and looks up by name.
- Pass 5 must seed per measurement and must run **one** open-beam reference, not
  an open-beam scan.
