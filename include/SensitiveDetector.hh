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
///
/// Pass 3 — a SECOND, parallel count of UNSCATTERED PRIMARIES.
/// Beer-Lambert, exp(-mu*t), describes primary transmission only. The Pass 1
/// counter registers every gamma that arrives, including ones that scattered
/// forward in the phantom and still landed on the face. Counting both lets the
/// validation quote the residual scatter contribution as a measured number
/// instead of assuming it away — the open item left by
/// docs/validation/geometry-update-500mm-sdd.md. See ADR 0004.
/// GetCount() is unchanged, so the Pass 1/2 regression anchors still mean the
/// same thing.
class SensitiveDetector : public G4VSensitiveDetector
{
  public:
    explicit SensitiveDetector(const G4String& name);
    ~SensitiveDetector() override = default;

    void   Initialize(G4HCofThisEvent*) override;   // per-event reset
    G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;

    G4int GetCount() const            { return fCount; }        // this event, single pixel
    G4int GetUnscatteredCount() const { return fUnscatteredCount; }

  private:
    G4int fCount = 0;                 // gamma entries this event (one pixel)
    G4int fUnscatteredCount = 0;      // subset that never interacted (Pass 3)
};

}  // namespace CTTwin

#endif
