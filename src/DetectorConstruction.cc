#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "Constants.hh"
#include "SensitiveDetector.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SDManager.hh"

#include <cmath>

namespace CTTwin
{

// -----------------------------------------------------------------------------
// ─── CTTWIN START: Pass 3 messenger ownership ───
DetectorConstruction::DetectorConstruction()
{
  // ─── CTTWIN START: Pass 4 shared phantom rotation ───
  // Created before the messenger, because a /cttwin/scan/... command issued
  // during macro parsing will write through it.
  fPhantomFrameRotation = new G4RotationMatrix();
  UpdateFrameRotation();
  // ─── CTTWIN END ───

  fMessenger = new DetectorMessenger(this);
}

DetectorConstruction::~DetectorConstruction()
{
  delete fMessenger;
  // Placements hold this pointer but do not own it; the geometry is being torn
  // down alongside us, so releasing it here is safe.
  delete fPhantomFrameRotation;
}
// ─── CTTWIN END ───

// -----------------------------------------------------------------------------
// ─── CTTWIN START: Pass 4 scan transform ───
//
// The scan coordinates live on the PHANTOM, not on the source-detector pair
// (ADR 0005). For a phantom point p expressed in the phantom's own frame:
//
//     p_world = R_z(theta) * p  -  t * y_hat
//
// The first term is the canonical rotation of Coordinate Conventions (theta
// about +z, counterclockwise seen from +z). The second is the whole system
// rigidly shifted by -t so that the rig can stay at y = 0 instead of moving to
// y = +t. Relative geometry — which is all the line integral sees — is
// identical to the canonical picture.
//
// Sign warning: with the current phantom set this convention is only pinned
// down up to a SIMULTANEOUS flip of both signs. Every Phase 1 phantom (pipe,
// bars, slab) happens to be mirror-symmetric about the beam axis, so
// (theta, t) -> (-theta, -t) is an exact symmetry of the object and no
// measurement can distinguish it. The relative sign IS measurable and is what
// checkpoint 4 tests. The absolute convention is fixed here and in
// [[Coordinate Conventions]], and matters from Phase 3 onwards.

void DetectorConstruction::UpdateFrameRotation()
{
  // G4PVPlacement's rotation argument is the rotation of the MOTHER frame with
  // respect to the daughter — the inverse of the rotation you want the object
  // to undergo. Hence -fScanAngle here while WorldPositionOf() below uses
  // +fScanAngle. They are not inconsistent; they are the same rotation
  // expressed in the two conventions Geant4 uses.
  *fPhantomFrameRotation = G4RotationMatrix();
  fPhantomFrameRotation->rotateZ(-fScanAngle);
}

G4ThreeVector DetectorConstruction::WorldPositionOf(const G4ThreeVector& p) const
{
  const G4double c = std::cos(fScanAngle);
  const G4double s = std::sin(fScanAngle);

  return G4ThreeVector(p.x() * c - p.y() * s,
                       p.x() * s + p.y() * c - fScanTranslation,
                       p.z());
}

// An unrotated placement (nullptr) and an identity-rotation placement are
// physically the same thing, but Geant4's navigator reaches them by different
// code paths and the boundary arithmetic need not round identically. The Pass
// 1-3 regression anchors were measured with nullptr, and checkpoint 5 asks them
// to reproduce EXACTLY, so the unrotated case keeps its original path.
G4RotationMatrix* DetectorConstruction::RotationForPlacement() const
{
  return (std::fabs(fScanAngle) < 1.0e-12) ? nullptr : fPhantomFrameRotation;
}

G4VPhysicalVolume* DetectorConstruction::PlacePhantomVolume(
    const G4String& name, G4LogicalVolume* logic,
    const G4ThreeVector& localPos, G4LogicalVolume* world, G4int copyNo)
{
  auto* pv = new G4PVPlacement(RotationForPlacement(),
                               WorldPositionOf(localPos),
                               logic, name, world, false, copyNo, true);
  fPhantomPlacements.push_back({pv, localPos});
  return pv;
}

void DetectorConstruction::ApplyScanTransform()
{
  UpdateFrameRotation();

  // Before /run/initialize there is nothing to move: the placements do not
  // exist yet and Construct() will read the new values when it runs.
  if (fPhantomPlacements.empty()) return;

  for (auto& e : fPhantomPlacements) {
    e.pv->SetTranslation(WorldPositionOf(e.localPos));
    e.pv->SetRotation(RotationForPlacement());
  }

  // Tell the kernel the world changed so it re-optimises the voxel navigation
  // before the next run. Without this the navigator can happily transport
  // photons through a geometry that no longer exists — the silent stale-
  // geometry bug the vault warns about. Checkpoint 6 is the test that this
  // line is doing its job.
  if (auto* rm = G4RunManager::GetRunManager()) {
    rm->GeometryHasBeenModified();
  }

  G4cout << "[CTTwin] scan transform -> theta = " << fScanAngle / deg
         << " deg, t = " << fScanTranslation / mm << " mm" << G4endl;
}

void DetectorConstruction::SetScanAngle(G4double theta)
{
  fScanAngle = theta;
  ApplyScanTransform();
}

void DetectorConstruction::SetScanTranslation(G4double t)
{
  fScanTranslation = t;
  ApplyScanTransform();
}
// ─── CTTWIN END ───

// -----------------------------------------------------------------------------
void DetectorConstruction::DefineMaterials()
{
  G4NistManager* nist = G4NistManager::Instance();
  nist->FindOrBuildMaterial("G4_AIR");
  nist->FindOrBuildMaterial("G4_Al");
  nist->FindOrBuildMaterial("G4_POLYETHYLENE");

  // Custom carbon steel (~99% Fe, ~1% C by mass). Carried verbatim from v1 —
  // the material the Beer-Lambert numbers will be quoted against. See
  // [[Materials & Cross-Sections]] / Architecture Lockdown.
  //
  // Pass 3 note: the analytical reference mu/rho is computed for THIS
  // composition (0.99 x Fe + 0.01 x C by mass), not for pure iron. The
  // difference is small (+0.05% at 662 keV, since carbon's Z/A is higher) but
  // it is computed rather than waved away — see python/xcom_reference.py and
  // ADR 0004. If this composition ever changes, that reference changes with it.
  if (!G4Material::GetMaterial("CarbonSteel", false)) {
    G4Element* elFe = nist->FindOrBuildElement("Fe");
    G4Element* elC  = nist->FindOrBuildElement("C");
    auto* carbonSteel = new G4Material("CarbonSteel", 7.85 * g/cm3, 2);
    carbonSteel->AddElement(elFe, 99.0 * perCent);
    carbonSteel->AddElement(elC,   1.0 * perCent);
  }
}

// -----------------------------------------------------------------------------
G4VPhysicalVolume* DetectorConstruction::Construct()
{
  DefineMaterials();
  G4Material* air = G4Material::GetMaterial("G4_AIR");

  // ─── CTTWIN START: Pass 4 — reset the placement registry ───
  // Construct() can run more than once if the geometry is ever fully rebuilt.
  // Stale G4VPhysicalVolume pointers in this list would be dangling, and
  // ApplyScanTransform() would write through them.
  fPhantomPlacements.clear();
  UpdateFrameRotation();
  // ─── CTTWIN END ───

  // --- World ---
  const G4double w = Geometry::kWorldHalfSize;
  auto* solidWorld = new G4Box("World", w, w, w);
  auto* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
  auto* physWorld  = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld,
                                       "World", nullptr, false, 0, true);

