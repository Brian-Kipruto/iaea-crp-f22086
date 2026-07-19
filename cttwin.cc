// =============================================================================
// cttwin.cc — main entry point for the CTTwin first-generation gamma CT
// simulator.
//
// IAEA CRP F22086 — Advanced Nuclear Imaging Techniques for Industrial
// Process Analysis and Component Testing. Kenya contribution, University of
// Nairobi.
// =============================================================================

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "Randomize.hh"

// --- CTTWIN START: Pass 0 physics list ---
// Explicit EM physics for photon transport in the 50 keV - 1.5 MeV range.
// This REPLACES v1's QBBC (a hadronic/space-radiation list, wrong for our
// energies). Setting it wrong would silently skew Beer-Lambert in Pass 3.
// See Architecture Lockdown decisions #8 and #11.
#include "G4EmStandardPhysics_option4.hh"
#include "G4VModularPhysicsList.hh"

namespace {
class CTTwinPhysicsList : public G4VModularPhysicsList {
 public:
  CTTwinPhysicsList() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
  }
};
}  // namespace
// --- CTTWIN END ---

using namespace CTTwin;

int main(int argc, char** argv)
{
  G4UIExecutive* ui = nullptr;
  if (argc == 1) { ui = new G4UIExecutive(argc, argv); }

  auto* runManager =
      G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  runManager->SetUserInitialization(new DetectorConstruction());
  runManager->SetUserInitialization(new CTTwinPhysicsList());
  runManager->SetUserInitialization(new ActionInitialization());

  auto* visManager = new G4VisExecutive(argc, argv);
  visManager->Initialize();

  auto* UImanager = G4UImanager::GetUIpointer();

  if (!ui) {
    UImanager->ApplyCommand("/control/execute " + G4String(argv[1]));
  } else {
    UImanager->ApplyCommand("/control/execute vis.mac");
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}
