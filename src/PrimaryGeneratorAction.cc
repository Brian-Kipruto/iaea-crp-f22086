#include "PrimaryGeneratorAction.hh"
#include "Constants.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"

namespace CTTwin
{

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  fParticleGun = new G4ParticleGun(1);
  auto* gamma = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
  fParticleGun->SetParticleDefinition(gamma);

  // ─── CTTWIN START: Pass 0 placeholder beam ───
  // Placeholder only — a single gamma along +x so the app runs end-to-end.
  // Pass 2 replaces this with a collimated pencil beam; Pass 3 sets Cs-137
  // 662 keV. v1's Ir-192 3-line spectrum and 30-deg cone were NOT ported
  // (both on the "must not survive" list).
  fParticleGun->SetParticleEnergy(662.0 * keV);
  fParticleGun->SetParticlePosition(G4ThreeVector(-Geometry::kSourceToIso, 0, 0));
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(1, 0, 0));
  // ─── CTTWIN END ───
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  fParticleGun->GeneratePrimaryVertex(anEvent);
}

}  // namespace CTTwin
