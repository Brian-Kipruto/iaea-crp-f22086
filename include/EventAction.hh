#ifndef CTTWIN_EventAction_h
#define CTTWIN_EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

namespace CTTwin
{

/// Pass 0 stub. v1's AddEdep / fEdep dose members were NOT ported.
/// Pass 1 uses this to collect the per-event detector count.
class EventAction : public G4UserEventAction
{
  public:
    EventAction() = default;
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override {}
    void EndOfEventAction(const G4Event*) override {}
};

}  // namespace CTTwin

#endif
