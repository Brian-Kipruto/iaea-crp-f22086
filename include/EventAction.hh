#ifndef CTTWIN_EventAction_h
#define CTTWIN_EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class G4Event;

namespace CTTwin
{

class RunAction;
class SensitiveDetector;

/// At the end of each event, pulls the per-event counts from the
/// SensitiveDetector and hands them to RunAction for run-level accumulation.
/// Holds a RunAction* (constructed in ActionInitialization) — the standard
/// Geant4 per-event -> per-run bridge, same shape v1 used for dose.
///
/// Pass 3: two counts now travel this bridge — total arrivals and the
/// unscattered-primary subset. Same pull pattern, one extra call.
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
