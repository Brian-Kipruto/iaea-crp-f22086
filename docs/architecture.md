# Architecture

This document records the technical design of `cttwin`. It is intentionally
written for someone new to the codebase — a future student, a collaborator
from another CRP participating country, or yourself in six months when the
context has faded. Keep it accurate as the code evolves: if the architecture
changes, this file changes in the same commit.

## Purpose

`cttwin` is a Monte Carlo simulator for first-generation gamma computed
tomography of industrial components, contributed to IAEA CRP F22086 by the
Kenya team (University of Nairobi, Institute of Nuclear Science & Technology).
The simulator produces synthetic projection data that downstream code
(TomoPy, ASTRA Toolbox) reconstructs into cross-sectional images, and that
later phases of the CRP use to train AI models (CNN, SRGAN) for image
enhancement and defect classification.

## Scope

In scope for v2 (Phase 1 completion):
- A digital twin of the IAEA-Indonesian pipe phantom (Option A) and the
  multi-material validation phantom (Option B)
- A Cs-137 pencil-beam source at 662 keV
- An idealised photon-counting detector
- True first-generation translate-rotate scan geometry
- A Python driver that produces sinograms from a full scan
- TomoPy FBP and ASTRA SIRT reconstructions
- Physics validation against analytical Beer-Lambert attenuation

Out of scope for v2 (deferred to later phases):
- Realistic NaI(Tl) detector response (Compton continuum, energy resolution,
  efficiency curve)
- Multi-source simulation (Co-60, Am-241)
- Cone-beam or fan-beam geometries
- Laminography (tilted-axis geometry) — Phase 3
- Synthetic dataset generation for AI training — Phase 4

## System tiers

`cttwin` is a three-tier system. Each tier has a single responsibility and
communicates with its neighbours through well-defined interfaces.

### Tier 1 — Geant4 simulator (C++)

Lives in `src/` and `include/`. Compiled into a single executable, `cttwin`.

Responsibilities:
- Construct the simulated world (phantom, source, detector)
- Generate primary photons according to the source spectrum and geometry
- Track photons through matter using Geant4's electromagnetic physics
- Score photon crossings at the detector
- Write per-projection output to disk in CSV format

Key classes:
- `DetectorConstruction` — phantom + detector geometry. Active phantom
  selectable at runtime via the messenger.
- `PrimaryGeneratorAction` — Cs-137 pencil beam, position and direction
  set per-event from the messenger state.
- `DetectorMessenger`, `PrimaryGeneratorMessenger` — expose `/cttwin/...`
  UI commands so geometry can be changed between runs without recompiling.
- `SensitiveDetector` — counts photons crossing the detector pixel(s),
  accumulates per-run totals.
- `RunAction` — at end of run, writes the projection to CSV.
- `ActionInitialization` — wires the user actions to Geant4.

### Tier 2 — Python driver

Lives in `python/`. Runs on the host, invokes the Geant4 executable as a
subprocess per projection.

Responsibilities:
- Define the scan grid: which angles, which translations, how many photons
- Generate a Geant4 macro per projection from `full_scan_template.mac`
- Run the `cttwin` executable in batch mode with each macro
- Collect the resulting CSVs and assemble them into 2D sinogram arrays

The driver is deliberately stateless across projections — each Geant4 run
is independent, which makes the scan trivially parallelisable (one process
per projection, or per chunk of projections) on a multi-core machine or
cluster.

### Tier 3 — Post-processing

Also in `python/`. Reads sinograms, produces reconstructions and validation
outputs.

Responsibilities:
- Convert raw transmission counts `N` to line integrals `-ln(N/N0)`
- Run FBP reconstruction (TomoPy `gridrec`)
- Run iterative reconstruction (ASTRA SIRT)
- Compare against analytical predictions where applicable (Beer-Lambert)
- Save reconstructions and produce figures

## Data flow

```
PrimaryGenerator -> photon -> phantom -> SensitiveDetector -> CSV row
                                                                |
                                                                v
                          run_scan.py orchestrates many runs -> data/raw/
                                                                |
                                                                v
                          assemble_sinogram.py                  data/sinograms/
                                                                |
                                                                v
                          reconstruct.py                        data/reconstructions/
```

CSV row schema (one row per Geant4 run):

```
projection_id, angle_deg, translation_mm, pixel_index, n_counts
```

## Decisions and their justifications

Significant design decisions are recorded here so future readers (and
future you) understand not just *what* the code does but *why* it does it
that way.

### Idealised photon counting before NaI realism
*Decided in Pass 1.* Validation against Beer-Lambert is unambiguous with
ideal counting; with NaI energy deposition it requires backing counts out
of a smeared spectrum, conflating transport-physics errors with detector
response errors. NaI detector response is a planned Pass 6+ extension.

### Cs-137 as the single Pass-1 source
*Decided in Pass 1.* Single dominant line at 662 keV makes Beer-Lambert
validation trivial. Matches Ghana's and Indonesia's CRP plans, so any
inter-country phantom comparison is on common ground. Kenya's other
available sources (Co-60, Am-241) are added in later passes.

### Translate-rotate, not fan-beam
*Decided in Pass 1.* Matches Kenya's existing first-generation hardware
(portable clampable CT, 1-detector configuration). Simpler geometry to
validate; fan-beam is a later optimisation.

### Apache 2.0 licence
*Decided at project setup.* Compatible with Geant4's own licence terms.
Permissive enough for industrial partners but requires attribution.

## Open questions

Items the team has not yet decided. Resolve and move into "Decisions"
above as they are answered.

- Source-to-detector distance for the simulated geometry: matches hardware
  exactly, or chosen for simulation convenience?
- Detector pixel size for the idealised counter: 1 mm? 0.5 mm? Match the
  physical Ludlum 2x2" footprint?
- Phantom-rotation strategy: rotate the phantom geometry, or rotate the
  source-detector pair around a stationary phantom? Equivalent physically,
  but one is easier to messenger-control than the other.
- Output format long-term: stay with CSV (simple, portable), or migrate to
  HDF5/ROOT once dataset sizes grow?
