#ifndef CTTWIN_RunAction_h
#define CTTWIN_RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

class G4Run;

namespace CTTwin
{

/// Accumulates the detector photon count across all events in a run and prints
/// the total. Same wiring SHAPE as v1's dose chain (AddEdep/G4Accumulable),
/// with the quantity swapped from G4double energy to G4int count.
class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void AddCount(G4int n) { fDetectorCount += n; }

  private:
    G4Accumulable<G4int> fDetectorCount{0};
};

}  // namespace CTTwin

#endif
