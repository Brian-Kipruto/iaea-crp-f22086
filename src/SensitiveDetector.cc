#include "SensitiveDetector.hh"
#include "Constants.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Gamma.hh"
#include "G4StepPoint.hh"
#include "G4ThreeVector.hh"

#include <cmath>

namespace CTTwin
{

SensitiveDetector::SensitiveDetector(const G4String& name)
  : G4VSensitiveDetector(name)
{}

// Called by Geant4 at the start of every event — reset the per-event counts.
void SensitiveDetector::Initialize(G4HCofThisEvent*)
{
  fCount = 0;
  fUnscatteredCount = 0;
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

  if (pixel != 0) return true;

  fCount++;

  // ─── CTTWIN START: Pass 3 unscattered-primary discrimination ───
  // "Unscattered" here means: this photon is a primary AND it has undergone no
  // interaction at all between the source and this boundary.
  //
  // A photon that has never interacted arrives with exactly its launch energy
  // and exactly its launch direction — transportation changes neither. So the
  // test is a direct comparison against both, with tolerances that exist only
  // to avoid floating-point equality (see Constants.hh).
  //
  // Both gates are needed, and this is the reason:
  //   * Compton scattering changes direction AND energy -> either gate catches it.
  //   * Rayleigh scattering is ELASTIC. It deflects the photon but takes no
  //     energy. An energy-only gate would count Rayleigh-scattered photons as
  //     unscattered. The direction gate is what excludes them.
  // An energy-window count (what a real NaI photopeak would actually deliver)
  // is a different, detector-response question and belongs with the NaI model
  // in Pass 6+, not here.
  if (track->GetParentID() == 0) {
    const G4double dE = std::fabs(pre->GetKineticEnergy() -
                                  Physics::kCs137GammaEnergy);
    const G4double dCos = 1.0 - pre->GetMomentumDirection().x();

    if (dE < Physics::kUnscatteredEnergyTol &&
        dCos < Physics::kUnscatteredCosTol) {
      fUnscatteredCount++;
    }
  }
  // ─── CTTWIN END ───

  return true;
}

}  // namespace CTTwin
