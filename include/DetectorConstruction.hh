#ifndef CTTWIN_DetectorConstruction_h
#define CTTWIN_DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "Constants.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "globals.hh"

#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;

namespace CTTwin
{

class DetectorMessenger;

/// Builds the world, materials, the active phantom (one at a time, centred on
/// the rotation axis) and the idealised detector volume opposite the source.
///
/// Pass 4 adds the SCAN TRANSFORM. The source and detector are fixed in world
/// coordinates for the whole life of the process; the scan coordinates
/// (theta, t) are carried by the phantom instead. See ADR 0005 and the note in
/// Constants.hh for why that is exact rather than approximate.
///
/// Every phantom placement is registered in fPhantomPlacements together with
/// its position in the phantom's OWN frame (its position at theta = 0). That
/// list is what makes runtime motion possible: on a scan-coordinate change the
/// transform is recomputed and pushed onto the existing physical volumes with
/// SetTranslation/SetRotation, followed by
/// G4RunManager::GeometryHasBeenModified(). Nothing is rebuilt, so the move is
/// cheap enough to do between every beamOn — which is what lets one process
/// run a whole angle's worth of translations instead of one process per
/// measurement.
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

    // ─── CTTWIN START: Pass 4 scan coordinates ───
    // UNLIKE the two above, these are legal at any time. They move existing
    // volumes rather than rebuilding solids, so there is nothing to re-read at
    // initialisation and no stale-geometry hazard beyond the one Geant4 already
    // handles for us via GeometryHasBeenModified().
    void SetScanAngle(G4double theta);
    void SetScanTranslation(G4double t);

    G4double GetScanAngle() const       { return fScanAngle; }
    G4double GetScanTranslation() const { return fScanTranslation; }
    // ─── CTTWIN END ───

    // ─── CTTWIN START: Pass 5 projection output ───
    // Where RunAction appends this run's row, and what to label it.
    //
    // These live here rather than in RunAction because DetectorConstruction is
    // the one messenger-owned, master-side configuration object in the app, and
    // RunAction already reaches for it to echo the phantom and scan state. A
    // separate run-configuration holder is the tidier structure and is a Pass
    // 5+ job if this list grows past a handful.
    //
    // An empty path means "write nothing", which is the default — so every
    // Pass 1-4 macro and validate_beer_lambert.py behave exactly as before.
    void SetOutputFile(const G4String& path) { fOutputFile = path; }
    void SetProjectionId(G4int id)           { fProjectionId = id; }

    const G4String& GetOutputFile() const { return fOutputFile; }
    G4int GetProjectionId() const         { return fProjectionId; }
    // ─── CTTWIN END ───

  private:
    void DefineMaterials();
    G4LogicalVolume* BuildPipePhantom(G4LogicalVolume* world);   // Option A
    G4LogicalVolume* BuildBarsPhantom(G4LogicalVolume* world);   // Option B
    G4LogicalVolume* BuildSlabPhantom(G4LogicalVolume* world);   // Pass 3
    G4LogicalVolume* BuildDetector(G4LogicalVolume* world);      // Pass 1

    // ─── CTTWIN START: Pass 4 scan transform ───
    /// One phantom sub-volume and where it sits in the phantom's own frame.
    struct PhantomPlacement
    {
      G4VPhysicalVolume* pv = nullptr;
      G4ThreeVector      localPos;   // position at theta = 0, t = 0
    };

    /// Place a phantom sub-volume through the scan transform and register it
    /// so it can be moved later. Every phantom volume MUST go through this —
    /// a volume placed directly with G4PVPlacement will not follow the scan
    /// and will silently stay behind when the phantom rotates.
    G4VPhysicalVolume* PlacePhantomVolume(const G4String& name,
                                          G4LogicalVolume* logic,
                                          const G4ThreeVector& localPos,
                                          G4LogicalVolume* world,
                                          G4int copyNo);

    /// p_world = R_z(theta) * p_local - t * y_hat
    G4ThreeVector WorldPositionOf(const G4ThreeVector& localPos) const;

    /// Rebuild fPhantomFrameRotation from fScanAngle. Cheap; called by both
    /// the build path and the runtime-motion path so they cannot disagree.
    void UpdateFrameRotation();

    /// nullptr at theta = 0, the shared matrix otherwise. See the note in
    /// DetectorConstruction.cc — this exists to keep the unrotated case on the
    /// exact navigator code path the Pass 1-3 anchors were measured on.
    G4RotationMatrix* RotationForPlacement() const;

    /// Push the current (theta, t) onto every registered placement and tell the
    /// run manager the geometry moved. No-op before the geometry exists.
    void ApplyScanTransform();
    // ─── CTTWIN END ───

    G4String fActivePhantom = "pipe";
    G4double fSlabThickness = Geometry::kDefaultSlabThickness;
    G4LogicalVolume* fDetectorLV = nullptr;   // set in BuildDetector, used by ConstructSDandField
    DetectorMessenger* fMessenger = nullptr;  // Pass 3

    // ─── CTTWIN START: Pass 4 scan state ───
    G4double fScanAngle       = Geometry::kDefaultScanAngle;
    G4double fScanTranslation = Geometry::kDefaultScanTranslation;

    std::vector<PhantomPlacement> fPhantomPlacements;

    /// The rotation handed to G4PVPlacement / SetRotation. This is the rotation
    /// of the MOTHER frame with respect to the daughter, i.e. the INVERSE of
    /// the phantom's own rotation — the standard Geant4 sign trap. One matrix,
    /// shared by every phantom volume (they all rotate together) and owned
    /// here, because Geant4 does not take ownership of a placement rotation.
    G4RotationMatrix* fPhantomFrameRotation = nullptr;
    // ─── CTTWIN END ───

    // ─── CTTWIN START: Pass 5 projection output ───
    G4String fOutputFile;            // empty = no CSV written
    G4int    fProjectionId = 0;
    // ─── CTTWIN END ───
};

}  // namespace CTTwin

#endif
