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

// ─── CTTWIN START: Pass 3 flat-slab validation phantom ───
// The Beer-Lambert test geometry: a flat carbon-steel slab centred on the
// rotation axis, thickness along the beam axis (+x), so the on-axis pencil ray
// traverses EXACTLY the nominal thickness. This is why the slab exists and why
// the curved pipe wall is not used — path length through a curved wall varies
// with position and would smear the exponential. See [[Beer-Lambert Validation]].
//
// Lateral extent (y,z) is generous relative to the 50.8 mm detector face so the
// beam never sees a slab edge and no un-attenuated path exists around it. It is
// NOT a physically meaningful dimension; only the thickness enters the physics.
constexpr G4double kSlabLateral          = 100.0 * mm;  // full width in y and z
constexpr G4double kDefaultSlabThickness =  10.0 * mm;  // overridden by /cttwin/slabThickness
// ─── CTTWIN END ───

// ─── CTTWIN START: Pass 4 scan motion ───
// The scan coordinates of a first-generation translate-rotate CT: a rotation
// angle theta and a translation offset t. See [[Coordinate Conventions]].
//
// IMPLEMENTATION NOTE (ADR 0005): the source and detector NEVER move. The
// phantom carries the whole scan transform instead. The two are exactly, not
// approximately, equivalent here because the detector volume is G4_AIR sitting
// in a G4_AIR world — it is invisible to photon transport, so "small detector
// that translates" and "detector that sits still while the phantom slides past"
// are the same transport problem. Consequences: the gun stays at
// (-kSourceToIso, 0, 0) aimed +x forever, the detector placement is fixed, the
// beam always lands on the in-line pixel, and the Pass 1-3 regression anchors
// are unaffected at theta = 0, t = 0.
//
// The world transform applied to a phantom point p (expressed in the phantom's
// own frame, i.e. its position at theta = 0) is
//
//     p_world = R_z(theta) * p  -  t * y_hat
//
// which is the canonical picture (phantom rotated by theta, rig translated to
// y = +t) rigidly shifted by -t*y_hat so the rig can stay at y = 0.
constexpr G4double kDefaultScanAngle       =   0.0 * deg;
constexpr G4double kDefaultScanTranslation =   0.0 * mm;

// Guard on /cttwin/scan/translation. The largest phantom (Option B baseplate)
// has a 100 mm radius, so at |t| = 150 mm it still sits 250 mm clear of the
// 500 mm world half-extent and nowhere near the detector at x = +250 mm. This
// is a sanity bound, not a physical scan range — the useful range is
// [-r, +r] with r = phantom radius + margin.
constexpr G4double kMaxScanTranslation     = 150.0 * mm;
// ─── CTTWIN END ───

}  // namespace CTTwin::Geometry


namespace CTTwin::Physics {

// ─── CTTWIN START: Pass 3 Cs-137 source (LOCKED) ───
// Architecture Lockdown #1: Cs-137, single gamma line. Pass 2 set 662 keV as a
// working value; Pass 3 formalises it as the locked source definition and it is
// now referenced from here, not written as a literal anywhere else.
//
// 661.657 keV is the evaluated energy of the Ba-137m -> Ba-137 transition
// (the "662 keV" of the literature). The exact value is used because the
// validation compares against mu/rho evaluated at the SAME energy — the scar
// on the carry-forward list is "keep energy <-> mu/rho in sync", and syncing
// against a rounded energy is the easy way to break that quietly.
//
// The ~32 keV Ba-137m X-rays are deliberately NOT modelled: absorbed in any
// realistic source housing. See [[Source - Cs-137]] and [[Open Questions]].
constexpr G4double kCs137GammaEnergy = 661.657 * keV;

// Tolerances used by SensitiveDetector to identify an UNSCATTERED primary —
// a photon that reached the detector without interacting at all.
// A photon that has never interacted retains its launch direction and energy
// exactly (transportation alters neither), so these bounds are far looser than
// they need to be and exist only to avoid floating-point equality.
//   * Compton scattering changes both direction and energy.
//   * Rayleigh scattering is ELASTIC — it changes direction but NOT energy,
//     which is why the direction gate is present and the energy gate alone
//     would be incomplete.
// See ADR 0004.
constexpr G4double kUnscatteredEnergyTol  = 1.0 * eV;   // |E - E0| below this
constexpr G4double kUnscatteredCosTol     = 1.0e-9;     // 1 - dir.x() below this
// ─── CTTWIN END ───

}  // namespace CTTwin::Physics

#endif
