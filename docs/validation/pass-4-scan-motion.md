# Pass 4 validation — scan motion, sign convention, and the CSV contract

- **Date:** 2026-08-15
- **Pass:** 4
- **Commit:** `e240918` (docs `dbd7547`+)
- **Geant4:** 11.2.1, MT
- **Related:** [ADR 0005](../decisions/0005-phantom-carries-the-scan-transform.md),
  [features/pass-4-scan-motion.md](../features/pass-4-scan-motion.md)

## What was tested

Pass 4's stated checkpoint was qualitative — "360° rotation via macro visibly
changes the Option B projection." It was replaced with a seven-rung ladder,
because the configurations involved have hand-computable chord lengths and so
the checkpoint could be made quantitative at no extra cost.

Macros: `macros/checkpoints/c*.mac`. All runs 10⁵ primaries unless noted.

| # | Test | Criterion | Result |
|---|---|---|---|
| 5 | Pass 1–3 regression anchors | **exact** | ✅ exact |
| 1 | Pipe rotational invariance | θ-independent | ✅ χ² 7.95/4 |
| 3 | Bars rotation (headline) | visibly changes | ✅ factor **4.00** |
| 2 | Parallel-beam redundancy | agree within 1σ | ✅ 0.26σ |
| 4 | Sign convention | predicted material pattern | ✅ exact pattern |
| 6 | Stale geometry | bit-for-bit | ✅ identical |
| 7 | CSV contract | header + 3 rows, tracked counts | ✅ |

## Checkpoint 5 — the anchors, exactly

The Pass 4 changes had to be provable no-ops at θ = 0, t = 0. Not "within
statistics" — exactly, because the scan transform touches every phantom
placement and the unscattered gate was rewritten.

| Config | Metric | Pass 3 | Pass 4 |
|---|---|---|---|
| empty world, 10⁴ | total | 0.99600 | **0.99600** |
| | unscattered | 0.99580 | **0.99580** |
| pipe, 10⁴ | total | 0.47950 | **0.47950** |
| | unscattered | 0.47060 | **0.47060** |

Identical to all five digits, and re-verified a second time after the CSV output
was added. Two design choices earned this: the phantom carries the transform so
the beam and detector never move (ADR 0005), and the θ = 0 case keeps a
`nullptr` placement rotation so the unrotated geometry stays on the same
navigator code path the anchors were measured on.

## Checkpoint 1 — rotational invariance of the pipe

A cylinder about +z is invariant under rotation about +z. Counts must not depend
on θ. This is a known-answer test requiring no reference data, and it fails
loudly if the rotation is about the wrong axis or drags a spurious translation.

t = 0, unscattered per 10⁵ (1σ = 158):

| θ | counts | dev |
|---|---|---|
| 0° | 47208 | +1.98σ |
| 17° | 46783 | −0.71σ |
| 45° | 46898 | +0.02σ |
| 90° | 46611 | −1.80σ |
| 360° | 46977 | +0.52σ |

χ² = 7.95 on 4 dof (p ≈ 0.09). The θ = 360° point was added specifically as a
discriminator: it is physically identical to θ = 0 but goes through the
rotation-matrix placement path rather than the `nullptr` path. **θ = 0 vs
θ = 360° agree to 1.03σ**, which exonerates the code-path split as an
explanation for θ = 0 sitting high, leaving ordinary fluctuation.

## Checkpoints 3 and 4 — rotation and the sign convention

Geometry of Option B: ring bars at radius 60 mm, steel (r = 10 mm) at
φ = 0°, 120°, 240°, polyethylene (r = 15 mm) at 60°, 180°, 300°, central
aluminium bar r = 20 mm. A bar's perpendicular offset from the ray is
`d(φ) = |60 sin(φ + θ) − t|`, so it is cut dead centre when
`60 sin(φ + θ) = t`.

**Checkpoint 3, t = 0.** At θ = 0 the ray cuts 30 mm poly + 40 mm Al + 20 mm
steel. At θ = 30° no ring bar intersects at all — every one sits at least 30 mm
off the line, further than its own radius — leaving only the 40 mm aluminium
core.

| θ | measured | predicted | Δ |
|---|---|---|---|
| 0° | 0.11118 | 0.1104 | +0.71% |
| 30° | 0.44469 | 0.4457 | −0.23% |
| 60° | 0.11013 | — | 0.75σ from θ = 0 |

Ratio 30°/0° = **4.00**. The θ = 60° run tests order invariance: the ray then
cuts the same 30 mm poly and 20 mm steel in the opposite order, and exponential
attenuation is order-independent, so it must match θ = 0. It does. **Sampling
0/60/120 would have looked like a stuck geometry** — a trap worth recording.

**Checkpoint 4, |t| = 60 mm** isolates exactly one bar (the central Al bar is
60 mm off the ray; the baseplate is 75 mm below it in z):

| θ | t | bar | material | chord | measured | predicted | Δ |
|---|---|---|---|---|---|---|---|
| +30° | +60 mm | φ = 60° | poly | 30 mm | 0.77928 | 0.7853 | −0.77% |
| −30° | +60 mm | φ = 120° | steel | 20 mm | 0.31334 | 0.3154 | −0.65% |
| +30° | −60 mm | φ = 240° | steel | 20 mm | 0.31399 | 0.3154 | −0.44% |
| −30° | −60 mm | φ = 300° | poly | 30 mm | 0.77396 | 0.7853 | −1.45% |

