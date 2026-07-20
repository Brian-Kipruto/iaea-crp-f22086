# Pass 2 Retrospective — Collimated pencil beam

> Period: 2026-07-20. Sessions: 1.

## The surprise

**The pass was almost no code — and that was correct, not a shortcut.** Going
in, "replace the placeholder with a collimated pencil beam" sounded like real
work. It wasn't: the Pass 0 placeholder was *already* a single `(1,0,0)` ray
from `(-kSourceToIso, 0, 0)`, because v1's 30° Ir-192 cone was never ported.
So the physics of the beam didn't change — only three things did: the framing
(placeholder → deliberate abstraction), the documentation of the idealisations,
and the *verification* that the ray behaves as a line-integral sampler.

The instinct to pad a suspiciously-small pass with machinery it doesn't need is
the trap here. The value of Pass 2 lives in the checkpoint and the docs, not in
lines of code. A fresh chat re-deriving this pass should expect a ~35-line diff
and not go looking for what it "missed."

## What worked

**The two-number checkpoint proved intent, not just function.** Empty-world
0.9978 alone would only show "photons reach the detector." Pairing it with the
pipe 0.5012 drop shows the beam samples a *path*: unchanged through vacuum,
attenuated through steel. And empty-world landing at *exactly* the Pass 1 number
(0.9978, not merely "near 1.0") confirmed the beam geometry was untouched by the
rewrite — a free regression check against Pass 1.

**Deciding the idealisations up front, before GO, kept scope honest.** Two
questions were resolved in the spec: keep the beam zero-width (defer real
divergence to Pass 6+), and defer the `SetSourceY` translation hook to Pass 4
with its caller. Both pulled *against* an initial "make it as real-world /
robust as possible" instinct — and both were the right call, because a finite
beam spot would have quietly sabotaged Pass 3's Beer–Lambert test and a setter
with no caller is untestable dead code. Resolving them before writing code meant
no rework.

## What was hard

**Nothing technical.** The one real judgement was *not* over-building: resisting
the finite-spot and the premature translation hook, both of which sound more
"complete" but cost clean validation or add dead code.

**Procedural: the empty-world checkpoint still needs the temporary `"none"`
edit.** Unchanged from Pass 1 — edit default, rebuild, verify, revert. Ran
clean; the `git status` + grep guard confirmed the throwaway didn't ship. This
recurs until Pass 4 gives the messenger.

## What to do differently next time

**Nothing.** Spec → decisions-before-GO → GO → checkpointed build → verify both
halves → revert → commit ran clean. The only lesson is the meta one already
recorded above: trust a small diff when the pass is genuinely about intent and
verification.

## Numbers

- 2 checkpoint runs (empty-world 10k, pipe 10k)
- 1 code commit: `feat:` (2 files, +35 / -11)
- Empty world: 0.9978 detected · Pipe: 0.5012 detected (both reproducible,
  empty-world ≡ Pass 1)
- 0 new ADRs (no locked decision changed) · 0 troubleshooting entries (no
  time-costing errors)