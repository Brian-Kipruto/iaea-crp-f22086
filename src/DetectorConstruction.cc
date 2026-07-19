#include "DetectorConstruction.hh"
#include "Constants.hh"
#include "SensitiveDetector.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SDManager.hh"

namespace CTTwin
{

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
  // later open-beam N0 reference.
  if (fActivePhantom == "none") {
    // no phantom — empty world
  } else if (fActivePhantom == "bars") {
    BuildBarsPhantom(logicWorld);
  } else {
    BuildPipePhantom(logicWorld);   // default: Option A
  }
  // ─── CTTWIN END ───

  // ─── CTTWIN START: Pass 1 — detector volume ───
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

  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), logicPipe,
                    "PhysPipe", world, false, 0, true);
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
  new G4PVPlacement(nullptr, basePos, logicBase, "PhysBase", world, false, 0, true);

  // Bars sit on top of the baseplate. The 0.01 mm gap is DELIBERATE — the
  // navigator needs distinct boundaries between bars and baseplate. Not a bug.
  const G4double barZ = basePos.z() + baseHalfThick + pipeHalfHeight + 0.01*mm;

  // Central 40 mm-dia aluminium bar, on the axis.
  auto* sCenter = new G4Tubs("Center_Alum", 0, 20*mm, pipeHalfHeight, 0*deg, 360*deg);
  auto* lCenter = new G4LogicalVolume(sCenter, alum, "Center_Alum");
  lCenter->SetVisAttributes(new G4VisAttributes(G4Colour(1, 0.8, 0)));
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, barZ), lCenter,
                    "PhysCenter", world, false, 0, true);

  // Hexagonal ring, radius 60 mm, alternating steel / poly.
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
    new G4PVPlacement(nullptr, G4ThreeVector(xPos, yPos, barZ), lBar,
                      name, world, false, i, true);
  }

  return lCenter;
}

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
  new G4PVPlacement(nullptr, G4ThreeVector(Geometry::kIsoToDetector, 0, 0),
                    fDetectorLV, "PhysDetector", world, false, 0, true);

  return fDetectorLV;
}

// -----------------------------------------------------------------------------
// Pass 1 — attach the SensitiveDetector. Done here, NOT in Construct(), because
// in multithreaded mode ConstructSDandField() is called per worker thread and
// is the thread-safe hook for SD registration.
void DetectorConstruction::ConstructSDandField()
{
  auto* sd = new SensitiveDetector("CTTwin/Detector");
  G4SDManager::GetSDMpointer()->AddNewDetector(sd);
  SetSensitiveDetector(fDetectorLV, sd);
}

}  // namespace CTTwin
