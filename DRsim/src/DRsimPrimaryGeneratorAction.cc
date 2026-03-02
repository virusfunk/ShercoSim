#include "DRsimPrimaryGeneratorAction.hh"
#include "DRsimRunAction.hh"
#include "DRsimDetectorConstruction.hh"

#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4GenericMessenger.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"
#include "Randomize.hh"

#include <cmath>

namespace { G4Mutex DRsimPrimaryGeneratorMutex = G4MUTEX_INITIALIZER; }
int DRsimPrimaryGeneratorAction::sNumEvt = 0;
G4ThreadLocal int DRsimPrimaryGeneratorAction::sIdxEvt = 0;

using namespace std;

DRsimPrimaryGeneratorAction::DRsimPrimaryGeneratorAction(
    G4int seed, G4bool useHepMC, G4bool useCalib,
    G4bool useGPS, G4bool useCORSIKA, G4String corsikaDATpath)
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(nullptr), fGPS(nullptr),
  fCORSIKAReader(nullptr), fMessenger(nullptr)
{
  fSeed        = seed;
  fUseHepMC    = useHepMC;
  fUseCalib    = useCalib;
  fUseGPS      = useGPS;
  fUseCORSIKA  = useCORSIKA;
  fCORSIKAPath = corsikaDATpath;

  if (fUseCORSIKA) {
    if (!fCORSIKAPath.empty()) {
      fCORSIKAReader = new CORSIKAReader(fCORSIKAPath.c_str());
      if (!fCORSIKAReader->IsOpen()) {
        G4cerr << "DRsimPrimaryGeneratorAction: failed to open CORSIKA file: "
               << fCORSIKAPath << G4endl;
      }
    }
  } else if (fUseGPS) {
    initGPS();
  } else if (!fUseHepMC) {
    initPtcGun();
  }
}

void DRsimPrimaryGeneratorAction::SetCORSIKAFile(G4String path) {
  fCORSIKAPath = path;
  delete fCORSIKAReader;
  fCORSIKAReader = new CORSIKAReader(path.c_str());
  if (!fCORSIKAReader->IsOpen()) {
    G4cerr << "DRsimPrimaryGeneratorAction: failed to open CORSIKA file: "
           << path << G4endl;
  }
}

void DRsimPrimaryGeneratorAction::initPtcGun() {
  fTheta = -0.01111;
  fPhi   = 0.;
  fRandX = 10.*mm;
  fRandY = 10.*mm;
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

  DefineCommands();
}

void DRsimPrimaryGeneratorAction::initGPS() {
  fGPS = new G4GeneralParticleSource();
}

DRsimPrimaryGeneratorAction::~DRsimPrimaryGeneratorAction() {
  delete fCORSIKAReader;
  if (fUseGPS) {
    delete fGPS;
  } else if (!fUseHepMC && !fUseCORSIKA) {
    delete fParticleGun;
    delete fMessenger;
  }
}

