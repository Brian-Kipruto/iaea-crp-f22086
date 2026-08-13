#ifndef CTTWIN_DetectorMessenger_h
#define CTTWIN_DetectorMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithADoubleAndUnit;

namespace CTTwin
{

class DetectorConstruction;

/// Pass 3 — MINIMAL geometry messenger, pulled forward from Pass 4.
///
/// Scope is deliberately two commands. Pass 3 has to run five configurations
/// (open beam + four slab thicknesses) and the alternative was editing a
/// default, rebuilding, running and reverting five times — five chances to
/// commit an un-reverted default, and a validation script that could only
/// parse logs instead of driving runs. This is the smaller risk.
///
///   /cttwin/phantom       pipe | bars | slab | none
///   /cttwin/slabThickness <value> <unit>
///
/// Both are STATE-SETTING ONLY and must be issued BEFORE /run/initialize.
/// The geometry is read once, when it is closed. A command issued afterwards
/// is rejected with a loud error rather than being accepted and ignored —
/// silently running the wrong thickness is exactly the failure mode that would
/// produce a plausible-looking but wrong validation table.
///
/// Source position/angle/translation commands and the full `/cttwin/source`
/// directory remain Pass 4. This class will grow into that one.
class DetectorMessenger : public G4UImessenger
{
  public:
    explicit DetectorMessenger(DetectorConstruction* detector);
    ~DetectorMessenger() override;

    void SetNewValue(G4UIcommand* command, G4String newValue) override;

  private:
    G4bool GeometryIsClosed() const;

    DetectorConstruction* fDetector = nullptr;

    G4UIdirectory*             fDirectory    = nullptr;
    G4UIcmdWithAString*        fPhantomCmd   = nullptr;
    G4UIcmdWithADoubleAndUnit* fSlabThickCmd = nullptr;
};

}  // namespace CTTwin

#endif
