# Validation report

This document records the validation tests performed on `cttwin` and their
outcomes. It is the primary evidence base for any TRL claim made about the
simulator.

The report is intentionally append-only: as new tests are added in later
phases (detector response in Pass 6+, laminography geometry in Phase 3),
their results are appended here rather than overwriting earlier sections.

## Test 1 — Geometric overlap check

**Status:** Pass (carried over from v1 of the digital twin).

The Geant4 built-in geometry verifier (`/geometry/test/run`) reports zero
overlaps for the Option A and Option B phantoms in their combined
side-by-side visualisation configuration and in their individual
single-phantom scan configurations.

Console output is archived at `docs/figures/geometry_test_pass.png`
(carried over from `Phantom_Digitization.pdf` Figure 4).

## Test 2 — Beer-Lambert attenuation

**Status:** Not yet performed. Scheduled for Pass 3.

### Method

Fire a pencil beam of N0 mono-energetic 662 keV photons through a slab
of carbon steel of known thickness t. Count photons reaching the detector,
N. Compare against the analytical prediction

```
N_predicted = N0 * exp(-mu_rho * rho * t)
```

using the NIST XCOM mass attenuation coefficient for iron at 662 keV.

### Reference values

| Quantity              | Value                | Source                          |
| --------------------- | -------------------- | ------------------------------- |
| mu/rho (Fe, 662 keV)  | 0.07375 cm^2/g       | NIST XCOM                       |
| rho (carbon steel)    | 7.85 g/cm^3          | ASM Handbook (industrial 1018)  |
| mu (carbon steel)     | 0.5789 /cm           | computed: mu/rho * rho          |
| Half-value layer      | 11.97 mm             | computed: ln(2) / mu            |

### Acceptance criterion

Simulation must agree with analytical prediction within 2% relative error
for slab thicknesses of 5 mm, 10 mm, 20 mm, and 40 mm.

### Result

To be filled in after Pass 3 execution.

## Test 3 — Single-projection sanity check

**Status:** Not yet performed. Scheduled for Pass 4.

### Method

With Option B phantom (multi-material array) centred on the rotation axis,
fire a fan of pencil-beam projections across the detector at zero
rotation angle. The resulting transmission profile should show:

- Plateaus at near-N0 for rays missing all bars
- Deep dips where rays pass through the central aluminium bar
- Intermediate dips for polyethylene bars
- The deepest dips where rays pass through the steel bars

Visual inspection sufficient at this stage; this is a smoke test for the
translation logic, not a quantitative validation.

### Result

To be filled in after Pass 4 execution.

## Test 4 — Full-scan reconstruction sanity check

**Status:** Not yet performed. Scheduled for Pass 5.

### Method

Full 180-degree translate-rotate scan of Option B phantom. Reconstruct
with TomoPy FBP (`gridrec` algorithm). Expected outcome: a recognisable
cross-section showing the central bar, six outer bars in the correct
hexagonal arrangement, and the baseplate as a faint outer ring.

This is the first end-to-end test of the full pipeline (Geant4 -> Python
driver -> sinogram assembly -> reconstruction).

### Result

To be filled in after Pass 5 execution.