  // --- Active phantom, centred on the rotation axis (origin) ---
  // ─── CTTWIN START: Pass 0 — one active phantom at origin ───
  // v1 placed BOTH phantoms side by side at x = +/-160 mm. Rotation in the
  // CT scan is about the origin, so the active phantom must sit there.
  // Pass 1 adds "none": an empty world for the detector checkpoint and the
  // later open-beam N0 reference. Pass 3 adds "slab".
  //
  // Pass 4: the positions passed to the builders below are PHANTOM-FRAME
  // positions (theta = 0, t = 0). PlacePhantomVolume applies the scan
  // transform on the way out.
  if (fActivePhantom == "none") {
    // no phantom — empty world
  } else if (fActivePhantom == "bars") {
    BuildBarsPhantom(logicWorld);
  } else if (fActivePhantom == "slab") {
    BuildSlabPhantom(logicWorld);
  } else {
    BuildPipePhantom(logicWorld);   // default: Option A
  }
  // ─── CTTWIN END ───

  // ─── CTTWIN START: Pass 1 — detector volume ───
  // NOT registered with the scan transform: the detector never moves. See
  // ADR 0005.
  BuildDetector(logicWorld);
  // ─── CTTWIN END ───

  // NOTE: no fScoringVolume. v1 set fScoringVolume = logicPipe and scored dose
  // in the object. The detector is a separate volume (below), counted by a
  // SensitiveDetector — not dose in the phantom.

  return physWorld;
}

