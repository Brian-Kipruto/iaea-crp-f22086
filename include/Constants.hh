#ifndef CTTWIN_CONSTANTS_HH
#define CTTWIN_CONSTANTS_HH 1

#include "G4SystemOfUnits.hh"

// =============================================================================
// CTTwin — project-wide constants.
// =============================================================================

namespace CTTwin::Geometry {

// ─── CTTWIN START: scan geometry (confirmed 2026-07-20) ───
//
// Scan geometry — CONFIRMED against hardware, 2026-07-20.
//   Source:   Dr. Kairu, on the NDT-lab gamma column scanner.
//   Answer:   SDD approximately 0.5 m, source and detector SYMMETRIC about the
//             object. Rig is adjustable, but I'll mirror the hardware as stated —
//             the point of a digital twin is that it matches the real rig.
//   Supersedes the symmetric 150/150 mm placeholder carried from v1.
// See Architecture Lockdown decision #10 (amended 2026-07-20) and ADR 0003
// (which supersedes ADR 0002).

constexpr G4double kSourceToIso    = 250.0 * mm;   // source -> rotation axis
constexpr G4double kIsoToDetector  = 250.0 * mm;   // rotation axis -> detector (symmetric, confirmed)
constexpr G4double kSourceToDetector = kSourceToIso + kIsoToDetector;  // 500 mm SDD

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
