#ifndef CTTWIN_PrimaryGeneratorAction_h
#define CTTWIN_PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;

namespace CTTwin
{

/// Fires gamma primaries as a collimated pencil beam (Pass 2): a single,
/// zero-width mono-directional gamma along +x, launched from the source plane
/// at x = -kSourceToIso and aimed at the in-line detector pixel. This makes
/// each projection a clean line integral (Beer-Lambert path), the basis for
/// Pass 3 validation. v1's Ir-192 3-line spectrum and 30-deg cone were
/// deliberately NOT ported. Energy is fixed at Cs-137 662 keV (formalised in
/// Pass 3). Beam divergence and source-Y translation are deferred: divergence
/// to a later realism mode (Pass 6+, alongside NaI detector response), the
/// translation hook to Pass 4 with the messenger that drives it.
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event*) override;

    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }

  private:
    G4ParticleGun* fParticleGun = nullptr;
};

}  // namespace CTTwin

#endif
