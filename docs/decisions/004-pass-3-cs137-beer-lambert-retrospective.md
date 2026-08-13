# Pass 3 — retrospective

> Companion to [pass-3-cs137-beer-lambert.md](./pass-3-cs137-beer-lambert.md).
> Records the surprises. The recap lives in the feature doc.

## The surprise: the pass was nearly lost to bookkeeping, not physics

The physics was never in doubt. Geant4 transports 662 keV photons through steel
correctly; that was true before this pass started. What could have failed Pass
3 was three pieces of bookkeeping, and all three were invisible until someone
did the arithmetic:

- 10 000 primaries gives 2.94% spread at 40 mm, against a 2% criterion.
- The inherited μ/ρ was ambiguous at the 0.65% level, worth ~1.5% of
  transmission at 40 mm.
- Scatter contamination in the total count reaches 6.2% at 40 mm.

Any one of them alone could have produced a failing table with correct physics
underneath, or — worse — a *passing* table that was not reproducible. The
lesson generalises past this project: for a validation pass, the error budget
deserves as much design attention as the thing being validated, and it should
be drawn up before any code is written, not after the first failing number.

## The half-value-layer checkpoint paid for itself immediately

Setting the slab to exactly one HVL gives an answer fixed by construction:
0.5. No reference data, no interpolation, no argument. That checkpoint is where
the scatter problem became visible — total count read +2.38%, unscattered read
+0.70% — several hours before the production runs, when it was still cheap to
act on.

Worth keeping as a habit: **before running the real measurement, run the one
case whose answer you already know.** It costs one run and it is the only test
that can distinguish "the code is wrong" from "the reference is wrong", because
there is no reference involved.

## A 2σ warning that correctly did not become a failure

The HVL point read +0.70% ± 0.32%. Extrapolating that as a systematic in μ
predicted +2.32% ± 1.07% at 40 mm — a likely failure of the headline claim. The
production runs came back at +0.746%, and the fitted offset was −0.140%, not
−1.0%.

So the warning was a 2.1σ fluctuation, and calling it as "suggestive, not
proven, with roughly even odds" was the right call rather than either ignoring
it or treating it as established. The useful discipline was attaching an
uncertainty to the extrapolation instead of quoting the central value alone;
"+2.3%" would have triggered a day of unnecessary investigation, while
"+2.3% ± 1.1%" correctly said *find out cheaply, don't panic*.

The way to find out cheaply turned out to be the production runs themselves.

## Fitting μ was worth more than the four deviations

The per-thickness table answers the abstract's question and nothing else. The
fit answers the question that actually matters for everything downstream: *is
attenuation exponential in t?* Reduced χ² = 1.33 says yes, and that is a
statement about cttwin alone — no external data can make it pass or fail.

Which means the residual slope offset stops being a worry and becomes a
measurement of a library difference. Without the fit, the same physics would
have shown up as four unexplained per-thickness errors growing with thickness,
and the natural response would have been to go hunting for a geometry bug that
does not exist.

Carry forward: **when comparing a simulation against reference data, fit the
model as well as checking the endpoints.** The residuals validate your code;
the parameters characterise the difference between the two data sources. They
are separate claims and they want separate responses.

## The messenger deleted a recurring scar instead of adding one

Pass 3 needed five configurations, and the standing method was "edit the
default, rebuild, run, revert, check `git status` before commit" — which the
handoff predicted would recur twice this pass. Pulling two commands forward
from Pass 4 took about eighty lines and removed the dance entirely, permanently.

The general shape: a workflow scar that recurs every pass is a missing feature,
and the cost of building it early is usually less than the accumulated cost of
working around it. It also made `validate_beer_lambert.py` a real driver rather
than a log-parser, which is what the vault always said it should be.

The rejection path mattered as much as the happy path. A messenger that
accepted a post-init command and quietly did nothing would have produced a
validation table with the wrong thicknesses and no visible symptom.

## The nested-sample artefact

The variance sweep produced 29 933 at N = 300 000 and 99 777 at N = 1 000 000.
Since 0.3 × 99 777 = 29 933.1, those are not independent measurements — every
`cttwin` process starts from the same default seed, so the smaller run is
literally a prefix of the larger one.

Nothing is wrong, and the compute-budget conclusion is unaffected, but the
sweep demonstrates *convergence*, not run-to-run spread, and calling it a
"variance study" without that caveat would be a small misrepresentation in a
paper. Caught by noticing that two numbers agreed far more closely than their
own error bars allowed.

Carry forward: **agreement that is too good is a finding too.** Two independent
measurements matching to 0.1σ are telling you they are not independent.

## What would be done differently

- Draw up the error budget in the spec, before the first line of code. Two of
  the three near-misses above were arithmetic that could have been done on day
  one; the third (scatter) was measurable only after the HVL run, which is
  itself an argument for scheduling a known-answer checkpoint early.
- Set an explicit seed per run from the start, rather than inheriting the
  default and discovering the consequence in the results.

## What carries into Pass 4

- The unscattered gate assumes a single-energy on-axis source. Pass 4 moves the
  source. **This will break the gate and it will break quietly** — the count
  will simply fall to zero or near it once the beam is off-axis, and the
  symptom will look like a detector bug. Generalise the gate in the same pass
  that introduces translation, not after.
- `CTTWIN_RESULT` is now a parsed contract between C++ and Python.
- Messenger commands are pre-init only. Source position, angle and translation
  join the same directory in Pass 4 and inherit that constraint.
