#include "SensitiveDetector.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Gamma.hh"
#include "G4StepPoint.hh"
#include "G4ThreeVector.hh"

namespace CTTwin
{

SensitiveDetector::SensitiveDetector(const G4String& name)
  : G4VSensitiveDetector(name)
{}

// Called by Geant4 at the start of every event — reset the per-event count.
void SensitiveDetector::Initialize(G4HCofThisEvent*)
{
  fCount = 0;
}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*)
{
  auto* track = step->GetTrack();

  // Gammas only — the primary beam. (Idealised counter: no secondaries logic.)
  if (track->GetDefinition() != G4Gamma::Definition()) return false;

  // Count ONCE per photon, on entry: the pre-step point sits on the geometric
  // boundary of the detector. Without this guard a photon taking several steps
  // inside the 1 mm volume would be counted multiple times.
  auto* pre = step->GetPreStepPoint();
  if (pre->GetStepStatus() != fGeomBoundary) return false;

  // ─── CTTWIN START: Pass 1 pixel index ───
  // One pixel in Pass 1 (whole 50.8 mm face). Position is captured now so
  // subdivision later (fan beam / finer sampling) needs no re-plumbing:
  // derive a pixel index from pos.y()/pos.z() here when the time comes.
  const G4ThreeVector pos = pre->GetPosition();
  const G4int pixel = 0;   // single pixel
  (void)pos;               // captured, unused while pixel count is 1
  // ─── CTTWIN END ───

  if (pixel == 0) fCount++;
  return true;
}

}  // namespace CTTwin
