#ifndef CTTWIN_SensitiveDetector_h
#define CTTWIN_SensitiveDetector_h 1

#include "G4VSensitiveDetector.hh"
#include "globals.hh"
#include <vector>

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

namespace CTTwin
{

/// Pass 1 — idealised photon counter (pattern 1: G4VSensitiveDetector).
/// Counts gamma tracks ENTERING the detector volume (one crossing per photon,
/// caught at the geometric boundary). NOT energy deposition — that was the v1
/// mistake. One pixel in Pass 1 (whole face); ProcessHits already derives a
/// pixel index from hit position so subdivision later needs no re-plumbing.
///
/// EventAction PULLS the per-event count via GetCount() at EndOfEventAction —
/// no back-pointer into the action classes, which keeps this MT-clean.
class SensitiveDetector : public G4VSensitiveDetector
{
  public:
    explicit SensitiveDetector(const G4String& name);
    ~SensitiveDetector() override = default;

    void   Initialize(G4HCofThisEvent*) override;   // per-event reset
    G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;

    G4int GetCount() const { return fCount; }        // this event, single pixel

  private:
    G4int fCount = 0;                 // gamma entries this event (one pixel)
};

}  // namespace CTTwin

#endif
