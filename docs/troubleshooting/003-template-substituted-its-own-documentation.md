# 003 — A macro template substituted its own documentation

**Date:** 2026-08-20
**Pass:** 5
**Cost:** caught in a test harness before the real binary ran. Would have been
expensive: the run exits zero and writes a plausible CSV.

---

## Symptom

The first end-to-end run of `run_scan.py` against a stand-in binary produced
output that was wrong in two ways that did not obviously belong together:

- every run reported `phantom=pipe`, despite `--phantom bars` and a correctly
  rendered `/cttwin/phantom bars` line;
- one run showed `/run/beamOn 20000.` — a trailing full stop on an integer.

Return code zero. CSVs written. Row counts plausible.

## Diagnosis

`full_scan_template.mac` documents its own placeholders in its header comment.
It contains, in a comment, the sentence:

```
# Pass 5 will substitute {{PHANTOM}}, {{OUTPUT_FILE}}, {{ANGLE}} and
# {{TRANSLATION_BLOCK}}.
```

`render_angle_macro` filled the template with a whole-file `str.replace`, which
replaces **every** occurrence — including the ones inside that comment.

So the entire 324-line translation block was injected into the middle of a
comment line. Only its **first** line inherited the `#`. Every line after it
became a live command, positioned **above `/run/initialize`**, i.e. before the
phantom was ever set. The full stop that ended the documentation sentence
landed after the last `beamOn` argument, which is where `20000.` came from.

Net effect: the whole translation sweep ran twice — once at the default
phantom before initialisation, once correctly afterwards — and the CSV was
appended to by both.

## Fix

Substitute **line by line, skipping comments**, and assert that every
placeholder found a substitutable line:

```python
for line in TEMPLATE.read_text().splitlines():
    if line.lstrip().startswith("#"):
        out.append(line)          # comments are never substituted
        continue
    ...
unfilled = set(fields) - seen
if unfilled:
    raise RuntimeError(f"{TEMPLATE.name} has no substitutable line for ...")
```

A second, related bug surfaced in the same harness: the template line is
`/cttwin/scan/angle {{ANGLE}} deg`, so the unit is **already there**.
Substituting `"30.0 deg"` emits `angle 30.0 deg deg`, which
`G4UIcmdWithADoubleAndUnit` rejects — and a rejected `scan/angle` leaves the run
at the **previous** angle while still exiting zero.

## Lessons

- **A templating language and a documentation language must not be the same
  language.** Any file that explains its own placeholders will contain them.
  Either restrict substitution to non-comment lines (done here), or use a
  placeholder syntax that the documentation cannot accidentally contain.
- **Assert that every placeholder was consumed.** A template and its driver
  drift apart silently otherwise.
- **A rejected Geant4 command is not a failed run.** It leaves the previous
  configuration in place and exits zero. `run_scan.py` now scans stdout for
  `*** CTTwin ERROR`, `***** COMMAND NOT FOUND`, `command refused` and
  `illegal application state`, and refuses to keep that angle's numbers. This
  extends the Pass 3 lesson from `/cttwin` commands to *any* G4 command.
- **Check what the substitution produced, not just that it ran.** Counting live
  (non-comment) lines in a rendered macro is one line of Python and would have
  caught this instantly: 8 header + 81 x 4 = 332, not 656.
- **A stand-in binary that parses the real grammar is worth writing.** ~40
  lines of Python found this, the unit duplication, a degenerate redundancy
  test and a units error in a validation metric — all before Geant4 ran once.
