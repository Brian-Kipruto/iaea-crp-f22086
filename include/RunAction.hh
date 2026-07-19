#ifndef CTTWIN_RunAction_h
#define CTTWIN_RunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;

namespace CTTwin
{

/// Pass 0 stub. v1's dose-accumulation members (G4Accumulable<G4double> fEdep)
/// were NOT ported — Pass 1 wires this to the SensitiveDetector photon count.
class RunAction : public G4UserRunAction
{
  public:
    RunAction() = default;
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
};

}  // namespace CTTwin

#endif
