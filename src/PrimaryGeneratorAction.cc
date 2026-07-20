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

  // ─── CTTWIN START: Pass 2 collimated pencil beam ───
  // A single, zero-width gamma ray — the idealised pencil beam of a 1st-gen
  // translate-rotate CT. Every photon is "useful": it travels one defined path
  // from source to the in-line detector pixel, so the transmitted count is a
  // clean line integral (the basis for Pass 3 Beer-Lambert validation). This
  // is the deliberate abstraction, NOT the Pass 0 incidental ray.
  //
  // Geometry (see include/Constants.hh, [[Coordinate Conventions]]):
  //   +x  = source->detector line at theta = 0. Source on -x, detector on +x.
  //   Source sits on the source plane at x = -kSourceToIso, on-axis (y=z=0).
  //   Aim is +x, straight at the detector centre placed at x = +kIsoToDetector,
  //   which lands the ray on the in-line pixel (pixel 0, the whole face today).
  //
  // Idealisations (locked for Phase 1, deferred realism noted):
  //   * Zero angular spread — real collimators diverge <1 deg. Divergence is a
  //     later realism mode (Pass 6+), NOT Phase 1. See [[Source - Cs-137]].
  //   * Zero beam-spot width — a point ray, so one path length per projection.
  //     A finite spot would sample a spread of chords and blur Beer-Lambert.
  //   * Source-Y translation (scan motion) is fixed at 0 here; the SetSourceY
  //     hook arrives in Pass 4 with the DetectorMessenger that drives it.
  //
  // Energy: Cs-137's 662 keV single line. Set here so the app is physically
  // meaningful now; formalised as a locked source choice in Pass 3.
  fParticleGun->SetParticleEnergy(662.0 * keV);
  fParticleGun->SetParticlePosition(
      G4ThreeVector(-Geometry::kSourceToIso, 0.0, 0.0));
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(1.0, 0.0, 0.0));
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
