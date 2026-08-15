#ifndef CTTWIN_RunAction_h
#define CTTWIN_RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

class G4Run;

namespace CTTwin
{

/// Accumulates the detector photon counts across all events in a run and prints
/// the totals. Same wiring SHAPE as v1's dose chain (AddEdep/G4Accumulable),
/// with the quantity swapped from G4double energy to G4int count.
///
/// Pass 3 adds a second accumulable for unscattered primaries, and a single
/// machine-readable summary line (CTTWIN_RESULT) so python/validate_beer_lambert.py
/// can parse a run without screen-scraping the human-readable block. The human
/// block stays: it is what gets read during a manual checkpoint.
class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void AddCount(G4int n)      { fDetectorCount += n; }
    void AddUnscattered(G4int n) { fUnscatteredCount += n; }

  private:
    // ─── CTTWIN START: Pass 5 projection output ───
    /// Append one measurement to the per-projection CSV. Master thread only,
    /// called once per run from EndOfRunAction. Writes the header if and only
    /// if the file is new or empty, so repeated runs in one process accumulate
    /// into a single file. See [[Output Format]].
    void WriteProjectionRow(const G4String& path, G4int projectionId,
                            G4double angleDeg, G4double translationMM,
                            G4int total, G4int unscattered, G4int events) const;
    // ─── CTTWIN END ───

    G4Accumulable<G4int> fDetectorCount{0};
    G4Accumulable<G4int> fUnscatteredCount{0};
};

}  // namespace CTTwin

#endif
