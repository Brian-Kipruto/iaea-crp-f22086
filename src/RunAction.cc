#include "RunAction.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"

namespace CTTwin
{

void RunAction::BeginOfRunAction(const G4Run*) {}

void RunAction::EndOfRunAction(const G4Run* run)
{
  const G4int n = run->GetNumberOfEvent();
  if (n == 0) return;
  if (IsMaster()) {
    G4cout << "\n--- CTTwin run summary: " << n
           << " events processed. ---\n" << G4endl;
  }
}

}  // namespace CTTwin
