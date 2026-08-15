#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"
#include "Constants.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4RunManager.hh"
#include "G4StateManager.hh"
#include "G4ApplicationState.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

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
  fPhantomCmd->SetGuidance("PRE-INIT ONLY: rebuilds solids.");
  fPhantomCmd->SetParameterName("name", false);
  fPhantomCmd->SetCandidates("pipe bars slab none");
  fPhantomCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // --- /cttwin/slabThickness ---
  fSlabThickCmd = new G4UIcmdWithADoubleAndUnit("/cttwin/slabThickness", this);
  fSlabThickCmd->SetGuidance("Thickness of the flat slab phantom along the beam axis (+x).");
  fSlabThickCmd->SetGuidance("Only meaningful when /cttwin/phantom is 'slab'.");
  fSlabThickCmd->SetGuidance("PRE-INIT ONLY: rebuilds solids.");
  fSlabThickCmd->SetParameterName("t", false);
  fSlabThickCmd->SetDefaultUnit("mm");
  fSlabThickCmd->SetUnitCategory("Length");
  fSlabThickCmd->SetRange("t > 0");
  fSlabThickCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // ─── CTTWIN START: Pass 4 scan commands ───
  fScanDirectory = new G4UIdirectory("/cttwin/scan/");
  fScanDirectory->SetGuidance("Translate-rotate scan coordinates.");
  fScanDirectory->SetGuidance("Both may be changed between runs — no rebuild needed.");

  // --- /cttwin/scan/angle ---
  fAngleCmd = new G4UIcmdWithADoubleAndUnit("/cttwin/scan/angle", this);
  fAngleCmd->SetGuidance("Projection angle theta: phantom rotation about +z,");
  fAngleCmd->SetGuidance("counterclockwise viewed from +z. See [[Coordinate Conventions]].");
  fAngleCmd->SetGuidance("A full parallel-beam scan covers theta in [0, 180) deg.");
  fAngleCmd->SetParameterName("theta", false);
  fAngleCmd->SetDefaultUnit("deg");
  fAngleCmd->SetUnitCategory("Angle");
  fAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // --- /cttwin/scan/translation ---
  fTranslationCmd = new G4UIcmdWithADoubleAndUnit("/cttwin/scan/translation", this);
  fTranslationCmd->SetGuidance("Translation offset t of the ray along +y, in the");
  fTranslationCmd->SetGuidance("canonical frame where the source-detector pair moves.");
  fTranslationCmd->SetGuidance("Implemented by shifting the phantom by -t (ADR 0005);");
  fTranslationCmd->SetGuidance("the ray itself always stays on the world x-axis.");
  fTranslationCmd->SetParameterName("t", false);
  fTranslationCmd->SetDefaultUnit("mm");
  fTranslationCmd->SetUnitCategory("Length");
  fTranslationCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  // ─── CTTWIN END ───

  // ─── CTTWIN START: Pass 5 output commands ───
  fOutputDirectory = new G4UIdirectory("/cttwin/output/");
  fOutputDirectory->SetGuidance("Per-projection CSV output.");

  // --- /cttwin/output/file ---
  fOutputFileCmd = new G4UIcmdWithAString("/cttwin/output/file", this);
  fOutputFileCmd->SetGuidance("Append one CSV row per /run/beamOn to this path.");
  fOutputFileCmd->SetGuidance("The header is written only if the file is new or empty,");
  fOutputFileCmd->SetGuidance("so one file can accumulate a whole angle's translations —");
  fOutputFileCmd->SetGuidance("180 files for a full scan rather than ~23,000.");
  fOutputFileCmd->SetGuidance("Unset (the default) means no file is written at all.");
  fOutputFileCmd->SetGuidance("Relative paths resolve against the working directory,");
  fOutputFileCmd->SetGuidance("which is where cttwin was launched — usually build/.");
  fOutputFileCmd->SetParameterName("path", false);
  fOutputFileCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // --- /cttwin/output/projectionId ---
  fProjectionIdCmd = new G4UIcmdWithAnInteger("/cttwin/output/projectionId", this);
  fProjectionIdCmd->SetGuidance("Label written in the projection_id column of the next row.");
  fProjectionIdCmd->SetGuidance("Explicit rather than auto-incremented: the Python driver");
  fProjectionIdCmd->SetGuidance("owns the numbering, and a row that mislabels itself is");
  fProjectionIdCmd->SetGuidance("worse than one that has no label.");
  fProjectionIdCmd->SetParameterName("id", false);
  fProjectionIdCmd->SetRange("id >= 0");
  fProjectionIdCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  // ─── CTTWIN END ───
}

