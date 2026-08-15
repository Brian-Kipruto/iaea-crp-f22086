#ifndef CTTWIN_DetectorMessenger_h
#define CTTWIN_DetectorMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithAnInteger;

namespace CTTwin
{

class DetectorConstruction;

/// The `/cttwin/...` command vocabulary. Pass 3 pulled a two-command skeleton
/// forward out of Pass 4; this is that skeleton grown into the full thing.
///
///   GEOMETRY REBUILD — pre-init only, rejected loudly afterwards:
///     /cttwin/phantom          pipe | bars | slab | none
///     /cttwin/slabThickness    <value> <unit>
///
///   SCAN MOTION — legal at any time, including between runs:
///     /cttwin/scan/angle        <value> <unit>    rotation theta about +z
///     /cttwin/scan/translation  <value> <unit>    offset t along +y
///
///   OUTPUT — legal at any time:
///     /cttwin/output/file          <path>   append rows here; empty = none
///     /cttwin/output/projectionId  <int>    label for the next row
///
/// The split is the point of this class. The first pair changes which SOLIDS
/// exist and can only be read when the geometry is built, so issuing one after
/// /run/initialize is an error worth shouting about — silently running the
/// wrong thickness is exactly the failure mode that produces a plausible but
/// wrong validation table. The second pair only MOVES volumes that already
/// exist, which Geant4 supports between runs via
/// G4RunManager::GeometryHasBeenModified(). That is what allows one process to
/// run a whole sweep instead of one process per measurement — roughly 180
/// launches for a full scan instead of 23,000, and it also stops every point in
/// the sinogram inheriting the same default RNG seed.
///
/// Naming note: the vault planned `/cttwin/source setAngle`. Since ADR 0005
/// moves the phantom and leaves the source fixed, `source` would be actively
/// misleading. `/cttwin/scan/...` names the scan coordinate rather than the
/// mechanism, which is what the Python driver actually cares about.
class DetectorMessenger : public G4UImessenger
{
  public:
    explicit DetectorMessenger(DetectorConstruction* detector);
    ~DetectorMessenger() override;

    void SetNewValue(G4UIcommand* command, G4String newValue) override;

  private:
    G4bool GeometryIsClosed() const;
    void   RejectPostInit(G4UIcommand* command) const;

    DetectorConstruction* fDetector = nullptr;

    G4UIdirectory*             fDirectory    = nullptr;   // /cttwin/
    G4UIcmdWithAString*        fPhantomCmd   = nullptr;
    G4UIcmdWithADoubleAndUnit* fSlabThickCmd = nullptr;

    // ─── CTTWIN START: Pass 4 scan commands ───
    G4UIdirectory*             fScanDirectory = nullptr;  // /cttwin/scan/
    G4UIcmdWithADoubleAndUnit* fAngleCmd      = nullptr;
    G4UIcmdWithADoubleAndUnit* fTranslationCmd = nullptr;
    // ─── CTTWIN END ───

    // ─── CTTWIN START: Pass 5 output commands ───
    G4UIdirectory*        fOutputDirectory = nullptr;   // /cttwin/output/
    G4UIcmdWithAString*   fOutputFileCmd   = nullptr;
    G4UIcmdWithAnInteger* fProjectionIdCmd = nullptr;
    // ─── CTTWIN END ───
};

}  // namespace CTTwin

#endif
