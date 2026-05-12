# cttwin — Project Guide

> Master reference for Kenya's simulation contribution to IAEA CRP F22086.
> Read this forst!! Everything else (README, architecture
> notes, validation reports) is derived from or elaborates on this.

**Status:** Active draft, Phase 0 → Phase 1.
**Last updated:** 2026-05-12.
**Maintained by:** Brian Kipruto, Research Assistant.
**Supervisor:** Dr. Wilson Macharia Kairu, Chief Scientific Investigator,
University of Nairobi.

---

## How to read this document

If you're picking this up cold — a future AI assistant in a new chat, a new
collaborator, or me in three months — read sections in this order:

1. **Context & framing** (where am I, what is this project)
2. **Resume instructions** (how do I become useful immediately) — this is at
   the bottom, but read it second, before the technical material
3. **Current status & next actions** (what's the live state)
4. Then everything else, top to bottom, as reference

If you're me (Brian), checking the doc during normal development — jump
straight to "Current status & next actions" and "Decision log". Those are
the two sections that change frequently and matter most for day-to-day
work.

---

## 1. Context & framing

### 1.1 The CRP

IAEA CRP F22086 is a four-year Coordinated Research Project titled
**"Advanced Nuclear Imaging Techniques for Industrial Process Analysis and
Component Testing"**, running 2026–2029. Participating member states are
Egypt, Ghana, Indonesia, Kenya, Malaysia, Morocco, Norway, and Zimbabwe.
The First Research Coordination Meeting (1st RCM) was held 15–19 December
2025 at IAEA Headquarters in Vienna.

The CRP aims to take nuclear imaging techniques — computed tomography (CT),
laminography, radiotracers — out of the laboratory and into large-scale
industrial deployment, with artificial intelligence woven through image
reconstruction and interpretation. Application targets include refineries,
geothermal power plants, food processing facilities, pipelines, pressure
vessels, and process columns.

### 1.2 Kenya's contribution

Kenya's contribution, led by Dr. Wilson Macharia Kairu of the University of
Nairobi, is titled **"Developing AI-Enabled Multi-Modality CT for On-site
Process Column Inspection"**.

The Kenya workplan has four phases over four years:

- **Year 1 (2026):** System design and early prototyping. Adapt first-generation
  gamma CT (fgen-CT) for reinforced concrete and industrial components.
  Establish baseline datasets and initial reconstruction workflows.
- **Year 2 (2027):** Imaging algorithm development and laminography setup.
  Refine filtered back-projection and iterative reconstruction for
  limited-angle data. Establish industrial laminography configuration for
  non-rotatable components.
- **Year 3 (2028):** AI integration and dataset expansion. Train CNN and
  SRGAN models for automated feature extraction, defect classification,
  and resolution enhancement.
- **Year 4 (2029):** Field validation and industrial pilots.

Kenya's available radiation sources are Am-241 (3.70 GBq), Cs-137 (370 MBq),
and Co-60 (370 MBq). Available imaging hardware includes a first-generation
gamma CT prototype, a portable clampable CT unit, NaI(Tl) scintillation
detectors, a high-purity germanium detector, and a Bruker S2 Picofox TXRF
spectrometer. The software stack the team works in is MATLAB, Python
(Anaconda), TomoPy, ASTRA Toolbox, and OpenCV.

### 1.3 My role

I (Brian Kipruto) am a paid Research Assistant under Dr. Kairu. My
responsibility on this project is the **simulation work** — Monte Carlo
photon transport, synthetic sinogram generation, reconstruction algorithm
testing, and (in later years) synthetic dataset generation for the AI
training pipeline. I do not work on the physical hardware or experimental
measurements directly; the simulation outputs I produce feed into work that
others, including Dr. Kairu and the Kenya MSc students, will then carry into
the lab.

### 1.4 What this project (`cttwin`) is

`cttwin` is the codebase that implements Kenya's simulation contribution.
It is a Geant4 + Python pipeline that, given a phantom geometry and a
radiation source configuration, simulates a full translate-rotate CT scan
and reconstructs the resulting sinogram into a cross-sectional image.

The project takes its name from "CT digital twin" — the goal is a faithful
virtual replica of the Kenyan first-generation gamma CT system, which can
be used to:

- Test reconstruction algorithms against ground truth
- Generate synthetic training data for AI models
- Explore geometry trade-offs (source position, detector resolution, scan
  angles) without consuming beam time on the physical system
- Anchor cross-country collaboration — Indonesia, Egypt, and Ghana are all
  building related systems; a shared phantom simulated by each team is the
  natural basis for inter-comparison

### 1.5 Where in the four-year arc this work sits

The simulation work is **upstream of everything else in the Kenya plan**.
Year 1 of the Kenya plan needs baseline datasets and initial reconstruction
workflows — those are simulation outputs. Year 3 needs trained AI models —
those require synthetic training data, which is a simulation output. Year 4
needs validated reconstruction pipelines — validation requires
ground-truth simulated data.

In other words, if the simulation pipeline doesn't work, the rest of the
Kenya plan doesn't have its foundation. This is why getting `cttwin` right
in Year 1 is non-negotiable.

---

## 2. Scope & success criteria

### 2.1 What `cttwin` must do

In scope:

- Simulate photon transport through industrial phantoms (steel pipes,
  multi-material test objects) using Geant4
- Use Kenya's actually available radiation sources: Cs-137, Co-60, Am-241
- Implement true first-generation translate-rotate CT scan geometry,
  matching the Kenya hardware architecture
- Produce sinograms in a format compatible with TomoPy and ASTRA Toolbox
- Reconstruct via filtered back-projection and iterative methods (SIRT/SART)
- Validate the physics against analytical predictions (Beer-Lambert)
- Be reproducible: a single command should reproduce any published figure
- Be extensible: adding laminography geometry, multi-detector arrays, or new
  phantoms should not require rewriting the core

Out of scope (for the simulator itself; relevant to the wider project):

- Hardware operation, beam-time scheduling, radiation safety procedures
- AI model training (`cttwin` produces the data; training happens elsewhere)
- Reconstruction algorithm research (we use TomoPy and ASTRA off-the-shelf;
  novel reconstruction methods are a possible Year 2–3 extension but not
  the primary deliverable)
- Industrial deployment, field testing, customer engagement

### 2.2 Design questions the simulator must answer

These are the concrete questions that justify the simulator's existence.
Every architectural decision should be evaluated against whether it makes
answering these questions easier or harder.

1. **What scan time is needed to achieve a given image quality on a given
   phantom?** Parameterised by source activity, detector efficiency,
   phantom material and thickness, target SNR.
2. **What spatial resolution is achievable with the existing Kenya hardware
   on each phantom type?** Parameterised by detector pixel size,
   source-to-detector geometry, number of projections.
3. **How does reconstruction quality degrade as the number of projections
   is reduced?** Critical for the SRGAN work in Year 3 — SRGAN's value
   proposition is recovering quality from undersampled scans.
4. **For laminography geometry (Year 2), what tilt angle and angular range
   give the best reconstruction of a flat object of given thickness?**
5. **For each phantom, what is the smallest defect detectable at SNR > 3?**
   This is the metric that ultimately matters for industrial NDT.

### 2.3 Success criteria, by phase

These are the specific, testable conditions under which we can say a phase
is "done". They are deliberately conservative — better to declare Phase 1
done and move on than to keep polishing it.

**Phase 0 — Scoping (this document).** Done when: this guide is reviewed by
Dr. Kairu and his open questions are recorded in the decision log.

**Phase 1 — Validated simulator.** Done when:

- A single Geant4 run with a Cs-137 pencil beam through a slab of known
  thickness reproduces `N = N0 · exp(-μρt)` within 2% relative error for
  four test thicknesses (5, 10, 20, 40 mm of carbon steel)
- The simulator can switch active phantom (pipe / multi-material array) at
  runtime via the messenger
- The simulator can position the source at any (translation, rotation)
  pair via the messenger
- Per-projection output is written to disk in a documented format

**Phase 2 — Working reconstruction pipeline.** Done when:

- A full 180° translate-rotate scan of the multi-material array phantom
  can be run end-to-end without manual intervention
- The resulting sinogram is reconstructed by both TomoPy FBP and ASTRA
  SIRT, producing visually recognisable cross-sections (central bar, six
  outer bars in hexagonal arrangement, baseplate as outer ring)
- The reconstruction is quantitatively scored (RMSE, contrast-to-noise
  ratio) against the known phantom geometry

**Phase 3 — Laminography extension.** Done when:

- The CT geometry can be re-parameterised as a tilted-axis laminography
  scan
- A flat test phantom (steel plate with embedded defects) is reconstructed
  by ASTRA's general 3D geometry support
- The CT vs CL reconstruction trade-off is quantified on the flat phantom

**Phase 4 — Synthetic dataset pipeline.** Done when:

- A pilot dataset of ~500 paired (low-quality, high-quality) reconstructions
  exists, with documented generation parameters
- The dataset is in a format suitable for handing to whoever does the
  SRGAN training in Year 3
- A "data card" documents the generation process, parameter ranges, and
  any known biases

### 2.4 Assumptions

These are things we are taking as given rather than verifying. If any of
them turns out to be wrong, decisions downstream may need revisiting.

- Geant4's `G4EmStandardPhysics_option4` accurately models photon transport
  in the 50 keV – 1.5 MeV range for the materials of interest. (Defensible
  from published validation literature; we will also do the Beer-Lambert
  check in Phase 1.)
- TomoPy and ASTRA Toolbox are production-ready and correct for our scan
  geometries. We are using them off-the-shelf and not validating them
  internally. (Industry-standard tools; this is a reasonable assumption.)
- The Indonesian phantom specification will not change materially. If
  Indonesia revises their phantom dimensions, our Option A is no longer a
  cross-comparison baseline.
- Kenya hardware specifications (NaI detector type, geometry) will not
  change materially during the four-year project. If new detectors are
  procured, the simulator will need updating.

### 2.5 Risks

- **Compute capacity.** A 180-projection scan with 100,000 photons per
  projection is 1.8 × 10⁷ photons total. Generating a 500-image training
  set at that fidelity is 9 × 10⁹ photons. Whether this runs on a single
  workstation in reasonable time is an open question. Mitigation: profile
  early in Phase 1, consider compute resources at the University of Nairobi
  or via the IAEA's compute support if available.
- **Indonesian phantom changes.** Mitigation: maintain a clean separation
  between phantom specs (parameterised) and the rest of the geometry, so a
  spec change is a parameter update rather than a code rewrite.
- **Solo development.** This is currently a one-person codebase (mine).
  Bus-factor of 1 is a project risk. Mitigation: rigorous documentation
  (this document, README, architecture.md), version control on day one,
  consider involving an MSc student in Year 2.
- **Scope creep on Phase 1 validation.** "Validate the simulator" can eat
  arbitrary time. Mitigation: hold Phase 1 to the criteria in §2.3; defer
  more sophisticated validation (NaI detector response, multi-source
  comparison) explicitly to later phases.

---

## 3. Roadmap & work breakdown

### 3.1 The five phases

The project breaks into five phases. Phase 0 is mostly done (this document
is its main deliverable). Phases 1–4 each correspond to one of the success
criteria in §2.3. There is rough mapping to the CRP years, but the phases
are sized by simulation work, not by calendar — so for example, Year 2 of
the CRP encompasses Phase 3 of `cttwin` plus the lab-side work that uses
Phase 2's outputs.

| Phase | Description                          | Target CRP year | Deliverable                                          |
| ----- | ------------------------------------ | --------------- | ---------------------------------------------------- |
| 0     | Scoping and decision-making          | Year 1, Q1      | This document                                        |
| 1     | Validated Geant4 simulator           | Year 1          | A simulator that reproduces Beer-Lambert             |
| 2     | End-to-end CT pipeline               | Year 1          | A reconstructed cross-section of the multi-mat array |
| 3     | Laminography extension               | Year 2          | A CT vs CL reconstruction comparison on a flat plate |
| 4     | Synthetic dataset pipeline           | Year 3          | ~500-image pilot training set                        |

Beyond Phase 4 the work shifts to the wider Kenya plan: dataset scaling,
AI training, field validation. `cttwin` continues to exist as the data
factory but stops being the primary focus.

### 3.2 Phase 1 — Pass-by-pass breakdown

Phase 1 is decomposed into five passes. Each pass leaves the code
compiling, runnable, and verified at a defined checkpoint. Resist the urge
to skip checkpoints.

**Pass 0 — Project skeleton and namespace cleanup.**
- Create directory layout, CMake config, gitignore, licence, README, this
  guide. *Done — see commit history.*
- Standardise on `namespace CTTwin` across all classes (v1 had a
  mismatched `B1` namespace partially applied).

**Pass 1 — Sensitive detector + per-pixel scoring.**
- New files: `SensitiveDetector.hh` / `.cc`.
- Modify `DetectorConstruction.cc` to add a flat detector volume opposite
  the source.
- Score photon crossings per pixel (idealised, no energy response yet).
- Checkpoint: fire 10,000 photons in empty world, confirm all hit the
  expected detector pixel.

**Pass 2 — Pencil beam source.**
- Modify `PrimaryGeneratorAction.cc`: replace cone with a collimated
  pencil beam aimed at the active detector pixel.
- Checkpoint: with empty world, every photon hits the same pixel. With
  phantom in place, transmitted count drops by an attenuation factor.

**Pass 3 — Cs-137 + explicit physics list + Beer-Lambert validation.**
- Modify `PrimaryGeneratorAction.cc`: switch from Ir-192 placeholder to
  single-line Cs-137 at 662 keV.
- Modify `cttwin.cc`: set `G4EmStandardPhysics_option4` explicitly.
- New file: `python/validate_beer_lambert.py`.
- Checkpoint: 2% agreement with `N = N0 · exp(-μρt)` for 5, 10, 20, 40 mm
  carbon steel slabs at 662 keV using NIST XCOM coefficient
  `μ/ρ = 0.07375 cm²/g`.
- This is the pass that earns a defensible TRL 3 claim.

**Pass 4 — UI messenger for runtime geometry control.**
- New files: `DetectorMessenger.hh` / `.cc`, `PrimaryGeneratorMessenger.hh` / `.cc`.
- Expose commands: `/cttwin/phantom select`, `/cttwin/source setAngle`,
  `/cttwin/source setTranslation`, `/cttwin/output setFile`.
- Checkpoint: rotate the phantom 360° in 10° steps via macro, visually
  confirm projection through asymmetric Option B phantom changes
  plausibly.

**Pass 5 — Python driver + sinogram + reconstruction.**
- New files: `python/run_scan.py`, `python/assemble_sinogram.py`,
  `python/reconstruct.py`.
- Sweep angles and translations, generate macros, run Geant4, collect
  CSVs, assemble sinogram, reconstruct.
- Checkpoint: end-to-end reconstruction of multi-material array phantom
  is visually recognisable.

### 3.3 Phase 2 onward — outline only

Detailed pass-level decomposition will happen at the start of each phase,
not now. The shape:

**Phase 2 — End-to-end pipeline.** Phase 1 produces a working simulator and
a working reconstruction, but they are stitched together by hand. Phase 2
makes the pipeline robust, parameterised, and reproducible. Likely passes:
sweep automation across phantoms and sources, output format standardisation,
quantitative reconstruction metrics, the first full validation report
including reconstruction RMSE and CNR.

**Phase 3 — Laminography extension.** ASTRA Toolbox's general 3D geometry
(`parallel3d_vec`) supports arbitrary source-detector trajectories,
including tilted-axis. The simulator needs a geometry parameterisation
that can express both CT and CL configurations, and the Python driver
needs to know how to sweep CL parameters (tilt angle, in-plane angle).
This is the phase where Kenya's contribution becomes most distinctive
relative to other CRP participants.

**Phase 4 — Synthetic dataset pipeline.** The bottleneck for AI training in
Year 3 will be data. Phase 4 builds the infrastructure to generate paired
training examples at scale, with parameterised phantom variation and noise
models calibrated against the Phase 1 Monte Carlo data.

### 3.4 Collaboration touchpoints

Other CRP countries are doing related work. Where our work intersects
theirs, we should coordinate; where it doesn't, we should respect that
their priorities differ from ours.

- **Indonesia (BRIN, geothermal pipes).** Direct overlap. Their phantom is
  ours (Option A). They are using PHITS rather than Geant4; cross-toolkit
  validation on a shared phantom would be a genuine collaboration win. The
  Indonesian work plan explicitly mentions Monte Carlo simulations in
  2026 and AI-assisted reconstruction in 2027.
- **Egypt (gamma + neutron CT, AI for 3D reconstruction).** Adjacent work.
  Their AI techniques (CNN, GAN, transformer-based) align with Kenya's
  Year 3 plans. Shared dataset formats would help both teams.
- **Ghana (industrial CT, AI-enhanced reconstruction).** Similar 1st-gen
  CT hardware to Kenya (Ludlum detector, NaI, Cs-137 source). Cross-team
  validation of reconstruction quality on a shared phantom would be useful.
- **Norway, Malaysia, Morocco, Zimbabwe.** Less direct overlap. Track at
  RCMs but no specific collaboration planned.

---

## 4. Technical specifications

### 4.1 Architecture (three-tier)

```
Tier 1 — Geant4 simulator (C++)
   |
   | per-projection CSV: angle, translation, pixel, n_counts
   v
Tier 2 — Python driver
   |
   | sinogram: NumPy array (n_angles, n_pixels)
   v
Tier 3 — Post-processing (reconstruction, validation)
```

The tiers are deliberately decoupled. The Geant4 simulator knows nothing
about scans, sinograms, or reconstruction — it is told "fire N photons
from position (x, y, z) towards direction (dx, dy, dz) with the phantom
rotated by angle θ, count photons crossing the detector, write to file F"
and that is all. The Python driver orchestrates many such invocations.
The post-processing tier consumes the outputs.

This separation matters because it makes each tier independently
testable, independently parallelisable, and independently replaceable. If
we ever want to swap Geant4 for PHITS (to match Indonesia), only Tier 1
changes. If we ever want to swap TomoPy for a custom reconstruction
algorithm, only Tier 3 changes.

### 4.2 Coordinate conventions

All geometry uses Geant4's coordinate system: right-handed, units in mm
unless otherwise stated.

- **+x:** the axis along which the source-detector line lies at zero
  rotation. Source on the negative side, detector on the positive side.
- **+y:** perpendicular to the source-detector line, in the scan plane.
  Translation steps are along this axis.
- **+z:** the cylinder axis of the pipe phantom; also the rotation axis.

Rotation angle `θ` is the angle of the phantom about +z, measured
counterclockwise from +x as viewed from +z looking back to the origin.
At `θ = 0` the phantom is in its "neutral" orientation. A full scan
covers `θ ∈ [0°, 180°)` in even steps; for parallel-beam geometry
180° is sufficient because projections from `θ` and `θ + 180°` carry the
same information.

Translation `t` is the offset of the source-detector pair along +y. At
`t = 0` the line passes through the rotation axis (origin). The
translation range is `[-r, +r]` where `r` is the phantom's radius plus a
small margin.

### 4.3 Source spectrum and geometry

Pass 3 onwards: Cs-137 at 662 keV, single line. Real Cs-137 also emits
32 keV X-rays from the Ba-137m daughter, but these are absorbed in any
realistic source housing and are not modelled.

The source emits a collimated pencil beam — `SetParticleMomentumDirection`
is set to `(1, 0, 0)` in the source's local frame, with no angular
spread. This idealises away beam divergence; for a real collimated
Cs-137 source the divergence is small (< 1°) and can be added later if
needed.

Future passes: Co-60 (1173 + 1332 keV) and Am-241 (59.5 keV) will be
added in later passes. The source selection will become a runtime choice
via the messenger.

### 4.4 Detector model

Pass 1 onwards: idealised photon counter. A thin scoring surface
(0.1 mm thick, transparent to photons in practice — we score crossings,
not energy deposition) is placed at `x = +source_to_detector_distance`.
The surface is divided into pixels along y; each pixel records the
count of photons that crossed it.

For Pass 1 of the translate-rotate geometry, only one pixel is active —
the one in line with the source. This matches first-generation hardware.
Later passes may add multiple pixels for parallel acquisition.

Idealised counting is justified in §2.4 (assumptions) and §3.2 (Pass 1
decision). A realistic NaI(Tl) detector model — Compton continuum,
energy resolution, efficiency curve — is a deferred extension, not part
of the Phase 1 deliverable.

### 4.5 Output format

Per-projection CSV file with one header line and one data line:

```
projection_id,angle_deg,translation_mm,pixel_index,n_counts
0,0.0,0.0,0,84217
```

Many such files are produced by a single scan (one per (angle,
translation) pair), all in `data/raw/`. The Python driver consumes them
and assembles a 2D NumPy array of shape `(n_angles, n_pixels)` saved as
a single `.npy` file in `data/sinograms/`.

CSV is chosen over ROOT or HDF5 for simplicity, portability, and
human-debuggability. If file count becomes unwieldy (> 10⁴ files per
scan) we may switch to HDF5 with one dataset per scan.

### 4.6 Validation: Beer-Lambert

The fundamental validation test, performed in Pass 3.

For a monoenergetic pencil beam of `N₀` photons passing through a
homogeneous slab of thickness `t`, density `ρ`, and mass attenuation
coefficient `μ/ρ` at the photon energy, the number of transmitted
photons (ignoring scatter — defensible for a thin pencil beam and small
detector) is:

```
N = N₀ · exp( -(μ/ρ) · ρ · t )
```

For Cs-137 at 662 keV through carbon steel:

| Quantity                       | Value          | Source              |
| ------------------------------ | -------------- | ------------------- |
| μ/ρ (Fe at 662 keV)            | 0.07375 cm²/g  | NIST XCOM           |
| ρ (carbon steel, 99% Fe + 1% C)| 7.85 g/cm³     | ASM Handbook        |
| μ                              | 0.5789 /cm     | computed            |
| half-value layer (HVL)         | 11.97 mm       | ln(2) / μ           |

Acceptance criterion: simulation agrees with prediction within 2%
relative error for `t ∈ {5, 10, 20, 40}` mm.

### 4.7 Reconstruction

Two methods, both off-the-shelf:

- **TomoPy `gridrec`:** filtered back-projection with the gridding
  algorithm. Fast, good baseline, well-supported.
- **ASTRA Toolbox SIRT (Simultaneous Iterative Reconstruction Technique):**
  iterative method, slower but better noise behaviour and more flexible
  geometry support. Required anyway for Phase 3 (laminography geometry).

Both consume the sinogram as `(n_angles, n_pixels)`, both produce a
square reconstructed slice. Reconstruction parameters (centre of
rotation, filter choice, iteration count for SIRT) live in
`python/reconstruct.py` as command-line arguments.

### 4.8 Decision log

Decisions taken so far. New decisions append below. Each entry: date,
decision, rationale.

- **2026-05-12 — Source for Phase 1: Cs-137 (662 keV).** Single dominant
  line makes Beer-Lambert validation unambiguous. Matches Ghana and
  Indonesia's primary sources, so cross-country phantom inter-comparison
  is on common ground. Kenya has 370 MBq of Cs-137 available. Co-60 and
  Am-241 deferred to later passes.
- **2026-05-12 — Detector model for Phase 1: idealised photon counting.**
  Validation against Beer-Lambert is unambiguous with ideal counting; NaI
  energy deposition would conflate transport-physics errors with detector
  response errors. NaI realism is a Pass 6+ extension.
- **2026-05-12 — Scan geometry: true 1st-gen translate-rotate.** Matches
  Kenya's existing portable clampable CT hardware. Fan-beam is a later
  optimisation if it becomes necessary.
- **2026-05-12 — Namespace: `CTTwin`.** Project-specific, replaces the
  inherited `B1` from the original Geant4 example.
- **2026-05-12 — Repository name: `iaea-crp-f22086-kenya`.** Discoverable,
  attributes the country, ties to the CRP code.
- **2026-05-12 — Licence: Apache 2.0.** Compatible with Geant4's licence
  family. Permissive but requires attribution.

### 4.9 Open questions (need lecturer input)

These are flagged so they don't get lost. Resolve and migrate to the
decision log as they are answered.

- **Source-to-detector distance.** What does the Kenya portable clampable
  CT actually use? The simulation should match if at all possible.
- **Detector pixel size.** The Ludlum 2×2" NaI has a 50.8 mm × 50.8 mm
  face. For the simulator, are we modelling 1 pixel = full detector face,
  or sub-pixelating?
- **Phantom rotation vs source rotation.** Physically equivalent, but one
  is easier to implement via the messenger than the other. Likely:
  rotate the phantom, keep the source-detector pair fixed in world
  coordinates.
- **Indonesian phantom revisions.** Has the Indonesian team revised their
  phantom spec since the 1st RCM? Worth a check before deep work on
  Option A.
- **Compute budget.** What compute do we have access to at UoN? Is a
  single workstation sufficient, or do we need cluster access?
- **GitHub repo visibility.** Private during development; when does it
  open up? Tied to IAEA publication norms — needs lecturer call.
- **Publication strategy.** When and where do we publish Phase 1 results?
  Conference vs journal, which venue, target submission date.

---

## 5. Current status & next actions

> **This section is updated frequently. If it's stale, that's a bug.**

### 5.1 As of 2026-05-12

**Completed:**

- Reviewed the IAEA CRP F22086 1st RCM materials
- Reviewed v1 of the digital twin (Geant4 phantom geometry only) and the
  Phantom_Digitization.pdf presentation
- Identified gaps in v1: no detector, no pencil beam, wrong source
  (Ir-192 placeholder), wrong physics list (FTFP_BERT default),
  no scan orchestration, no reconstruction
- Established the five-phase, five-pass roadmap (this document, §3)
- Created the project skeleton: directory structure, CMake configuration,
  README, gitignore, placeholder source files
- Drafted the supporting documentation: this guide, `architecture.md`,
  `phantom_specs.md`, `validation_report.md`

**In progress:**

- This document.

**Not started:**

- GitHub repository creation and initial push
- Pass 1 (sensitive detector implementation)
- Conversation with Dr. Kairu about the v1 → v2 plan

### 5.2 Next actions

In rough order of priority. Tick off as completed.

- [ ] Brian: review this document end-to-end, flag anything inaccurate or
      missing
- [ ] Brian: review the open questions in §4.9, write down preliminary
      answers, then schedule a conversation with Dr. Kairu
- [ ] Brian: create the GitHub repository, push the skeleton
- [ ] Brian: walk Dr. Kairu through this document and the skeleton; get
      explicit sign-off on the Phase 1 plan before writing more code
- [ ] Brian (with AI assistant): Pass 1 implementation — sensitive
      detector and per-pixel scoring
- [ ] Brian (with AI assistant): Pass 2 implementation — pencil beam
- [ ] Brian (with AI assistant): Pass 3 implementation — Cs-137, physics
      list, Beer-Lambert validation. **Phase 1 done after this pass.**

---

## 6. Resume instructions

> **If you are an AI assistant picking up this project in a new chat —
> read this section carefully. It is written specifically for you.**

### 6.1 Who you are talking to

The user is **Brian Kipruto**, a paid Research Assistant at the University
of Nairobi, working under **Dr. Wilson Macharia Kairu** on Kenya's
contribution to IAEA CRP F22086. Brian's responsibility is the
simulation work — Geant4 Monte Carlo, Python reconstruction, eventual AI
training data.

Brian is technically capable but not a Geant4 expert; he started from the
B1 example and adapted it. He thinks carefully and pushes back when
something doesn't feel right, which is good — engage with the pushback
rather than capitulating. He values being told when something he's done
is incorrect, as long as the criticism is specific and constructive.

### 6.2 What state the project is in

Read §5 (Current status & next actions) — that is the ground truth as of
last update. Verify the date at the top of this document; if it's old,
ask Brian what's changed.

The codebase lives at `https://github.com/[brian-username]/iaea-crp-f22086-kenya`
(or whatever repo name was actually chosen — check with Brian). The first
thing to do, before any technical conversation, is:

1. Ask Brian where the repo is hosted and request the URL
2. Ask Brian to share the latest version of the codebase (he can paste
   files, or you can browse if the repo is public)
3. Ask Brian what pass he is on
4. Read this document end-to-end before suggesting anything

### 6.3 What you should not do

- Do not invent technical decisions. If the decision log doesn't have it,
  ask Brian or recommend the decision but make clear it's a new decision.
- Do not skip passes. The roadmap is broken into small passes for a
  reason — each one is independently testable.
- Do not pad. Brian dislikes verbose output. Get to the point.
- Do not assume what hardware Kenya has. Read §1.2 of this document.
- Do not invent file paths, function names, or library APIs. If you need
  to know something, ask or read the actual code.
- Do not claim TRL levels without validation evidence. TRL 3 specifically
  requires demonstrated functional simulation, not just verified
  geometry.
- Do not generate marketing-style language, motivational filler, or
  excessive emoji. The audience is a research engineer and his
  supervisor.

### 6.4 What you should do

- Read §1 to understand the project context
- Read §2 to understand what counts as success
- Read §3 to understand where in the roadmap Brian is
- Read §4 to understand the technical conventions
- Read §5 to understand the current live state
- Ask sharp questions when you don't have context, instead of guessing
- When suggesting code, suggest small increments that compile and run,
  not large rewrites
- Update §4.8 (Decision log) and §5 (Current status) when material
  decisions are made
- Treat this document as authoritative. If it says one thing and Brian
  says another, ask which is correct rather than assuming.

### 6.5 What to do in the first turn of a resumed conversation

A good opener for a resumed conversation, if Brian shares this document
with you and says "let's continue the project":

> I've read the project guide. Before we go further, let me confirm
> where we are: you're at [pass X of phase Y], the last decision logged
> was [decision], and the next action is [action]. Is that still
> accurate, or has something changed since the document was last
> updated?

That establishes shared context in one turn and gives Brian a chance to
correct anything stale.

### 6.6 What this document is not

This document is the project's strategic anchor. It is *not* a substitute
for:

- The actual codebase. Read the code when discussing implementation.
- The CRP documentation. Read the 1st RCM final report for the full CRP
  context.
- Conversations with Dr. Kairu. He is the CSI and his decisions override
  anything in this document.
