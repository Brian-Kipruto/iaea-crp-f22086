#ifndef CTTWIN_DetectorConstruction_h
#define CTTWIN_DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

namespace CTTwin
{

/// Builds the world, materials and the active phantom (one at a time,
/// centred on the rotation axis). The detector volume is added in Pass 1.
class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction() = default;
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;

    // Pass 4 will drive this via a messenger. "pipe" (Option A) | "bars" (Option B).
    void SetActivePhantom(const G4String& name) { fActivePhantom = name; }

  private:
    void DefineMaterials();
    G4LogicalVolume* BuildPipePhantom(G4LogicalVolume* world);   // Option A
    G4LogicalVolume* BuildBarsPhantom(G4LogicalVolume* world);   // Option B

    G4String fActivePhantom = "pipe";
};

}  // namespace CTTwin

#endif
