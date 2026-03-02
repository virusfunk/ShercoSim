#ifndef DRsimRunAction_h
#define DRsimRunAction_h 1

#include "RootInterface.h"
#include "DRsimInterface.h"
#ifdef USE_HEPMC3
#include "HepMCG4Reader.hh"
#endif

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;

class DRsimRunAction : public G4UserRunAction {
public:
  DRsimRunAction(G4int seed, G4String filename, G4bool useHepMC);
  virtual ~DRsimRunAction();

  virtual void BeginOfRunAction(const G4Run*);
  virtual void EndOfRunAction(const G4Run*);

#ifdef USE_HEPMC3
  static HepMCG4Reader* sHepMCreader;
#endif
  static RootInterface<DRsimInterface::DRsimEventData>* sRootIO;
  static int sNumEvt;

private:
  G4int fSeed;
  G4String fFilename;
  G4bool fUseHepMC;
};

#endif
