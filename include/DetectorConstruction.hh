#ifndef CTTWIN_DetectorConstruction_h
#define CTTWIN_DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

namespace CTTwin
{

/// Builds the world, materials, the active phantom (one at a time, centred on
/// the rotation axis) and the idealised detector volume opposite the source.
class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction() = default;
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;   // Pass 1: attach the SensitiveDetector (MT-safe hook)

    // Pass 4 will drive this via a messenger.
    // "pipe" (Option A) | "bars" (Option B) | "none" (empty world, for the
    // Pass 1 detector checkpoint and the Pass 3/5 open-beam N0 reference).
    void SetActivePhantom(const G4String& name) { fActivePhantom = name; }

  private:
    void DefineMaterials();
    G4LogicalVolume* BuildPipePhantom(G4LogicalVolume* world);   // Option A
    G4LogicalVolume* BuildBarsPhantom(G4LogicalVolume* world);   // Option B
    G4LogicalVolume* BuildDetector(G4LogicalVolume* world);      // Pass 1

    G4String fActivePhantom = "pipe";
    G4LogicalVolume* fDetectorLV = nullptr;   // set in BuildDetector, used by ConstructSDandField
};

}  // namespace CTTwin

#endif
