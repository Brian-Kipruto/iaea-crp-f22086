# Workflow amendment — documentation is a per-pass deliverable

Adopted 2026-07-19. Fold these edits into the vault's Handoff Template so every
future handoff carries them.

## Why

Docs were happening informally. Making them explicit — mapped from the
ranger-v3 knowledge base — means every pass leaves a frozen record of what was
built, why, whether it was physically verified, and how to recover if it breaks.
A pass is not closed until its docs exist, the same way it is not closed until
its checkpoint number comes out right.

## Edit 1 — Closing ritual, step 3 expands

Replace step 3 of the closing ritual with:

> 3. **Update the vault note(s) AND write the pass docs.** In `docs/`:
>    - `features/pass-N-<name>.md` — what the pass built, end to end
>    - `features/pass-N-<name>-retrospective.md` — surprises, not recap
>    - `decisions/NNNN-<slug>.md` — one per ADR-worthy decision (if any)
>    - `troubleshooting/NNN-<slug>.md` — one per error that cost real time (if any)
>    - `validation/pass-N-<test>.md` — if a checkpoint number was measured
>    Then update the `docs/README.md` index with links to the new files.

The rest of the ritual is unchanged (verify → commit → docs → validation_report
→ push → write next handoff).

## Edit 2 — Handoff gains a required section

Every handoff now states, explicitly, what docs the pass produced. Add this
section to the template (after "Decisions made this session"):

> ## Docs written this pass
> - `features/pass-N-<name>.md` — <one line>
> - `features/pass-N-<name>-retrospective.md` — <one line>
> - `decisions/NNNN-<slug>.md` — <if any, else "none">
> - `troubleshooting/NNN-<slug>.md> — <if any, else "none">
> - `validation/pass-N-<test>.md` — <if a number was measured, else "n/a">

A handoff with an empty "Docs written" section is a **visible failure** — the
same signal as a pass that compiled but was never verified.

## The discipline (from ranger, keep it)

- **Retrospectives record the surprise, not the recap.** A clean pass earns a
  short retrospective ("clean, checkpoint passed first try, one gotcha: X").
  Padding it is worse than brevity. Ranger's simulator retrospective opens "What
  was hard: Nothing structural" and then gives the one real caveat — model that.
- **ADRs are frozen; the vault's Architecture Lockdown is live.** Lockdown is the
  index (current truth, rewritten on change); ADRs are the entries (full context,
  frozen at decision time). A decision worth a Lockdown row is usually worth an ADR.
- **Troubleshooting entries are earned, not routine.** Write one when an error
  costs real time, so a fresh chat or machine doesn't rediscover it.
- **Validation is the cttwin-specific store.** One file per measured checkpoint —
  method + expected + actual. This is what the September paper cites.

## Numbering

- Decisions: 4-digit, sequential, never reused. Next free: **0003**.
- Troubleshooting: 3-digit, sequential. Next free: **002**.
- Features/validation: keyed by pass number, no separate counter.