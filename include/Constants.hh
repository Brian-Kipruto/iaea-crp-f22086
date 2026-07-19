#ifndef CTTWIN_CONSTANTS_HH
#define CTTWIN_CONSTANTS_HH 1

#include "G4SystemOfUnits.hh"

// =============================================================================
// CTTwin — project-wide constants.
// =============================================================================

namespace CTTwin::Geometry {

// ─── CTTWIN START: Pass 0 placeholder geometry ───
// PLACEHOLDER — NOT hardware-derived.
//
// Open Question: "Source-to-detector distance. What does the Kenya portable
// clampable CT actually use?" (00 - Overview/Open Questions.md)
//   Owner:    Dr. Kairu
//   Basis:    kSourceToIso carried from v1 (source at x = -150 mm).
//             kIsoToDetector ASSUMED symmetric — no hardware basis. Real
//             portable clamp rigs are often source-close/detector-far.
//   Blocking: Pass 3 Beer-Lambert numbers are quoted against THIS geometry.
//             If Dr. Kairu supplies a real SDD, these change and Pass 3 must
//             be re-run. Deadline for the answer: before Pass 3.
// See Architecture Lockdown decision #10 (2026-07-19).

constexpr G4double kSourceToIso    = 150.0 * mm;   // source -> rotation axis
constexpr G4double kIsoToDetector  = 150.0 * mm;   // rotation axis -> detector (assumed symmetric)
constexpr G4double kSourceToDetector = kSourceToIso + kIsoToDetector;  // 300 mm

constexpr G4double kWorldHalfSize  = 500.0 * mm;   // world half-extent (v1)
// ─── CTTWIN END ───

// ─── CTTWIN START: Pass 1 detector geometry ───
// Idealised photon counter — a thin AIR box the transmitted beam lands on.
// Air, not scintillator: it registers arrival without attenuating. Real NaI
// response (Ludlum 2x2", ~7% resolution at 662 keV) is deferred to Pass 6+.
// See Architecture Lockdown #2 and [[Detector Model]].
//
// PLACEHOLDER dimensions — face matches the eventual 2" NaI so the geometry
// carries over; thickness is minimal (a counter shouldn't perturb transport).
// Pixelation is OPEN (one pixel = whole face for Pass 1). See [[Open Questions]].
constexpr G4double kDetectorFace      = 50.8 * mm;   // 2" square face
constexpr G4double kDetectorThickness =  1.0 * mm;   // along the beam axis (thin)
// ─── CTTWIN END ───

}  // namespace CTTwin::Geometry

#endif
