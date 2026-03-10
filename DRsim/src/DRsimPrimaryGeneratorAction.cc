#include "DRsimPrimaryGeneratorAction.hh"
#include "DRsimRunAction.hh"
#include "DRsimDetectorConstruction.hh"
#include "DRsimActionInitialization.hh"

#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4GenericMessenger.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"
#include "Randomize.hh"

#include <cmath>
#include <limits>

namespace { G4Mutex DRsimPrimaryGeneratorMutex = G4MUTEX_INITIALIZER; }
int DRsimPrimaryGeneratorAction::sNumEvt = 0;
G4ThreadLocal int DRsimPrimaryGeneratorAction::sIdxEvt = 0;

using namespace std;

DRsimPrimaryGeneratorAction::DRsimPrimaryGeneratorAction(G4int seed, G4bool useHepMC, G4bool useCalib, G4bool useGPS)
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(nullptr), fGPS(nullptr),
  fCORSIKAReader(nullptr), fMessenger(nullptr), fCORSIKAMessenger(nullptr)
{
  fSeed           = seed;
  fUseHepMC       = useHepMC;
  fUseCalib       = useCalib;
  fUseGPS         = useGPS;
  fUseCORSIKA     = false;
  fCORSIKAPath    = "";
  fCORSIKASkip    = 0;
  fCORSIKASkipped = false;
  fCenterMode     = "core";

  if (!fUseHepMC) {
    if (fUseGPS) initGPS();
    else initPtcGun();
  }
}

void DRsimPrimaryGeneratorAction::initPtcGun() {
  fTheta = -0.01111;
  fPhi   = 0.;
  fRandX = 10.*mm;
  fRandY = 10.*mm;
  fRandZ = 10.*mm;
  fX_0   = 0.;
  fY_0   = 0.;
  fZ_0   = 0.;
  fParticleGun = new G4ParticleGun(1);

  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4String particleName;
  fElectron = particleTable->FindParticle(particleName="e-");
  fPositron = particleTable->FindParticle(particleName="e+");
  fMuon     = particleTable->FindParticle(particleName="mu+");
  fPion     = particleTable->FindParticle(particleName="pi+");
  fKaon0L   = particleTable->FindParticle(particleName="kaon0L");
  fProton   = particleTable->FindParticle(particleName="proton");
  fOptGamma = particleTable->FindParticle(particleName="opticalphoton");

  // define commands for this class
  DefineCommands();
}

void DRsimPrimaryGeneratorAction::initGPS() {
  fGPS = new G4GeneralParticleSource();
}

DRsimPrimaryGeneratorAction::~DRsimPrimaryGeneratorAction() {
  delete fCORSIKAReader;
  if (fUseGPS) {
    delete fGPS;
  } else if (!fUseHepMC) {
    delete fParticleGun;
    delete fMessenger;
  }
  delete fCORSIKAMessenger;
}

