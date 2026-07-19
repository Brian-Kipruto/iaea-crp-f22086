#ifndef CTTWIN_PrimaryGeneratorAction_h
#define CTTWIN_PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;

namespace CTTwin
{

/// Fires gamma primaries. Pass 0 is a placeholder: a single mono-directional
/// gamma along +x from the source position. The Cs-137 662 keV energy and the
/// collimated pencil beam land in Pass 2/3 — v1's Ir-192 spectrum and 30-deg
/// cone were deliberately NOT ported.
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