The predicted poly/steel/steel/poly pattern, a factor 2.5 apart. The relative
sign of θ and t is confirmed.

**Checkpoint 2 — redundancy.** For a parallel beam,
`p(θ, t) = p(θ + 180°, −t)`, *not* `p(θ + 180°, t)` — the detector coordinate
reverses. Assuming otherwise is how a sinogram ends up flipped. Measured:
p(30°, +60) = 77928 vs p(210°, −60) = 77880, a difference of **0.26σ**.

## The mirror pair: a 2.86σ that was chased and cleared

p(+30, +60) and p(−30, −60) are exact mirror images and should agree. The first
measurement gave 77928 vs 77396 — **2.86σ apart**. Three additional independent
seeds were run on the isolated pair:

| seed | (+30, +60) | (−30, −60) | difference |
|---|---|---|---|
| default | 77928 | 77396 | **+532** |
| 1111 | 77683 | 77826 | −143 |
| 2222 | 77662 | 77746 | −84 |
| 3333 | 77740 | 77764 | −24 |

**The sign flipped.** A real geometric asymmetry cannot reverse sign with the
RNG seed. The three new differences average −0.78σ, consistent with zero.
Pooling all nine measurements of what is physically one configuration
(the two mirror partners plus the redundant p(210°, −60)): mean 77736,
χ² = 11.0/8 dof, and **4.3/7 dof after removing the single −2.59σ point**. One
2.6σ outlier in nine samples is expected roughly once. Conclusion: fluctuation,
not asymmetry.

## Checkpoint 6 — stale geometry

The one test that justifies letting geometry move after `/run/initialize`.
Identical seeds set before each `beamOn`, so the comparison is exact rather than
statistical.

| | total | unscattered |
|---|---|---|
| batched run 1 (θ = 0) | 11744 | 11000 |
| separate process (θ = 0) | **11744** | **11000** |
| batched run 2 (θ = 30°) | 45105 | 44353 |
| separate process (θ = 30°) | **45105** | **44353** |

Bit-for-bit identical under MT. `GeometryHasBeenModified()` is doing its job;
there is no stale-geometry path.

## Checkpoint 7 — the CSV contract

`macros/checkpoints/c7-output-csv.mac`, three translations at one angle,
5 × 10⁴ each, appended to one file:

```
projection_id,angle_deg,translation_mm,pixel_index,n_counts,n_unscattered,n_events
0,0,0,0,23947,23467,50000
1,0,20,0,22966,22471,50000
2,0,40,0,19852,19357,50000
```

| id | t | steel path | measured | predicted | Δ |
|---|---|---|---|---|---|
| 0 | 0 mm | 13.10 mm | 0.46934 | 0.4696 | −0.12σ |
| 1 | 20 mm | 13.72 mm | 0.44942 | 0.4531 | −1.66σ |
| 2 | 40 mm | 16.30 mm | 0.38714 | 0.3905 | −1.56σ |

Header written once, three rows appended, ids and scan coordinates correct,
normalisation carried. Both anchors were re-run afterwards and still gave
0.99600/0.99580 and 0.47950/0.47060, confirming the output path is inert when
unset — `validate_beer_lambert.py` is untouched.

## Independent validation of the geometry, as a by-product

Pipe chords are hand-computable:
`L(t) = 2√(r_o² − t²) − 2√(r_i² − t²)` with r_o = 70.65 mm, r_i = 64.10 mm, and
μ = 0.5770 cm⁻¹ (carbon steel at 661.657 keV, [ADR 0004](../decisions/0004-beer-lambert-reference-and-acceptance.md)).

Pooling every independent sample taken during the pass:

| offset | path | events | measured | predicted | Δ |
|---|---|---|---|---|---|
| t = 0 | 13.10 mm | 5.6 × 10⁵ | 0.46902 | 0.4696 | −0.13% (0.90σ) |
| t = 40 mm | 16.30 mm | 4.5 × 10⁵ | 0.38875 | 0.3905 | −0.46% (2.46σ) |

The t = 40 mm case is the more interesting of the two: its chord depends on the
translation being implemented correctly, so this is a quantitative check of
translation and not merely of invariance.

**Open observation.** The off-axis deficit is 0.46% at 2.46σ while the on-axis
case sits at 0.13%. It does not scale with path length, so it is not a μ/ρ
error. At 2.5σ across a handful of samples it may well be statistical. It
affects no Pass 4 claim — the checkpoints were invariance, sign and contract —
but it should be settled in Pass 5 with one high-statistics run at t = 40 mm,
when that data is being generated anyway. Recorded here so it is not
rediscovered.

## Caveat on the predicted values

μ/ρ for **carbon steel** is the derived value from ADR 0004
(0.0735004 cm²/g at 661.657 keV). μ/ρ for **aluminium (0.07485)** and
**polyethylene (0.0857)** are literature-grade values, *not* derived by
`python/xcom_reference.py`. Residuals of 0.2–1.5% in the Option B tables are
consistent with that and should not be read as physics. Extending
`xcom_reference.py` to Al and poly would let these be quoted at Pass 3's rigour;
it is not required for any NUTECH claim.
