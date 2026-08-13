#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "Constants.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4AccumulableManager.hh"
#include "G4SystemOfUnits.hh"

#include <iomanip>

namespace CTTwin
{

RunAction::RunAction()
{
  auto* accumulables = G4AccumulableManager::Instance();
  accumulables->RegisterAccumulable(fDetectorCount);
  accumulables->RegisterAccumulable(fUnscatteredCount);
}

void RunAction::BeginOfRunAction(const G4Run*)
{
  G4AccumulableManager::Instance()->Reset();
}

void RunAction::EndOfRunAction(const G4Run* run)
{
  const G4int nEvents = run->GetNumberOfEvent();
  if (nEvents == 0) return;

  // Merge per-thread accumulables into the master.
  G4AccumulableManager::Instance()->Merge();

  if (!IsMaster()) return;

  const G4int counts       = fDetectorCount.GetValue();
  const G4int unscattered  = fUnscatteredCount.GetValue();

  const G4double fracTotal = static_cast<G4double>(counts) / nEvents;
  const G4double fracUnsc  = static_cast<G4double>(unscattered) / nEvents;

  // ─── CTTWIN START: Pass 3 run configuration echo ───
  // Read the geometry configuration back from DetectorConstruction so the
  // summary states what was actually built. A validation table assembled from
  // runs that don't say what they ran is a table nobody can check.
  G4String phantom = "unknown";
  G4double slabMM  = 0.0;
  if (auto* dc = dynamic_cast<const DetectorConstruction*>(
          G4RunManager::GetRunManager()->GetUserDetectorConstruction())) {
    phantom = dc->GetActivePhantom();
    slabMM  = dc->GetSlabThickness() / mm;
  }
  // ─── CTTWIN END ───

  // Fraction of the counted signal that reached the face after scattering.
  // Beer-Lambert describes the unscattered part only; this is the contamination
  // an idealised non-discriminating counter carries. See ADR 0004.
  const G4double scatterFrac =
      (counts > 0) ? (1.0 - static_cast<G4double>(unscattered) / counts) : 0.0;

  G4cout << "\n--------------------- CTTwin run summary ---------------------\n"
         << "  Phantom           : " << phantom;
  if (phantom == "slab") G4cout << "  (t = " << slabMM << " mm)";
  G4cout << "\n"
         << "  Source            : Cs-137, "
         << Physics::kCs137GammaEnergy / keV << " keV\n"
         << "  Events fired      : " << nEvents << "\n"
         << "  Detector counts   : " << counts
         << "   (fraction " << std::setprecision(6) << fracTotal << ")\n"
         << "  Unscattered       : " << unscattered
         << "   (fraction " << std::setprecision(6) << fracUnsc << ")\n"
         << "  Scatter in count  : " << std::setprecision(4)
         << scatterFrac * 100.0 << " %\n"
         << "--------------------------------------------------------------\n"
         << G4endl;

  // ─── CTTWIN START: Pass 3 machine-readable result line ───
  // Single line, key=value, stable field names. python/validate_beer_lambert.py
  // greps for the CTTWIN_RESULT prefix. Do not reformat casually — the parser
  // and the Pass 3 validation table both depend on it.
  G4cout << "CTTWIN_RESULT"
         << " phantom=" << phantom
         << " slab_mm=" << slabMM
         << " energy_keV=" << Physics::kCs137GammaEnergy / keV
         << " events=" << nEvents
         << " total=" << counts
         << " unscattered=" << unscattered
         << G4endl;
  // ─── CTTWIN END ───
}

}  // namespace CTTwin