// ── GeneratePrimaries ──────────────────────────────────────────────────────
void DRsimPrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {

  // ── GPS mode ──
  if (fUseGPS) {
    G4AutoLock lock(&DRsimPrimaryGeneratorMutex);
    fGPS->GeneratePrimaryVertex(event);
    sIdxEvt = sNumEvt;
    sNumEvt++;
    return;
  }

  // ── HepMC mode ──
  if (fUseHepMC) {
    G4AutoLock lock(&DRsimPrimaryGeneratorMutex);
    DRsimRunAction::sHepMCreader->GeneratePrimaryVertex(event);
    sIdxEvt = sNumEvt;
    sNumEvt++;
    return;
  }

  // ── CORSIKA mode ──
  // Read settings from DRsimActionInitialization statics, which are set by the
  // macro AFTER Build() has already run (Build() runs before macro in Geant4).
  G4bool   useCORSIKA = DRsimActionInitialization::GetUseCORSIKA();
  G4String corsPath   = DRsimActionInitialization::GetCORSIKAFile();
  G4int    corsSkip   = DRsimActionInitialization::GetCORSIKASkip();
  G4String centerMode = DRsimActionInitialization::GetCenterMode();

  if (useCORSIKA) {
    // Lazy-open the reader on first call.
    if (!fCORSIKAReader && !corsPath.empty()) {
      fCORSIKAReader = new CORSIKAReader(corsPath.c_str());
      fCORSIKASkip   = corsSkip;
      fCenterMode    = centerMode;
    }
    if (!fCORSIKAReader || !fCORSIKAReader->IsOpen()) {
      G4cerr << "CORSIKAReader not open (path=\"" << corsPath
             << "\") — stopping run." << G4endl;
      G4RunManager::GetRunManager()->AbortRun();
      return;
    }

    // Skip events on first call (for single-event jobs to select different showers)
    if (!fCORSIKASkipped) {
      if (fCORSIKASkip > 0) {
        G4cout << "[CORSIKAReader] Skipping " << fCORSIKASkip << " events." << G4endl;
        fCORSIKAReader->SkipEvents(fCORSIKASkip);
      }
      fCORSIKASkipped = true;
    }

    {
      G4AutoLock lock(&DRsimPrimaryGeneratorMutex);
      if (!fCORSIKAReader->ReadNextEvent(fCORSIKAEvent)) {
        G4cout << "CORSIKAReader: EOF reached — stopping run." << G4endl;
        G4RunManager::GetRunManager()->AbortRun();
        return;
      }
      sIdxEvt = sNumEvt++;
    }

    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();

    // ── Shower centering ──
    // centerMode="core": use EVTH(98/99) to subtract shower core → centre at (0,0)
    G4double x_core = 0., y_core = 0.;
    G4double worldHalfXY = DRsimDetectorConstruction::GetWorldHalfXY();

    if (fCenterMode == "core") {
      x_core = fCORSIKAEvent.x_core_cm * 10.;   // cm → mm
      y_core = fCORSIKAEvent.y_core_cm * 10.;
      G4cout << "[CORSIKA] centerMode=core: EVTH core ("
             << x_core << ", " << y_core << ") mm → centred at (0,0)" << G4endl;
    } else if (fCenterMode == "energy") {
      float best_pmag = -1.f;
      for (const auto& p : fCORSIKAEvent.particles) {
        if (CORSIKAReader::CorsikaToPDG(p.code) == 0) continue;
        float pmag = std::sqrt(p.px*p.px + p.py*p.py + p.pz*p.pz);
        if (pmag > best_pmag) {
          best_pmag = pmag;
          x_core = p.x * 10.;
          y_core = p.y * 10.;
        }
      }
      G4cout << "[CORSIKA] centerMode=energy: highest-p particle ("
             << x_core << ", " << y_core << ") mm, |p|=" << best_pmag
             << " GeV/c → centred at (0,0)" << G4endl;
    } else if (fCenterMode == "random") {
      G4double rx = (G4UniformRand() - 0.5) * worldHalfXY;
      G4double ry = (G4UniformRand() - 0.5) * worldHalfXY;
      x_core = fCORSIKAEvent.x_core_cm * 10. - rx;
      y_core = fCORSIKAEvent.y_core_cm * 10. - ry;
      G4cout << "[CORSIKA] centerMode=random: shower core placed at ("
             << rx << ", " << ry << ") mm in detector frame" << G4endl;
    }

    // ── Shower-front time reference ──
    float t_min = std::numeric_limits<float>::max();
    for (const auto& part : fCORSIKAEvent.particles) {
      if (CORSIKAReader::CorsikaToPDG(part.code) == 0) continue;
      if (std::abs(part.x * 10. - x_core) > worldHalfXY) continue;
      if (std::abs(part.y * 10. - y_core) > worldHalfXY) continue;
      if (t_min > part.t) t_min = part.t;
    }
    if (t_min == std::numeric_limits<float>::max()) t_min = 0.f;

    G4cout << "[CORSIKAReader] Event " << fCORSIKAEvent.event_id
           << "  E=" << fCORSIKAEvent.energy_GeV << " GeV"
           << "  zenith=" << fCORSIKAEvent.zenith_rad * (180./M_PI) << " deg"
           << "  azimuth=" << fCORSIKAEvent.azimuth_rad * (180./M_PI) << " deg"
           << "  N_particles=" << fCORSIKAEvent.particles.size()
           << "  t_min=" << t_min << " ns"
           << G4endl;

    for (const auto& part : fCORSIKAEvent.particles) {
      int pdg = CORSIKAReader::CorsikaToPDG(part.code);
      if (pdg == 0) continue;

      G4ParticleDefinition* partDef = particleTable->FindParticle(pdg);
      if (!partDef && pdg > 1000000000) {
        int Z = (pdg / 10000) % 1000;
        int A = (pdg / 10)    % 1000;
        partDef = G4IonTable::GetIonTable()->GetIon(Z, A, 0.);
      }
      if (!partDef) continue;

      // ── Coordinate transform: CORSIKA → Geant4 ──
      G4double x_mm =  part.x * 10. - x_core;
      G4double y_mm =  part.y * 10. - y_core;
      G4double z_mm = -(DRsimDetectorConstruction::GetTowerHalfZ() + 5.);

      G4double px_MeV =  part.px * GeV;
      G4double py_MeV =  part.py * GeV;
      G4double pz_MeV =  part.pz * GeV;   // pz>0 downward = +z into detector

      if (px_MeV == 0. && py_MeV == 0. && pz_MeV == 0.) continue;

      // Reject particles outside the world volume's XY boundary
      G4double worldHalf = DRsimDetectorConstruction::GetWorldHalfXY();
      if (std::abs(x_mm) > worldHalf || std::abs(y_mm) > worldHalf) continue;

      G4double t_rel = (part.t - t_min) * ns;

      G4PrimaryVertex*   vtx = new G4PrimaryVertex(x_mm*mm, y_mm*mm,
                                                    z_mm*mm, t_rel);
      G4PrimaryParticle* primary = new G4PrimaryParticle(partDef,
                                                          px_MeV, py_MeV,
                                                          pz_MeV);
      vtx->SetPrimary(primary);
      event->AddPrimaryVertex(vtx);
    }
    return;
  }

  // ── Particle-gun mode ──
  G4double x = (G4UniformRand()-0.5)*fRandX + fX_0;
  G4double y = (G4UniformRand()-0.5)*fRandY + fY_0;
  G4double z = (G4UniformRand()-0.5)*fRandZ + fZ_0;
  fOrg.set(x, y, z);

  fParticleGun->SetParticlePosition(fOrg);

  fDirection.setREtaPhi(1., 0., 0.);
  fDirection.rotateY(-M_PI * ((90. - fTheta)/180.));
  fDirection.rotateZ(M_PI * (fPhi/180.));
  fDirection.rotateX(-M_PI * (fPhi/180.));
  fParticleGun->SetParticleMomentumDirection(fDirection);

  G4AutoLock lock(&DRsimPrimaryGeneratorMutex);
  fParticleGun->GeneratePrimaryVertex(event);
  sIdxEvt = sNumEvt;
  sNumEvt++;
}

