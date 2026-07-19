#include "EventAction.hh"
#include "RunAction.hh"
#include "SensitiveDetector.hh"

#include "G4Event.hh"
#include "G4SDManager.hh"

namespace CTTwin
{

EventAction::EventAction(RunAction* runAction)
  : fRunAction(runAction)
{}

// Resolve the SD by name once, then cache. Done lazily (not in the constructor)
// because the SD is registered in ConstructSDandField(), which runs per worker
// thread and may not be ready when EventAction is constructed.
SensitiveDetector* EventAction::FindDetector()
{
  if (!fDetector) {
    auto* sd = G4SDManager::GetSDMpointer()
                 ->FindSensitiveDetector("CTTwin/Detector");
    fDetector = static_cast<SensitiveDetector*>(sd);
  }
  return fDetector;
}

void EventAction::BeginOfEventAction(const G4Event*)
{
  // SD resets its own count in its Initialize(); nothing to do here.
}

void EventAction::EndOfEventAction(const G4Event*)
{
  auto* det = FindDetector();
  if (det && fRunAction) {
    fRunAction->AddCount(det->GetCount());
  }
}

}  // namespace CTTwin
