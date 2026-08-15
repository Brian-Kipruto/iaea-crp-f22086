# 002 — Delivered files land at a flattened path

> Pass 4. Severity: low individually, high cumulatively — five rounds lost, and
> it produced a bad commit that needed cleaning up.

## Symptom

Two distinct presentations, hours apart, with the same root cause.

**First**, IntelliSense reporting symbols that demonstrably existed:

```
namespace "CTTwin::Geometry" has no member "kDefaultScanAngle"
namespace "CTTwin::Geometry" has no member "kDefaultScanTranslation"
namespace "CTTwin::Geometry" has no member "kMaxScanTranslation"
```

All three live in one new block in `include/Constants.hh`, and no other file
complained.

**Second**, and repeatedly, `cttwin` refusing to run a macro that had just been
delivered:

```
ERROR: Can not open a macro file <../macros/checkpoints/c5-anchor-none.mac>.
Set macro path with "/control/macroPath" if needed.
```

The build was clean, the executable ran, and the macro "existed."

## Diagnosis

The files were **arriving, at a different path than the one the run command
referenced.** The download panel flattens directory structure: files delivered
as `macros/checkpoints/c1-pipe-invariance.mac` landed as
`macros/c1-pipe-invariance.mac`, so `macros/checkpoints/` did not exist at all
while `macros/` was full of correctly-named files.

This was only diagnosed at commit time, when `git status --short` showed **both**
copies — nine flat strays alongside nine in `checkpoints/` — and a diff showed
them to be different vintages of the same macros (the downloaded copies had
em-dashes and fuller comments; the heredoc copies were plain ASCII and included
a later `sed` edit). Both sets were committed in `e240918` and removed in
`dbd7547`.

The first symptom was a different failure mode of the same class, compounded by
tooling: `Constants.hh` had in fact landed correctly, and the IntelliSense
errors were a **stale cache** — the C/C++ extension does not re-parse dependent
translation units when a header changes underneath it. The compiler was never
consulted before the errors were reported as real. Two checks settled it in
under a minute:

```bash
grep -n "namespace CTTwin::Geometry\|kMaxScanTranslation" include/Constants.hh
find ~/projects -name "Constants.hh" -not -path "*/build/*"
```

— confirming the constants sat inside the namespace, and that only one
`Constants.hh` existed on the include path.

## Fix

Remove the strays, keeping the copies that were actually executed (those are the
ones the validation record must match):

```bash
git rm -q macros/c1-pipe-invariance.mac macros/c2-c4-bars-signs.mac \
          macros/c3-bars-rotation.mac macros/c5-anchor-none.mac \
          macros/c5-anchor-pipe.mac macros/c6-batched.mac \
          macros/c6-single-a.mac macros/c6-single-b.mac \
          macros/c7-output-csv.mac
git commit -m "fix: remove duplicate checkpoint macros from macros/ root"
```

For the stale IntelliSense: **Ctrl+Shift+P → C/C++: Reset IntelliSense
Database**. Cosmetic, but worth doing so four false errors aren't sitting in the
Problems panel while real checkpoint failures are being read.

## Lesson

The project already carried the scar *"check that a delivered file actually
landed."* Pass 4 shows it is pointed at the wrong question. The file **had**
landed. An existence check passes while the run still fails, because the failure
is about *where*.

**Revised check — verify the path, and do it before the run rather than after
the failure:**

```bash
ls macros/checkpoints/            # does the directory exist at all?
grep -c "<a phrase from the new content>" <path>   # right file, right version
```

A count of `0` means wrong path or wrong vintage; either way, stop before
running.

Three further points:

- **For a multi-file delivery into a new directory, generate with a terminal
  heredoc rather than downloading.** This is the Pass 0 scar ("overwrite whole
  files via terminal heredoc, never paste into the editor") one layer out: the
  heredoc carries the path explicitly, so it cannot be flattened. The switch
  should have happened after the second failure, not the fifth.
- **Diff before committing a directory that was populated twice.** `git add -A`
  with `git status --short` glanced at rather than read is what let nine
  duplicates into `e240918`.
- **An IDE diagnostic is not a compiler diagnostic.** Confirm against a real
  build before spending time on a symbol-not-found error, especially right after
  a header changes.
