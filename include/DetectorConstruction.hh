#ifndef CTTWIN_DetectorConstruction_h
#define CTTWIN_DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "Constants.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

namespace CTTwin
{

class DetectorMessenger;

/// Builds the world, materials, the active phantom (one at a time, centred on
/// the rotation axis) and the idealised detector volume opposite the source.
class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;   // Pass 1: attach the SensitiveDetector (MT-safe hook)

    // ─── CTTWIN START: Pass 3 runtime configuration ───
    // Driven by DetectorMessenger. Both MUST be set before /run/initialize —
    // they are read during Construct(), which runs once when the geometry is
    // closed. The messenger refuses commands issued after that point rather
    // than silently doing nothing.
    //
    // "pipe" (Option A) | "bars" (Option B) | "slab" (Pass 3 Beer-Lambert) |
    // "none" (empty world — the open-beam N0 reference).
    void SetActivePhantom(const G4String& name) { fActivePhantom = name; }
    void SetSlabThickness(G4double t)           { fSlabThickness = t; }

    const G4String& GetActivePhantom() const { return fActivePhantom; }
    G4double        GetSlabThickness() const { return fSlabThickness; }
    // ─── CTTWIN END ───

  private:
    void DefineMaterials();
    G4LogicalVolume* BuildPipePhantom(G4LogicalVolume* world);   // Option A
    G4LogicalVolume* BuildBarsPhantom(G4LogicalVolume* world);   // Option B
    G4LogicalVolume* BuildSlabPhantom(G4LogicalVolume* world);   // Pass 3
    G4LogicalVolume* BuildDetector(G4LogicalVolume* world);      // Pass 1

    G4String fActivePhantom = "pipe";
    G4double fSlabThickness = Geometry::kDefaultSlabThickness;
    G4LogicalVolume* fDetectorLV = nullptr;   // set in BuildDetector, used by ConstructSDandField
    DetectorMessenger* fMessenger = nullptr;  // Pass 3
};

}  // namespace CTTwin

#endif