// -----------------------------------------------------------------------------
// Option A — 5" NPS SCH 40 pipe (OD 141.3 mm, wall 6.55 mm), carbon steel.
G4LogicalVolume* DetectorConstruction::BuildPipePhantom(G4LogicalVolume* world)
{
  G4Material* steel = G4Material::GetMaterial("CarbonSteel");

  const G4double pipeHalfHeight = 75.0 * mm;
  const G4double pipeOD   = 141.3 * mm;
  const G4double pipeWall =   6.55 * mm;

  auto* solidPipe = new G4Tubs("SolidPipe", (pipeOD/2) - pipeWall, pipeOD/2,
                               pipeHalfHeight, 0*deg, 360*deg);
  auto* logicPipe = new G4LogicalVolume(solidPipe, steel, "LogicPipe");

  auto* vis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.4));
  vis->SetForceSolid(true);
  logicPipe->SetVisAttributes(vis);

  PlacePhantomVolume("PhysPipe", logicPipe, G4ThreeVector(0, 0, 0), world, 0);
  return logicPipe;
}

// -----------------------------------------------------------------------------
// Option B — aluminium baseplate + central bar + hexagonal ring of
// alternating steel/poly bars. Ported from v1, recentred on the origin.
G4LogicalVolume* DetectorConstruction::BuildBarsPhantom(G4LogicalVolume* world)
{
  G4Material* alum  = G4Material::GetMaterial("G4_Al");
  G4Material* poly  = G4Material::GetMaterial("G4_POLYETHYLENE");
  G4Material* steel = G4Material::GetMaterial("CarbonSteel");

  const G4double pipeHalfHeight = 75.0 * mm;
  const G4double baseHalfThick  =  5.0 * mm;

  // Baseplate centred on the origin in x,y; sits below the bars in z.
  G4ThreeVector basePos(0, 0, -pipeHalfHeight - baseHalfThick);
  auto* solidBase = new G4Tubs("Baseplate", 0, 100*mm, baseHalfThick, 0*deg, 360*deg);
  auto* logicBase = new G4LogicalVolume(solidBase, alum, "LogicBase");
  logicBase->SetVisAttributes(new G4VisAttributes(G4Colour(0.0, 1.0, 1.0)));
  PlacePhantomVolume("PhysBase", logicBase, basePos, world, 0);

  // Bars sit on top of the baseplate. The 0.01 mm gap is DELIBERATE — the
  // navigator needs distinct boundaries between bars and baseplate. Not a bug.
  const G4double barZ = basePos.z() + baseHalfThick + pipeHalfHeight + 0.01*mm;

  // Central 40 mm-dia aluminium bar, on the axis.
  auto* sCenter = new G4Tubs("Center_Alum", 0, 20*mm, pipeHalfHeight, 0*deg, 360*deg);
  auto* lCenter = new G4LogicalVolume(sCenter, alum, "Center_Alum");
  lCenter->SetVisAttributes(new G4VisAttributes(G4Colour(1, 0.8, 0)));
  PlacePhantomVolume("PhysCenter", lCenter, G4ThreeVector(0, 0, barZ), world, 0);

  // Hexagonal ring, radius 60 mm, alternating steel / poly.
  //
  // Pass 4: these are PHANTOM-FRAME positions. They are NOT recomputed when
  // theta changes — the transform is applied to them by PlacePhantomVolume and
  // re-applied by ApplyScanTransform. Rotating them here instead would bake the
  // angle into the geometry and defeat runtime motion.
  const G4double ringR = 60.0 * mm;
  for (int i = 0; i < 6; ++i) {
    const G4double angle = i * 60 * deg;
    const G4double xPos = ringR * std::cos(angle);
    const G4double yPos = ringR * std::sin(angle);

    G4String    name;
    G4Material* barMat;
    G4Colour    barCol;
    G4double    barRad;

    if (i % 2 == 0) {                    // steel, 20 mm dia
      name = "SteelBar"; barMat = steel; barCol = G4Colour(1, 0, 0); barRad = 10*mm;
    } else {                             // poly, 30 mm dia
      name = "PolyBar";  barMat = poly;  barCol = G4Colour(0, 1, 0); barRad = 15*mm;
    }

    auto* sBar = new G4Tubs(name, 0, barRad, pipeHalfHeight, 0*deg, 360*deg);
    auto* lBar = new G4LogicalVolume(sBar, barMat, name);
    lBar->SetVisAttributes(new G4VisAttributes(barCol));
    PlacePhantomVolume(name, lBar, G4ThreeVector(xPos, yPos, barZ), world, i);
  }

  return lCenter;
}

