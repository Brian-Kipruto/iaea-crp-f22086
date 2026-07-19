#ifndef CTTWIN_EventAction_h
#define CTTWIN_EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class G4Event;

namespace CTTwin
{

class RunAction;
class SensitiveDetector;

/// At the end of each event, pulls the per-event count from the
/// SensitiveDetector and hands it to RunAction for run-level accumulation.
/// Holds a RunAction* (constructed in ActionInitialization) — the standard
/// Geant4 per-event -> per-run bridge, same shape v1 used for dose.
class EventAction : public G4UserEventAction
{
  public:
    explicit EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;

  private:
    SensitiveDetector* FindDetector();   // lazily resolved via G4SDManager

    RunAction* fRunAction = nullptr;
    SensitiveDetector* fDetector = nullptr;   // cached after first lookup
};

}  // namespace CTTwin

#endif
