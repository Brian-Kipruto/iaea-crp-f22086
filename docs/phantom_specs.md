# Phantom specifications

This document is the authoritative specification for the phantoms modelled
in `cttwin`. It supersedes the earlier `Phantom_Digitization.pdf` and
corrects the inconsistencies present in that document (see the closing
section for the change record).

The phantoms originate from the design package shared by the Indonesian
team (BRIN) at the First Research Coordination Meeting in Vienna,
December 2025. The Kenya team selected the configurations below from the
options offered.

## Option A — Industrial baseline pipe

A short section of standard industrial steel pipe, intended as a
geometrically simple baseline for attenuation studies and as a stand-in
for the geothermal piping that Indonesia targets in their part of the CRP.

| Property              | Value         |
| --------------------- | ------------- |
| Standard              | 5" NPS SCH 40 |
| Outer diameter        | 141.3 mm      |
| Wall thickness        | 6.55 mm       |
| Inner diameter        | 128.2 mm      |
| Section height        | 150 mm        |
| Material              | Carbon steel  |
| Density               | 7.85 g/cm^3   |
| Composition (by mass) | Fe 99%, C 1%  |

The Indonesian specification offered three NPS sizes (5", 6", 8") at two
schedules (SCH 10, SCH 40). Kenya selected 5" SCH 40 as the baseline
configuration. Other sizes can be added by changing `pipeOD` and
`pipeWall` in `DetectorConstruction.cc`.

## Option B — Multi-material validation phantom

A hexagonal arrangement of seven bars on an aluminium baseplate, designed
to test material discrimination across a range of effective atomic
numbers.

### Baseplate

| Property | Value     |
| -------- | --------- |
| Shape    | Disk      |
| Diameter | 200 mm    |
| Thickness | 10 mm    |
| Material | Aluminium |

### Central bar

| Property | Value     |
| -------- | --------- |
| Diameter | 40 mm     |
| Height   | 150 mm    |
| Material | Aluminium |
| Z        | 13        |

### Outer bars (hexagonal ring at radius 60 mm from centre)

Six bars alternating between two materials:

| Position    | Diameter | Material         | Z (or Z_eff) | Density (g/cm^3) |
| ----------- | -------- | ---------------- | ------------ | ---------------- |
| 0, 120, 240 | 20 mm    | Carbon steel     | ~26          | 7.85             |
| 60, 180, 300 | 30 mm   | Polyethylene     | ~5.5         | 0.94             |

### Geometric note: the z-gap

The bars are offset 0.01 mm above the baseplate to ensure Geant4 treats
the bar-baseplate interface as two distinct volume boundaries. This
0.01 mm gap is far below the physical resolution of any modelled imaging
system and has no effect on simulated attenuation, but it is required
for the navigator to function correctly. The geometry overlap check
(`/geometry/test/run`) passes with zero overlaps under this configuration.

## Materials

All materials are referenced from the Geant4 NIST database
(`G4NistManager`) for cross-section reliability, with the exception of
carbon steel which is custom-defined to match the 99% Fe / 1% C
composition typical of industrial low-carbon steels.

| Material name in code | Source            | Notes                       |
| --------------------- | ----------------- | --------------------------- |
| `G4_AIR`              | NIST              | World fill                  |
| `G4_Al`               | NIST              | Aluminium components        |
| `G4_POLYETHYLENE`     | NIST              | Polyethylene bars           |
| `CarbonSteel`         | Custom            | 99% Fe + 1% C, 7.85 g/cm^3  |

## Visualisation

The two phantoms can be displayed side by side for visual inspection (Option A
at +160 mm in x, Option B at -160 mm in x). For an actual scan, only one
phantom is placed at a time, centred on the rotation axis. The selection is
made at runtime via the `/cttwin/phantom select pipe|bars` command.

## Change record relative to Phantom_Digitization.pdf

The earlier presentation document contained the following issues, which
have been corrected here:

- **Pipe outer diameter:** the document body stated 180 mm; the
  specification table and the code state 141.3 mm. The correct value is
  141.3 mm (5" NPS SCH 40).
- **TRL claim:** the document claimed TRL 3. Without a validated source
  and detector, the project was in TRL 2 (technology concept formulated)
  with geometry verification supporting an *approach* to TRL 3. The full
  TRL 3 claim is now defensible once Beer-Lambert validation is complete
  (Pass 3).
- **"Indonesian pipe phantom":** the Indonesian specification offers
  multiple sizes; the document framing implied a single canonical choice.
  Corrected to specify "5" NPS SCH 40 selected from the Indonesian
  specification."
- **"Contrast Resolution Phantom" acronym:** removed. The term is
  medical-imaging usage and conflicts with the project's use of "CRP"
  for Coordinated Research Project.
