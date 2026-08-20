# Pass 5 retrospective — the pass where the model did the verifying

**Date:** 2026-08-20
**Code commit:** `80d07a2`

Pass 5 was billed as the least predictable pass, the only one whose failure
mode is *"the picture looks wrong and I don't know why."* It closed in a day of
wall-clock work plus ~14 hours of unattended compute, with every checkpoint
passing on the first run of the real binary.

That is not because the risk was overstated. It is because the risk was moved.

---

## The one decision that mattered: verify the sinogram, not the reconstruction

The handoff's warning list was mostly about reconstruction: TomoPy and ASTRA
have different geometry conventions, budget a day for a flipped reconstruction,
validate on the pipe first because it hides convention errors.

The thing that dissolved all of it was noticing that **the sinogram is
predictable in closed form.** Circles, a pencil beam, and the chord of a ray
through a circle. So `phantom_model.py` predicts `p(theta, t)` exactly, and the
measured sinogram can be scored against it before any reconstruction runs.

The payoff is that two failures which look identical in an image — "my driver
is wrong" and "TomoPy indexes differently" — become two measurements in two
different files. When `rot 270` turned out to be needed, there was no
investigation: the sinogram had already matched the model at RMS pull 1.04, so
the transform was provably a library convention. The handoff budgeted a day for
that. It cost a printed table.

**Carry forward:** when the deliverable is an image, find the intermediate
representation that is checkable as a number, and check that instead.

## The checkpoint became a number, and that changed the claim

The vault's checkpoint was *"the Option B reconstruction is recognisable."*
With a ground-truth mu map, it became per-material ROI means against NIST.

The claim went from "the picture looks right" to *"recovered mu within 1% across
Z_eff 5.5–26"* — which is a sentence that survives review. Cost: the ROI
arithmetic, which was already needed for the figure.

This is the Pass 4 scar (*make qualitative checkpoints quantitative if the
geometry is hand-computable*) paying off a second time.

---

## What actually went wrong

### The template substituted its own documentation

The worst bug of the pass, caught in a stand-in harness before the real binary
ever ran. `full_scan_template.mac` documents its own placeholders in a header
comment, and a whole-file `str.replace` hit those occurrences too — injecting
the entire 324-line translation block into a comment where only the first line
stayed commented. The rest executed **above `/run/initialize` at the default
phantom**, exiting zero and writing a plausible CSV.

Full write-up: [`troubleshooting/003`](../troubleshooting/003-template-substituted-its-own-documentation.md).

### A redundancy test that tested nothing

The obvious 180°-redundancy check is theta = 0 against theta = 180. It is
degenerate: at theta = 0 the Option B bars sit at y = 0, ±51.96, ±51.96, 0 mm
with each material appearing symmetrically on both sides, so the projection is
**even in t** — and against an even row, `p(theta+180, -t)` and the wrong
`p(theta+180, +t)` are identical. Measured: **both gave 0.8 sigma.** A green
tick that means nothing.

Moving the partner to theta = 30° (offsets +30, +60, +30, −30, −60, −30 mm with
steel at +30 and poly at −30 — genuinely asymmetric) separated them
**0.75 sigma vs 209.8 sigma.**

**Carry forward:** a passing test whose *failing* case would also pass is worse
than no test. The assembler now prints the wrong-relation figure alongside the
right one specifically so a degenerate test announces itself.

### An ROI that measured the wrong material

`roi_masks` originally shrank each positive-mu cylinder by a radial factor.
Correct for the bars, wrong for the pipe: the pipe is a solid steel disc plus a
*negative* bore disc, so shrinking the outer disc put the ROI at 42.4 mm radius
— entirely inside the 64.1 mm bore. It reported the air cavity's mu as the
steel's and produced a **−103% "error"** that was pure masking bug.

Fixed by selecting ROIs from the **net** mu map, so a material's ROI is
wherever that material actually ends up. Nesting can no longer mislead it.

**Carry forward:** a compositional geometry model needs a compositional mask.
If the forward model composes by addition, so must everything derived from it.

### An ROI that made two scans incomparable

Erosion was originally a fixed **pixel** count — a 2 mm margin at 2 mm pitch,
1 mm at 1 mm pitch. So the finer scan drew its ROI physically *closer* to the
edge, cancelling the resolution it had just paid for. Changed to a fixed
physical margin.

**Carry forward:** any tolerance compared across two sampling grids must be in
physical units, or the comparison measures the tolerance.

---

## Three explanations, two of them wrong

The FBP positive bias was explained three times before the explanation matched
the data.

1. *"Thin features read low from partial-volume averaging."* Written into the
   code as a warning **before** any measurement. Every deviation came back
   positive.
2. *"Ramp-filter overshoot at the edge; the ROI sits on the ringing."*
   Falsifiable, so it was tested: widening the ROI margin should reduce it.
   It made it **worse** (+25.85% → +27.92%).
3. What the data actually constrains: ordered by object *size* not mu,
   pitch-independent, margin-independent, and the background goes negative
   while every material goes positive — a redistribution, not a scale error.

The bad one was (1): it was asserted in a code comment before being measured,
so it would have been read as established by whoever came next. (2) was wrong
but cheap, because it was stated as a prediction that a two-line change could
test.

**Carry forward:** an explanation written into the code is a claim with the
project's authority behind it. Either measure it first, or mark it as a
hypothesis and say what would falsify it.

---

## What was cheap and worth repeating

- **A stand-in binary.** ~40 lines of Python that parses the real macro
  grammar and draws binomial counts from the analytic model. It found the
  template bug, the unit-duplication bug, the degenerate redundancy test and
  the invariance units bug — all before Geant4 ran once. Every checkpoint then
  passed on the first real attempt.
- **A cheap spot-check on a finished angle mid-scan.** 90 minutes into the
  pipe run, `angle_0000.csv` was already complete and Pass 4 had banked exact
  predictions for its theta = 0 row. Three numbers, ~0.25% agreement once the
  air path was included, and the remaining 90 minutes was known to be mere
  accumulation.
- **Running the cheap validators first.** `xcom_reference.py` and
  `phantom_model.py` both run in a second and both exercise the edited code.
  A file that failed to land would have surfaced there rather than after a
  build.

---

## What was deliberately not done

- **ASTRA.** conda-only, and no NUTECH claim depends on which library does the
  iterative reconstruction. TomoPy's own SIRT delivered sub-1%. Not worth
  deadline.
- **Any C++ change.** Holding that line kept the Pass 1–4 anchors valid and
  kept a rebuild out of the critical path.
- **The air correction.** Normalising by the open beam leaves a 0.05% term
  against mu_Al. Documented as deliberately neglected, not overlooked.
- **Scatter correction.** The total-count sinogram is assembled and available;
  reconstructing it is a Phase 2 question.
- **More SIRT iterations.** Faint streaks persist through the highest-contrast
  bars at 200 iterations. Cosmetic at sub-1% accuracy; a candidate if a prettier
  figure is ever wanted.

---

## Numbers worth remembering

- **12,804 primaries/s/core**, single-threaded, ~5.4e9 primaries across three
  scans.
- **RMS pull 1.04 and 1.06** — two sinograms against a parameter-free model.
- **161x** separation on the sign convention; **0.75 vs 209.8 sigma** on
  redundancy.
- **Sub-1% on three materials.** Claim 2, banked, five weeks early.
