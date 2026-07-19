#include "RunAction.hh"

#include "G4Run.hh"
#include "G4AccumulableManager.hh"

namespace CTTwin
{

RunAction::RunAction()
{
  G4AccumulableManager::Instance()->RegisterAccumulable(fDetectorCount);
}

void RunAction::BeginOfRunAction(const G4Run*)
{
  G4AccumulableManager::Instance()->Reset();
}

void RunAction::EndOfRunAction(const G4Run* run)
{
  const G4int nEvents = run->GetNumberOfEvent();
  if (nEvents == 0) return;

  // Merge per-thread accumulables into the master.
  G4AccumulableManager::Instance()->Merge();

  if (!IsMaster()) return;

  const G4int counts = fDetectorCount.GetValue();
  G4cout << "\n--------------------- CTTwin run summary ---------------------\n"
         << "  Events fired      : " << nEvents << "\n"
         << "  Detector counts   : " << counts << "\n"
         << "  Fraction detected : "
         << (nEvents > 0 ? (G4double)counts / nEvents : 0.0) << "\n"
         << "--------------------------------------------------------------\n"
         << G4endl;
}

}  // namespace CTTwin