// ── DefineCommands (particle-gun messenger) ────────────────────────────────
void DRsimPrimaryGeneratorAction::DefineCommands() {
  fMessenger = new G4GenericMessenger(this, "/DRsim/generator/",
                                      "Primary generator control");

  G4GenericMessenger::Command& thetaCmd = fMessenger->DeclareMethodWithUnit(
      "theta","rad", &DRsimPrimaryGeneratorAction::SetTheta, "theta of beam");
  thetaCmd.SetParameterName("theta", true);
  thetaCmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& phiCmd = fMessenger->DeclareMethodWithUnit(
      "phi","rad", &DRsimPrimaryGeneratorAction::SetPhi, "phi of beam");
  phiCmd.SetParameterName("phi", true);
  phiCmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& x0Cmd = fMessenger->DeclareMethodWithUnit(
      "x0","cm", &DRsimPrimaryGeneratorAction::SetX0, "x_0 of beam");
  x0Cmd.SetParameterName("x0", true);
  x0Cmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& y0Cmd = fMessenger->DeclareMethodWithUnit(
      "y0","cm", &DRsimPrimaryGeneratorAction::SetY0, "y_0 of beam");
  y0Cmd.SetParameterName("y0", true);
  y0Cmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& z0Cmd = fMessenger->DeclareMethodWithUnit(
      "z0","cm", &DRsimPrimaryGeneratorAction::SetZ0, "z_0 of beam");
  z0Cmd.SetParameterName("z0", true);
  z0Cmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& randxCmd = fMessenger->DeclareMethodWithUnit(
      "randx","mm", &DRsimPrimaryGeneratorAction::SetRandX, "x width of beam");
  randxCmd.SetParameterName("randx", true);
  randxCmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& randyCmd = fMessenger->DeclareMethodWithUnit(
      "randy","mm", &DRsimPrimaryGeneratorAction::SetRandY, "y width of beam");
  randyCmd.SetParameterName("randy", true);
  randyCmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& randzCmd = fMessenger->DeclareMethodWithUnit(
      "randz","mm", &DRsimPrimaryGeneratorAction::SetRandZ, "z width of beam");
  randzCmd.SetParameterName("randz", true);
  randzCmd.SetDefaultValue("0.");

  // Note: CORSIKA commands (/DRsim/corsika/ and /DRsim/action/useCORSIKA)
  // are registered by DRsimActionInitialization to avoid double-registration.
  // This class reads those settings lazily via DRsimActionInitialization statics.
}
