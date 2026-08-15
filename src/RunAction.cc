#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "Constants.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4AccumulableManager.hh"
#include "G4SystemOfUnits.hh"

#include <iomanip>
#include <fstream>

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
  // ─── CTTWIN START: Pass 4 scan coordinates ───
  // Read back for the same reason: a sinogram assembled from runs that do not
  // record their own (theta, t) is a sinogram nobody can check or re-order.
  G4double angleDeg = 0.0;
  G4double transMM  = 0.0;
  // ─── CTTWIN END ───
  // ─── CTTWIN START: Pass 5 output configuration ───
  G4String outputFile;
  G4int    projectionId = 0;
  // ─── CTTWIN END ───
  if (auto* dc = dynamic_cast<const DetectorConstruction*>(
          G4RunManager::GetRunManager()->GetUserDetectorConstruction())) {
    phantom      = dc->GetActivePhantom();
    slabMM       = dc->GetSlabThickness() / mm;
    angleDeg     = dc->GetScanAngle() / deg;
    transMM      = dc->GetScanTranslation() / mm;
    outputFile   = dc->GetOutputFile();
    projectionId = dc->GetProjectionId();
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
         << "  Scan              : theta = " << angleDeg << " deg,"
         << "  t = " << transMM << " mm\n"
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
  // Pass 4 adds angle_deg and translation_mm. Fields are APPENDED, never
  // renamed or removed: validate_beer_lambert.py builds a key->value dict from
  // this line and looks fields up by name, so additions are transparent to it.
  // The precision is raised here so the scan coordinates survive the round trip
  // (four significant figures would round t = 123.45 mm to 123.5).
  G4cout << std::setprecision(9)
         << "CTTWIN_RESULT"
         << " phantom=" << phantom
         << " slab_mm=" << slabMM
         << " angle_deg=" << angleDeg
         << " translation_mm=" << transMM
         << " energy_keV=" << Physics::kCs137GammaEnergy / keV
         << " events=" << nEvents
         << " total=" << counts
         << " unscattered=" << unscattered
         << G4endl;
  // ─── CTTWIN END ───

  // ─── CTTWIN START: Pass 5 per-projection CSV ───
  // Only if a path was set. Unset is the default, so every Pass 1-4 macro and
  // validate_beer_lambert.py are untouched by this.
  if (!outputFile.empty()) {
    WriteProjectionRow(outputFile, projectionId, angleDeg, transMM,
                       counts, unscattered, nEvents);
  }
  // ─── CTTWIN END ───
}

// ─── CTTWIN START: Pass 5 per-projection CSV ───
// Column set is [[Output Format]]'s, with two appended:
//
//   projection_id,angle_deg,translation_mm,pixel_index,n_counts,n_unscattered,n_events
//
//   * n_unscattered — there are two counts now, and the scatter-free sinogram
//     is the comparison figure for the paper. Deriving it later is impossible;
//     recording it costs one integer.
//   * n_events — line integrals are -ln(N/N0), so the normalisation has to
//     travel with the measurement. A row that does not state how many photons
//     produced it cannot be combined with a row taken at different statistics.
//
// pixel_index is always 0 in Phase 1. The detector is one pixel (Architecture
// Lockdown #2, [[Detector Model]]) and under ADR 0005 the ray always lands on
// its centre. NOTE FOR PASS 5: [[Output Format]] describes the sinogram as
// (n_angles, n_pixels) — for a true first-generation scanner that second axis
// is the TRANSLATION axis. The column stays so the format survives contact
// with a multi-pixel detector in Pass 6+.
void RunAction::WriteProjectionRow(const G4String& path, G4int projectionId,
                                   G4double angleDeg, G4double translationMM,
                                   G4int total, G4int unscattered,
                                   G4int events) const
{
  // Header only for a new or empty file, so successive runs in one process
  // accumulate into one file per angle instead of one file per measurement.
  G4bool needHeader = true;
  {
    std::ifstream probe(path, std::ios::ate);
    if (probe && probe.tellg() > 0) needHeader = false;
  }

  std::ofstream out(path, std::ios::app);
  if (!out) {
    // Loud, because a scan that silently writes nothing looks exactly like a
    // scan that worked until the sinogram comes out empty hours later. The
    // usual cause is a parent directory that does not exist relative to the
    // working directory cttwin was launched from.
    G4cerr << "\n*** CTTwin ERROR: cannot open projection output file <"
           << path << ">.\n"
           << "    This run's counts were NOT written. Check that the parent\n"
           << "    directory exists relative to the working directory.\n"
           << G4endl;
    return;
  }

  if (needHeader) {
    out << "projection_id,angle_deg,translation_mm,pixel_index,"
        << "n_counts,n_unscattered,n_events\n";
  }

  out << projectionId << ','
      << std::setprecision(10) << angleDeg << ','
      << std::setprecision(10) << translationMM << ','
      << 0 << ','
      << total << ','
      << unscattered << ','
      << events << '\n';

  G4cout << "[CTTwin] wrote projection " << projectionId << " -> " << path
         << G4endl;
}
// ─── CTTWIN END ───

}  // namespace CTTwin