// -----------------------------------------------------------------------------
// ─── CTTWIN START: Pass 3 — flat slab Beer-Lambert phantom ───
// A flat carbon-steel slab centred on the rotation axis, its thickness running
// along the beam axis (+x). The on-axis pencil ray therefore traverses exactly
// fSlabThickness of steel — a single, exactly-known path length, which is the
// whole point. The pipe wall was rejected for this test because the chord
// through a curved wall depends on where the ray enters.
//
// Placement: centred at the origin, so the slab spans x in [-t/2, +t/2]. The
// rotation axis passes through it, which keeps the slab consistent with every
// other phantom's convention.
//
// Pass 4: the slab is now the ONE phantom whose own rotation matrix (rather
// than just its position) affects the physics — the pipe and the bars are all
// cylinders about +z and are invariant under rotation about their own axes.
// Rotating the slab to theta = 90 deg presents kSlabLateral of steel to the
// beam instead of fSlabThickness, which is a useful sanity case.
//
// Lateral size is fixed (not a function of thickness) so that changing t
// changes exactly one thing. At 100 mm it is comfortably wider than the 50.8 mm
// detector face, so no ray reaching the detector can have travelled around the
// slab edge.
G4LogicalVolume* DetectorConstruction::BuildSlabPhantom(G4LogicalVolume* world)
{
  G4Material* steel = G4Material::GetMaterial("CarbonSteel");

  const G4double halfX = fSlabThickness / 2.0;              // along the beam
  const G4double halfY = Geometry::kSlabLateral / 2.0;
  const G4double halfZ = Geometry::kSlabLateral / 2.0;

  auto* solidSlab = new G4Box("SolidSlab", halfX, halfY, halfZ);
  auto* logicSlab = new G4LogicalVolume(solidSlab, steel, "LogicSlab");

  auto* vis = new G4VisAttributes(G4Colour(0.6, 0.6, 0.7, 0.5));
  vis->SetForceSolid(true);
  logicSlab->SetVisAttributes(vis);

  PlacePhantomVolume("PhysSlab", logicSlab, G4ThreeVector(0, 0, 0), world, 0);

  G4cout << "[CTTwin] slab phantom built: " << fSlabThickness / mm
         << " mm carbon steel along +x, " << Geometry::kSlabLateral / mm
         << " x " << Geometry::kSlabLateral / mm << " mm lateral" << G4endl;

  return logicSlab;
}
// ─── CTTWIN END ───

// -----------------------------------------------------------------------------
// Pass 1 — idealised photon counter. A thin AIR box on the far side of the
// phantom from the source, its face normal to the beam (+x). Air so it counts
// arrivals without attenuating; the SensitiveDetector does the counting.
G4LogicalVolume* DetectorConstruction::BuildDetector(G4LogicalVolume* world)
{
  G4Material* air = G4Material::GetMaterial("G4_AIR");

  const G4double halfX = Geometry::kDetectorThickness / 2.0;  // thin along beam
  const G4double halfY = Geometry::kDetectorFace / 2.0;
  const G4double halfZ = Geometry::kDetectorFace / 2.0;

  auto* solidDet = new G4Box("Detector", halfX, halfY, halfZ);
  fDetectorLV = new G4LogicalVolume(solidDet, air, "Detector");

  auto* vis = new G4VisAttributes(G4Colour(1.0, 1.0, 0.0, 0.3));  // yellow, transparent
  vis->SetForceSolid(true);
  fDetectorLV->SetVisAttributes(vis);

  // Placed at +kIsoToDetector on the beam axis, opposite the source (-x side).
  // Fixed for the life of the process — the phantom moves, not the rig.
  new G4PVPlacement(nullptr, G4ThreeVector(Geometry::kIsoToDetector, 0, 0),
                    fDetectorLV, "PhysDetector", world, false, 0, true);

  return fDetectorLV;
}

// -----------------------------------------------------------------------------
// Pass 1 — attach the SensitiveDetector. Done here, NOT in Construct(), because
// in multithreaded mode ConstructSDandField() is called per worker thread and
// is the thread-safe hook for SD registration.
//
// Pass 3: look the SD up before creating one. ConstructSDandField() can be
// invoked more than once per thread if the geometry is ever rebuilt; creating a
// second SD under the same name leaks and produces a duplicate-name warning.
// Cheap insurance, no behaviour change on the normal single-build path.
void DetectorConstruction::ConstructSDandField()
{
  auto* sdManager = G4SDManager::GetSDMpointer();

  auto* existing = sdManager->FindSensitiveDetector("CTTwin/Detector", false);
  if (existing) {
    SetSensitiveDetector(fDetectorLV, existing);
    return;
  }

  auto* sd = new SensitiveDetector("CTTwin/Detector");
  sdManager->AddNewDetector(sd);
  SetSensitiveDetector(fDetectorLV, sd);
}

}  // namespace CTTwin
