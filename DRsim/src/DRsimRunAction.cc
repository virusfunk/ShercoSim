#include "DRsimRunAction.hh"
#include "DRsimEventAction.hh"
#include "G4AutoLock.hh"
#include "G4Threading.hh"

#include <vector>

using namespace std;

namespace { G4Mutex DRsimRunActionMutex = G4MUTEX_INITIALIZER; }
#ifdef USE_HEPMC3
HepMCG4Reader* DRsimRunAction::sHepMCreader = 0;
#endif
RootInterface<DRsimInterface::DRsimEventData>* DRsimRunAction::sRootIO = 0;
int DRsimRunAction::sNumEvt = 0;

DRsimRunAction::DRsimRunAction(G4int seed, G4String filename, G4bool useHepMC)
: G4UserRunAction()
{
  fSeed = seed;
  fFilename = filename;
  fUseHepMC = useHepMC;

  G4AutoLock lock(&DRsimRunActionMutex);

  if (!sRootIO) {
    sRootIO = new RootInterface<DRsimInterface::DRsimEventData>(fFilename+"_"+std::to_string(fSeed)+".root", true);
    sRootIO->create("DRsim","DRsimEventData");
  }

#ifdef USE_HEPMC3
  if (fUseHepMC && !sHepMCreader) {
    sHepMCreader = new HepMCG4Reader(fSeed,fFilename);
  }
#endif
}

DRsimRunAction::~DRsimRunAction() {
  if (IsMaster()) {
    G4AutoLock lock(&DRsimRunActionMutex);

#ifdef USE_HEPMC3
    if (fUseHepMC && sHepMCreader) {
      delete sHepMCreader;
      sHepMCreader = 0;
    }
#endif

    if (sRootIO) {
      sRootIO->write();
      sRootIO->close();
      delete sRootIO;
      sRootIO = 0;
    }
  }
}

void DRsimRunAction::BeginOfRunAction(const G4Run*) {

}

void DRsimRunAction::EndOfRunAction(const G4Run*) {

}
