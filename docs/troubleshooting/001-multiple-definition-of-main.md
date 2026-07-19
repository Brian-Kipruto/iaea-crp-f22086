# 001 — `multiple definition of 'main'` (duplicate cttwin.cc)

> Pass 0. Severity: low (build-blocking, obvious once located).

## Symptom

Clean CMake configure (Geant4 11.2.1 found), but linking failed:

```
/usr/bin/ld: CMakeFiles/cttwin.dir/src/cttwin.cc.o: in function `main':
cttwin.cc:(.text+0x171): multiple definition of `main';
  CMakeFiles/cttwin.dir/cttwin.cc.o:cttwin.cc:(.text+0x0): first defined here
collect2: error: ld returned 1 exit status
```

The build log showed `cttwin.cc.o` being built **twice** — once from the repo
root, once from `src/`.

## Diagnosis

Two files named `cttwin.cc` existed: the intended one at the repo root, and a
stray copy in `src/`, left over from shuffling files during the port. Both
define `main`, and the CMake source glob pulls everything in `src/`, so both
compiled and the linker saw two `main` symbols.

A follow-on gotcha compounded it: after deleting `src/cttwin.cc`, the next
`make` still failed with "No such file or directory" for the deleted file —
because CMake's file glob was evaluated at configure time and the generated
makefiles still referenced it. And separately, a **stale `cttwin` binary** from
an earlier build sat in `build/` and kept running on `./cttwin` even though
every build since had failed to link — masking which version was actually current.

## Fix

1. Remove the stray: `rm src/cttwin.cc` (keep the root one — the CMake usage
   comment documents `./cttwin`, root is correct).
2. **Re-run CMake** so the glob re-evaluates: `cmake ..` — deleting a globbed
   file is invisible to make until reconfigure.
3. When the binary's identity is in doubt, **clean-build**:
   `rm -rf build && mkdir build && cd build && cmake .. && make -j$(nproc)` —
   this also clears any stale binary from failed builds.

Verify exactly one `main` before rebuilding:

```
grep -rl "int main" *.cc src/*.cc      # must return only ./cttwin.cc
```

## Lesson

Porting files between repos with same-named files invites duplicate-symbol and
hybrid-file bugs. Three guards:

- **`grep -rl "int main"` after any port** — exactly one hit.
- **Overwrite whole files via terminal heredoc** (`cat > f <<'EOF'`), never
  paste into the editor — avoids partial-paste hybrids.
- **Clean-build after structural changes** — CMake globs and stale binaries
  both bite otherwise. Failed builds don't overwrite the previous binary, so
  `./cttwin` can keep running a ghost.

## Related

- [features/pass-0-port.md](../features/pass-0-port.md)
- [features/pass-0-port-retrospective.md](../features/pass-0-port-retrospective.md)