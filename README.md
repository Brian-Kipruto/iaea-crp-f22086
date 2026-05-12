# cttwin — first-generation gamma CT simulator

A Geant4 + Python pipeline for simulating first-generation gamma computed
tomography of industrial components.

Developed at the University of Nairobi, Institute of Nuclear Science &
Technology, as Kenya's simulation contribution to
**IAEA CRP F22086 — Advanced Nuclear Imaging Techniques for Industrial
Process Analysis and Component Testing**.

## What it does

Given a phantom geometry (a steel pipe, a multi-material test object) and
a radiation source (Cs-137 at 662 keV in v2; more sources later),
`cttwin` simulates a full translate-rotate CT scan, writes per-projection
photon counts to disk, and reconstructs the resulting sinogram into a
cross-sectional image using FBP and iterative methods.

The simulator is the upstream end of a longer pipeline that, in later
phases of the CRP, will feed AI models (CNN, SRGAN) for image
enhancement and defect classification.

## Current status

| Phase           | Component                          | Status         |
| --------------- | ---------------------------------- | -------------- |
| Phase 1, Pass 1 | Sensitive detector + pixel scoring | Not yet built  |
| Phase 1, Pass 2 | Pencil beam source                 | Not yet built  |
| Phase 1, Pass 3 | Cs-137 + physics list + validation | Not yet built  |
| Phase 1, Pass 4 | UI messenger for geometry control  | Not yet built  |
| Phase 2, Pass 5 | Python driver + reconstruction     | Not yet built  |

See `docs/architecture.md` for what each pass involves and
`docs/validation_report.md` for validation results as they are recorded.

## Repository layout

```
include/    C++ headers
src/        C++ implementations
macros/     Geant4 macro files (visualisation, scan templates)
python/     Driver scripts and post-processing
data/       Simulation outputs (gitignored)
docs/       Project documentation (architecture, phantoms, validation)
```

## Building

Requires Geant4 11.x (built with `-DGEANT4_USE_QT=ON -DGEANT4_USE_OPENGL_X11=ON`
for the visualisation driver) and CMake 3.16 or newer.

```bash
mkdir build && cd build
cmake ..
make -jN          # N = number of cores
./cttwin          # interactive (visualisation)
./cttwin run.mac  # batch mode
```

If you don't already have a Geant4 environment set up, source the
Geant4 install's environment script first:

```bash
source /path/to/geant4-install/bin/geant4.sh
```

## Running a scan

Once the Python tier is built (Pass 5), the typical workflow is:

```bash
# From the repo root, with a Python environment that has tomopy installed:
python python/run_scan.py --phantom bars --angles 180 --photons 100000
python python/assemble_sinogram.py
python python/reconstruct.py
```

Outputs land in `data/raw/`, `data/sinograms/`, and `data/reconstructions/`
respectively.

## Project context

This work is part of **IAEA CRP F22086**, a four-year coordinated research
project running 2026–2029. Participating member states are Egypt, Ghana,
Indonesia, Kenya, Malaysia, Morocco, Norway, and Zimbabwe. The Kenya
contribution focuses on advancing first-generation gamma CT for
reinforced concrete and industrial components, with later integration of
AI-driven reconstruction (SRGAN) and limited-angle laminography for flat
components.

The CSI (Chief Scientific Investigator) for the Kenya team is
Dr. Wilson Macharia Kairu.

## Licence

Apache 2.0. See `LICENSE`.

## Citing this work

If you use `cttwin` in academic work, please cite the IAEA CRP F22086
final report and acknowledge the University of Nairobi, Institute of
Nuclear Science & Technology. A formal citation entry will be added here
once the first journal output is published.

## Contributing

This is a research codebase developed under an active CRP. External
contributions are welcome but should be discussed first — open an issue
describing what you would like to contribute, or contact the maintainers
directly.

## Contact

Maintained by Brian Kipruto under the supervision of Dr. Wilson Macharia
Kairu, University of Nairobi.