DetectorMessenger::~DetectorMessenger()
{
  // ─── CTTWIN START: Pass 5 ───
  delete fProjectionIdCmd;
  delete fOutputFileCmd;
  delete fOutputDirectory;
  // ─── CTTWIN END ───
  // ─── CTTWIN START: Pass 4 ───
  delete fTranslationCmd;
  delete fAngleCmd;
  delete fScanDirectory;
  // ─── CTTWIN END ───
  delete fSlabThickCmd;
  delete fPhantomCmd;
  delete fDirectory;
}

// The SOLIDS are built once, in Construct(), when the run manager leaves
// PreInit. After that the phantom-selection and slab-thickness setters would
// change a member but not the world, so we refuse rather than accept-and-ignore.
//
// Pass 4: this test no longer gates the whole messenger. /cttwin/scan/... moves
// volumes that already exist and is legal in Idle.
G4bool DetectorMessenger::GeometryIsClosed() const
{
  auto* rm = G4RunManager::GetRunManager();
  return rm && (rm->GetCurrentRun() != nullptr ||
                G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit);
}

void DetectorMessenger::RejectPostInit(G4UIcommand* command) const
{
  G4cerr << "\n*** CTTwin ERROR: " << command->GetCommandPath()
         << " was issued after the geometry was initialised.\n"
         << "    This command rebuilds solids, and solids are built once, in\n"
         << "    Construct(). It has NOT been applied. Move it above\n"
         << "    /run/initialize in the macro, or start a fresh cttwin process\n"
         << "    for this configuration.\n"
         << "    (Scan motion, /cttwin/scan/..., is exempt — it moves existing\n"
         << "    volumes and is legal at any time.)\n"
         << G4endl;
}

void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (!fDetector) return;

  // --- Geometry-rebuild commands: pre-init only ---
  if (command == fPhantomCmd) {
    if (GeometryIsClosed()) { RejectPostInit(command); return; }
    fDetector->SetActivePhantom(newValue);
    G4cout << "[CTTwin] active phantom -> " << newValue << G4endl;
    return;
  }

  if (command == fSlabThickCmd) {
    if (GeometryIsClosed()) { RejectPostInit(command); return; }
    const G4double t = fSlabThickCmd->GetNewDoubleValue(newValue);
    fDetector->SetSlabThickness(t);
    G4cout << "[CTTwin] slab thickness -> " << t / mm << " mm" << G4endl;
    return;
  }

  // ─── CTTWIN START: Pass 4 scan motion — legal at any time ───
  if (command == fAngleCmd) {
    fDetector->SetScanAngle(fAngleCmd->GetNewDoubleValue(newValue));
    return;
  }

  if (command == fTranslationCmd) {
    const G4double t = fTranslationCmd->GetNewDoubleValue(newValue);

    // A translation beyond this is almost certainly a unit slip or a driver
    // bug, and it would walk the phantom towards the world boundary where the
    // beam path stops being pure air. Refuse rather than produce a projection
    // that looks fine and means nothing.
    if (std::fabs(t) > Geometry::kMaxScanTranslation) {
      G4cerr << "\n*** CTTwin ERROR: /cttwin/scan/translation " << t / mm
             << " mm exceeds the +/-" << Geometry::kMaxScanTranslation / mm
             << " mm guard.\n"
             << "    Not applied. Check the unit on the command.\n" << G4endl;
      return;
    }

    fDetector->SetScanTranslation(t);
    return;
  }
  // ─── CTTWIN END ───

  // ─── CTTWIN START: Pass 5 output ───
  if (command == fOutputFileCmd) {
    fDetector->SetOutputFile(newValue);
    G4cout << "[CTTwin] projection output -> " << newValue << G4endl;
    return;
  }

  if (command == fProjectionIdCmd) {
    fDetector->SetProjectionId(fProjectionIdCmd->GetNewIntValue(newValue));
    return;
  }
  // ─── CTTWIN END ───
}

}  // namespace CTTwin