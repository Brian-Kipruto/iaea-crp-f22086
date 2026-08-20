# Pass 5 validation — the reconstruction ladder

**Date:** 2026-08-20
**Code commit:** `<fill after push>`
**Verdict:** ✅ all seven checkpoints passed. **NUTECH abstract claim 2 banked.**

Seven rungs, each with an expected number written before it was run. Two
phantoms, two sampling grids, ~5.4e9 primaries total.

---

## Checkpoint 0 — regression anchors survive thread pinning

Run at `/run/numberOfThreads 1`, deliberately **without** `/random/setSeeds`,
to reproduce the Pass 1–4 conditions exactly except for the thread count.

| config | total | expected | unscattered | expected | |
|---|---|---|---|---|---|
| empty world | 0.99600 | 0.99600 | 0.99580 | 0.99580 | **EXACT** |
| pipe | 0.47950 | 0.47950 | 0.47060 | 0.47060 | **EXACT** |

**Finding: Geant4 11.2.1's event seeding is thread-count independent here.**
This was an open risk — if seeding had been thread-count dependent, every prior
anchor would only be reproducible at a fixed thread count. It is not, so the
anchors are unconditional and ADR 0006's thread pinning costs nothing.

## Checkpoint 1 — the driver, against numbers Pass 4 already banked

15 measurements, bars, 1e5 primaries each.

| | measured | Pass 4 | deviation |
|---|---|---|---|
| theta = 0°, t = 0 | 0.10970 | 0.11118 | −1.50 sigma |
| theta = 30°, t = 0 | 0.44449 | 0.44469 | −0.13 sigma |
| 60° degeneracy | 0.10970 vs 0.10912 | — | +0.42 sigma |

Different seeds, so a different realisation of the same physics. The 60°
degeneracy (same materials on the ray in opposite order; attenuation is
order-independent) is now confirmed three independent ways: Pass 4 measured it,
`phantom_model` predicts it, and the driver reproduces it.

## Checkpoint 2 — throughput

1,466,100,000 primaries in 31.81 core-hours, 161.8 min wall on 12 workers:
**12,804 primaries/s/core** (pipe, uncontended). Bars-A and bars-B run
concurrently gave 7,969 and 10,968 — the expected cost of sharing 12 workers,
not a regression.

## Checkpoint 3 — the pipe sinogram

The pipe is rotationally symmetric, so every sinogram column must be flat in
theta. It is what only the pipe can test.

| | result |
|---|---|
| shape | (181, 81) |
| **RMS pull vs analytic model** | **1.06** |
| mean pull | −0.33 sigma |
| rotational invariance (column scatter / expected) | median **1.00**, worst 1.10 |
| open beam | 0.995358 unscattered at 1e7 |
| ray noise on p | median 0.0037, worst 0.0145 |

14,580 measurements against a **closed-form model with no fitted parameters**,
scattering at exactly the counting statistics.

The mean pull is small and negative because forward-scattered photons still
reach the detector face, so the measured line integral sits slightly below pure
attenuation. One-sided and tiny: physics, not error.

Two tests correctly reported *no* result, which is the point of reporting them:
the sign-convention score tied across all four hypotheses (a rotationally
symmetric object carries no angular information), and the redundancy relation
did not separate (every pipe projection is even in t). Both are why Option B
exists.

The open beam agrees with Pass 4's 0.995292 (measured at 1e6) to 0.0066%,
inside its own 0.032% error — an independent confirmation of a banked number
that fell out for free.

## Checkpoint 4 — the Option B fingerprint

Each bar traces `t = 60 * sin(theta + phi)` in the sinogram, so the raw
sinogram carries a complete, hand-checkable signature of both the angular
direction and the theta↔t relative sign, readable *before* TomoPy is imported.

| | result |
|---|---|
| **sign convention, as-acquired** | RMS **0.00367** |
| sign convention, translation flip / angle flip | RMS 0.59163 (**161x worse**) |
| **redundancy p(30,t) = p(210,−t)** | **0.75 sigma** |
| the wrong relation p(theta+180,+t) | **209.8 sigma** |
| RMS pull vs analytic model | **1.04** |
| bar trace amplitudes | 60.0 mm x6, central bar 0.0 mm |
| ray noise on p | median 0.0024, worst 0.0096 |

**The (theta, t) sign convention is determined, not assumed.** `as-acquired`
ties exactly with `both flipped`, which is the expected mirror symmetry —
every Phase 1 phantom is symmetric about the beam axis, so the *absolute*
convention is unobservable (it becomes observable in Phase 3). What is pinned
down is the **relative** sign, which is the one that mirrors a reconstruction.

The redundancy partner is 30°, not 0°, and that matters — see the
retrospective. At 0° the projection is even in t and the test discriminates
nothing (measured: 0.8 sigma both ways on the pipe).

No baseplate trace appears, confirming the z = 0 slice analysis.

## Checkpoints 5 and 6 — reconstruction

Centre of rotation: geometric 40.00 px vs `find_center` 39.88 px (package A),
and **exactly 80.00 vs 80.00** on package B. The rig is centred by construction
and the reconstruction independently agrees to sub-pixel.

Orientation: **`rot 270`** on both packages and both algorithms. Because the
sinogram was already verified against the model, this is provably a TomoPy
convention rather than a driver bug — a different file, a different day.

### Pipe (package A) — 6.55 mm wall, 3.3 px

| | ROI mean | sd | NIST | dev |
|---|---|---|---|---|
| FBP (gridrec) | 0.64530 | 0.15228 | 0.57698 | +11.84% |
| **SIRT (200 it)** | 0.56417 | 0.04214 | 0.57698 | **−2.22%** |