// ── GeneratePrimaries ──────────────────────────────────────────────────────
void DRsimPrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {

  // ── GPS mode ──
  if (fUseGPS) {
    G4AutoLock lock(&DRsimPrimaryGeneratorMutex);
    fGPS->GeneratePrimaryVertex(event);
    sIdxEvt = sNumEvt++;
    return;
  }

  // ── HepMC mode ──
#ifdef USE_HEPMC3
  if (fUseHepMC) {
    G4AutoLock lock(&DRsimPrimaryGeneratorMutex);
    DRsimRunAction::sHepMCreader->GeneratePrimaryVertex(event);
    sIdxEvt = sNumEvt++;
    return;
  }
#endif

  // ── CORSIKA mode ──
  if (fUseCORSIKA) {
    if (!fCORSIKAReader || !fCORSIKAReader->IsOpen()) {
      G4cerr << "CORSIKAReader not open — stopping run." << G4endl;
      G4RunManager::GetRunManager()->AbortRun();
      return;
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

    for (const auto& part : fCORSIKAEvent.particles) {
      int pdg = CORSIKAReader::CorsikaToPDG(part.code);
      if (pdg == 0) continue;

      G4ParticleDefinition* partDef = particleTable->FindParticle(pdg);
      if (!partDef) continue;

      // ── Coordinate transform: CORSIKA → Geant4 ──
      // CORSIKA: x,y in cm; pz is stored positive for downward particles.
      // Geant4 DRsim: fibers along z-axis; beam enters from front face at
      //   z = -50 mm in +z direction.  pz > 0 → +z → entering from front.
      G4double x_mm =  part.x * 10.;    // cm → mm
      G4double y_mm =  part.y * 10.;    // cm → mm
      G4double z_mm = -55.;             // 5 mm above the front face at -50 mm

      G4double px_MeV =  part.px * GeV;
      G4double py_MeV =  part.py * GeV;
      // CORSIKA stores pz > 0 for downward particles; +z in DRsim = into detector
      G4double pz_MeV =  part.pz * GeV;

      if (px_MeV == 0. && py_MeV == 0. && pz_MeV == 0.) continue;

      // Reject particles outside the world volume's XY boundary
      G4double worldHalf = DRsimDetectorConstruction::GetWorldHalfXY();
      if (std::abs(x_mm) > worldHalf || std::abs(y_mm) > worldHalf) continue;

      G4PrimaryVertex*   vtx = new G4PrimaryVertex(x_mm*mm, y_mm*mm,
                                                    z_mm*mm, part.t*ns);
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
  fDirection.rotateX(-M_PI * (fPhi/180.));
  fParticleGun->SetParticleMomentumDirection(fDirection);

  G4AutoLock lock(&DRsimPrimaryGeneratorMutex);
  fParticleGun->GeneratePrimaryVertex(event);
  sIdxEvt = sNumEvt++;
}

// ── DefineCommands (particle-gun messenger) ────────────────────────────────
void DRsimPrimaryGeneratorAction::DefineCommands() {
  fMessenger = new G4GenericMessenger(this, "/DRsim/generator/",
                                      "Primary generator control");

  auto& thetaCmd = fMessenger->DeclareMethodWithUnit(
      "theta","rad", &DRsimPrimaryGeneratorAction::SetTheta, "theta of beam");
  thetaCmd.SetParameterName("theta", true);
  thetaCmd.SetDefaultValue("0.");

  auto& phiCmd = fMessenger->DeclareMethodWithUnit(
      "phi","rad", &DRsimPrimaryGeneratorAction::SetPhi, "phi of beam");
  phiCmd.SetParameterName("phi", true);
  phiCmd.SetDefaultValue("0.");

  auto& x0Cmd = fMessenger->DeclareMethodWithUnit(
      "x0","cm", &DRsimPrimaryGeneratorAction::SetX0, "x_0 of beam");
  x0Cmd.SetParameterName("x0", true);
  x0Cmd.SetDefaultValue("0.");

  auto& y0Cmd = fMessenger->DeclareMethodWithUnit(
      "y0","cm", &DRsimPrimaryGeneratorAction::SetY0, "y_0 of beam");
  y0Cmd.SetParameterName("y0", true);
  y0Cmd.SetDefaultValue("0.");

  auto& z0Cmd = fMessenger->DeclareMethodWithUnit(
      "z0","cm", &DRsimPrimaryGeneratorAction::SetZ0, "z_0 of beam");
  z0Cmd.SetParameterName("z0", true);
  z0Cmd.SetDefaultValue("0.");

  auto& randxCmd = fMessenger->DeclareMethodWithUnit(
      "randx","mm", &DRsimPrimaryGeneratorAction::SetRandX, "x width of beam");
  randxCmd.SetParameterName("randx", true);
  randxCmd.SetDefaultValue("0.");

  auto& randyCmd = fMessenger->DeclareMethodWithUnit(
      "randy","mm", &DRsimPrimaryGeneratorAction::SetRandY, "y width of beam");
  randyCmd.SetParameterName("randy", true);
  randyCmd.SetDefaultValue("0.");

  auto& randzCmd = fMessenger->DeclareMethodWithUnit(
      "randz","mm", &DRsimPrimaryGeneratorAction::SetRandZ, "z width of beam");
  randzCmd.SetParameterName("randz", true);
  randzCmd.SetDefaultValue("0.");
}
