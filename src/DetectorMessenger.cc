#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4RunManager.hh"
#include "G4StateManager.hh"
#include "G4ApplicationState.hh"
#include "G4SystemOfUnits.hh"

namespace CTTwin
{

DetectorMessenger::DetectorMessenger(DetectorConstruction* detector)
  : fDetector(detector)
{
  fDirectory = new G4UIdirectory("/cttwin/");
  fDirectory->SetGuidance("CTTwin gamma CT simulator controls.");

  // --- /cttwin/phantom ---
  fPhantomCmd = new G4UIcmdWithAString("/cttwin/phantom", this);
  fPhantomCmd->SetGuidance("Select the active phantom at the rotation axis.");
  fPhantomCmd->SetGuidance("  pipe - Option A, 5\" SCH40 carbon-steel pipe");
  fPhantomCmd->SetGuidance("  bars - Option B, Al baseplate + hex ring of steel/poly bars");
  fPhantomCmd->SetGuidance("  slab - Pass 3 flat carbon-steel Beer-Lambert slab");
  fPhantomCmd->SetGuidance("  none - empty world (open-beam N0 reference)");
  fPhantomCmd->SetParameterName("name", false);
  fPhantomCmd->SetCandidates("pipe bars slab none");
  fPhantomCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // --- /cttwin/slabThickness ---
  fSlabThickCmd = new G4UIcmdWithADoubleAndUnit("/cttwin/slabThickness", this);
  fSlabThickCmd->SetGuidance("Thickness of the flat slab phantom along the beam axis (+x).");
  fSlabThickCmd->SetGuidance("Only meaningful when /cttwin/phantom is 'slab'.");
  fSlabThickCmd->SetParameterName("t", false);
  fSlabThickCmd->SetDefaultUnit("mm");
  fSlabThickCmd->SetUnitCategory("Length");
  fSlabThickCmd->SetRange("t > 0");
  fSlabThickCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

DetectorMessenger::~DetectorMessenger()
{
  delete fSlabThickCmd;
  delete fPhantomCmd;
  delete fDirectory;
}

// The geometry is read once, in Construct(), when the run manager leaves
// PreInit. After that these setters would change the member but not the world,
// so we refuse rather than accept-and-ignore.
G4bool DetectorMessenger::GeometryIsClosed() const
{
  auto* rm = G4RunManager::GetRunManager();
  return rm && (rm->GetCurrentRun() != nullptr ||
                G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit);
}

void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (!fDetector) return;

  if (GeometryIsClosed()) {
    G4cerr << "\n*** CTTwin ERROR: " << command->GetCommandPath()
           << " was issued after the geometry was initialised.\n"
           << "    Geometry is built once, in Construct(). This command has NOT\n"
           << "    been applied. Move it above /run/initialize in the macro, or\n"
           << "    start a fresh cttwin process for this configuration.\n"
           << G4endl;
    return;
  }

  if (command == fPhantomCmd) {
    fDetector->SetActivePhantom(newValue);
    G4cout << "[CTTwin] active phantom -> " << newValue << G4endl;
  }
  else if (command == fSlabThickCmd) {
    const G4double t = fSlabThickCmd->GetNewDoubleValue(newValue);
    fDetector->SetSlabThickness(t);
    G4cout << "[CTTwin] slab thickness -> " << t / mm << " mm" << G4endl;
  }
}

}  // namespace CTTwin