### Option B — package A (2 mm pitch)

| | ROI mean | NIST | dev |
|---|---|---|---|
| FBP aluminium | 0.21798 | 0.20137 | +8.25% |
| FBP steel | 0.71854 | 0.57698 | +24.53% |
| FBP polyethylene | 0.09312 | 0.08273 | +12.57% |
| **SIRT aluminium** | 0.20640 | 0.20137 | **+2.50%** |
| **SIRT steel** | 0.58398 | 0.57698 | **+1.21%** |
| **SIRT polyethylene** | 0.08364 | 0.08273 | **+1.10%** |

CNR (steel vs poly): FBP 8.7, SIRT 11.8.

### Option B — package B (1 mm pitch) — **the headline result**

| | ROI mean | sd | NIST | dev |
|---|---|---|---|---|
| FBP aluminium | 0.21926 | 0.01014 | 0.20137 | +8.88% |
| FBP steel | 0.73809 | 0.03028 | 0.57698 | +27.92% |
| FBP polyethylene | 0.09642 | 0.00983 | 0.08273 | +16.55% |
| **SIRT aluminium** | 0.20256 | 0.00802 | 0.20137 | **+0.59%** |
| **SIRT steel** | 0.57937 | 0.00702 | 0.57698 | **+0.41%** |
| **SIRT polyethylene** | 0.08328 | 0.00856 | 0.08273 | **+0.66%** |

CNR (steel vs poly): FBP 13.1, **SIRT 18.9**.

> **The claim.** Reconstructed linear attenuation coefficients agree with
> NIST-derived reference values to better than **1%** for carbon steel,
> aluminium and polyethylene — Z_eff from 5.5 to 26, a factor of 7 in mu — from
> a simulated first-generation translate–rotate gamma CT scan at 661.657 keV.

---

## Finding — iterative reconstruction is what makes this quantitative

Not a refinement. On identical data, reproduced across two phantoms and two
sampling grids, FBP carries a large positive bias that SIRT does not.

Four things were measured about the FBP bias:

1. **It is ordered by object size, not by mu.** steel (r = 10 mm) +27.9% >
   poly (r = 15) +16.6% > Al (r = 20) +8.9%.
2. **It does not improve when the pitch is halved** (+24.53% → +25.85% on
   steel). That rules out partial-volume averaging.
3. **It does not improve when the ROI is pulled further from the edge**
   (+25.85% → +27.92% at a 2 mm margin instead of 1 mm). That rules out an ROI
   sampling boundary ringing.
4. **The background goes negative** (−0.0077) while every material goes
   positive. So it is a redistribution, not a scale error.

Consistent with the ramp filter's positive central lobe filling a compact
object against a compensating negative tail outside it — smaller object, larger
fractional excess. Recorded as what was measured; the mechanism is offered as
consistent-with rather than proven.

**SIRT is also insensitive to the ROI definition** — mean |dev| 0.55% at both a
1 mm and a 2 mm margin, where FBP moved 2 percentage points on the same change.
Flat interiors versus structured ones.

---

## Closing a Pass 4 open question — the off-axis chord

Pass 4 flagged pipe transmission at t = 40 mm sitting 0.46% below the
hand-computed prediction (2.46σ) against 0.13% on-axis (0.90σ), and assigned
it to Pass 5. Pooling the package-A pipe scan over 180 angles gives ~1.8e7
events per column — forty times Pass 4's statistics:

| t | chord | measured p | vs NIST mu | vs Pass 3 fitted mu |
|---|---|---|---|---|
| 0 mm | 13.10 mm | 0.75488 | −0.127% (−3.8 sigma) | **+0.013%** |
| +40 mm | 16.30 mm | 0.93882 | −0.150% (−4.8 sigma) | **−0.010%** |
| −40 mm | 16.30 mm | 0.93873 | −0.160% (−5.1 sigma) | **−0.019%** |

**It is not an off-axis effect.** The deviation is the same in relative terms
at both positions — path-length independent as a *fraction* of p, which is the
signature of a small error in mu. And it is one already measured: Pass 3 fitted
mu = 0.5761682 /cm against NIST 0.5769780, **−0.140%**. Re-scoring against the
fitted value collapses every residual to under ±0.02%.

So this is the known offset between Geant4's transport at 661.657 keV and the
NIST tabulation, and it is global rather than geometric. Pass 4 saw two
different significances from one cause because the chord is longer at t = 40 and
the error bar therefore tighter.

**The reasoning error worth recording.** Pass 4 concluded *"it does not scale
with path length, so it is not a mu/rho error."* That is backwards: p = mu x L,
so a mu error gives a constant deviation as a *fraction* of p. A constant
relative deviation is what a mu error looks like; a constant *absolute*
deviation is what would rule one out.

This is the third independent measurement of the −0.14% offset (Pass 3's slab
fit, and the pipe here at two positions). It also means the sub-1%
reconstruction deviations above, quoted against NIST, carry ~0.14% of it by
construction.

---

## Data provenance

| scan | directory | measurements | primaries |
|---|---|---|---|
| pipe, package A | `data/raw/pipe_pkgA` | 14,661 | 1.47e9 |
| bars, package A | `data/raw/bars_pkgA` | 14,661 | 1.47e9 |
| bars, package B | `data/raw/bars_pkgB` | 38,801 | 3.88e9 |

Each carries a `manifest.json` with the angle and translation grids, the seed
scheme, the git commit, the thread count and the executable path. Every scan is
replayable exactly.