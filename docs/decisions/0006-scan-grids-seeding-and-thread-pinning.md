# ADR 0006 — Scan grids, per-measurement seeding, and thread pinning

**Date:** 2026-08-20
**Status:** Accepted
**Pass:** 5
**Code commit:** `80d07a2`
**Supersedes:** nothing
**Expires:** D1 and D2 expire with the NaI detector model (Pass 6+), which
changes the sampling and statistics arguments. D3 and D4 are permanent.

---

## Context

Pass 5 had to turn a working simulator into a scan. Four numbers were open and
they constrain each other: how many angles, how many translation steps, how
many photons per measurement, and how the compute is parallelised. Together
they set the compute budget, the file count, the achievable spatial resolution
and the noise floor.

Nothing in the vault fixed them. `Translate-Rotate Geometry.md` suggested
"180 x 1° is conventional" without tying it to detector sampling.

---

## Decision

### D1 — Two matched grids, both run in full

`t` spans **±80 mm** for both packages, covering the pipe (support radius
70.65 mm) and the bars (75.00 mm) on one grid.

| | pitch | N_t | N_angles | measurements |
|---|---|---|---|---|
| **A** | 2.0 mm | 81 | 180 @ 1° | 14,661 |
| **B** | 1.0 mm | 161 | 240 @ 0.75° | 38,801 |

Angular sampling in both comfortably exceeds `(pi/2) x (samples across the
object)`, so angular sampling is never the resolution limit.

**N_t is odd and symmetric about zero** so the rotation axis lands exactly on a
sample and the geometric centre is `(N_t - 1) / 2` exactly. Confirmed:
`find_center` returned 80.00 against a geometric 80.00 on package B.

**A first, then B.** A is the insurance policy — a complete banked result at
2.7 h. B is the publication figure. The pipe reconstruction settled which:
a 6.55 mm wall is 3.3 px at 2 mm pitch, marginal, and 6.6 px at 1 mm.

### D2 — 1e5 primaries per measurement, 1e7 for the open beam

The worst ray in the set (bars, theta = 0, t = 0, transmission 0.111) yields
~11,100 unscattered counts: 0.95% relative, sigma ~0.0095 on a line integral of
2.20. Measured across the whole scan, ray noise came out median 0.0024–0.0037.

The open beam gets three extra orders of magnitude because its error is
**common-mode**: it shifts every line integral by the same amount rather than
averaging out, so it is the cheapest place to spend photons. 1e7 costs the same
as 100 measurements out of 14,661, and yields 0.032% — an order below any
single ray.

**One open-beam run, not a sweep.** Under ADR 0005 nothing in the world changes
with theta or t when the phantom is `none`.

### D3 — Seeds are a scrambled function of the global measurement index

```
i  = angle_index * n_translations + translation_index
s1 = (i * 2654435761 + 12345)     % 2147483647 + 1
s2 = (i * 40503 + 987654321)      % 2147483647 + 1
```

Recorded as a **formula** in `manifest.json`, not as 39,000 stored integers.
The whole scan replays exactly.

Multiplicative scrambling rather than `i + 1` because adjacent streams are
precisely the correlation this mechanism exists to prevent, and scrambling
costs nothing.

### D4 — Threads pinned to 1; parallelism is over angles

`/run/numberOfThreads 1` in `full_scan_template.mac` (pre-init only, hence its
position), with a pool of single-threaded angle processes in `run_scan.py`.

---

## Options considered

**Grids.** A single fine grid (B only) was rejected: 8+ hours before any
end-to-end result, with the NUTECH claim unbanked the whole time. A single
coarse grid (A only) was rejected once the pipe showed a 3.3 px wall.

**Photons.** 1e6 per measurement (Pass 3's figure) would have cost 1.5e10
primaries for package A alone, ~27 hours. Pass 3 needed that precision to fit
mu against a 2% criterion; a reconstruction ray does not — FBP and SIRT both
average across hundreds of rays per pixel.

**Seeds.** Sequential seeds were rejected as above. A per-run random seed was
rejected because the scan would not replay.

**Threads.** One MT process per angle using all cores, with angles run
serially, was the alternative. Rejected for three reasons: it pays the G4 MT
thread merge and start-up cost 181 times; it scales worse than independent
processes for a workload that is already embarrassingly parallel; and — the
decisive one — **it left an unmeasured variable in the anchors.**

Whether Geant4 11.2's event seeding is thread-count independent depends on
`seedOncePerCommunication`, which this project had never measured. If it were
dependent, every Pass 1–4 anchor would only be reproducible at a fixed thread
count, and any anchor drift would be ambiguous between "seeding artefact" and
"structural break". Pinning removes the variable.

---

## Consequences

**Good.**
- Checkpoint 0 reproduced both anchors **exactly** at threads = 1, which
  settles the open question: seeding here *is* thread-count independent. The
  anchors are unconditional.
- 12,804 primaries/s/core, and package A completed in 2.7 h wall.
- The odd symmetric grid made the centre of rotation exact rather than fitted.
- Package B recovered mu to better than 1% on all three materials.

**Costs.**
- Package B took 8.3 h (contended), so it is an overnight job. Any future
  change requiring a B re-scan costs a night.
- Two packages means two of everything downstream — sinograms,
  reconstructions, figures. Manageable at this scale; would not be at ten.
- The 1e5 figure is tied to Option B's worst-case transmission. A denser
  phantom, or the NaI detector's efficiency, invalidates the arithmetic and it
  must be redone rather than inherited.

**Neutral.**
- One extra angle beyond 180° is acquired per scan purely to test the
  redundancy relation, and excluded from reconstruction. 0.6% overhead. Its
  partner is **30°, not 0°** — see `validation/pass-5-reconstruction.md`.
