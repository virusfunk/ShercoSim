#ifndef DRsimActionInitialization_h
#define DRsimActionInitialization_h 1

#include "G4VUserActionInitialization.hh"
#include "globals.hh"

class G4GenericMessenger;

class DRsimActionInitialization : public G4VUserActionInitialization {
public:
  DRsimActionInitialization(G4int seed, G4String filename);
  virtual ~DRsimActionInitialization();

  virtual void BuildForMaster() const;
  virtual void Build() const;

  // Static getters: PGA reads these in GeneratePrimaries() after macro has run.
  // Using statics avoids messenger conflicts and the Build()-before-macro problem.
  static G4bool   GetUseCORSIKA()   { return sCORSIKAEnabled; }
  static G4String GetCORSIKAFile()  { return sCORSIKAFile; }
  static G4int    GetCORSIKASkip()  { return sCORSIKASkip; }
  static G4String GetCenterMode()   { return sCenterMode; }

private:
  void DefineCommands();
  void SetUseCORSIKA(G4bool v);
  void SetCORSIKAFile(G4String v);
  void SetCORSIKASkip(G4int v);
  void SetCenterMode(G4String v);

  G4GenericMessenger* fMessenger;
  G4GenericMessenger* fCORSIKAMessenger;
  G4int fSeed;
  G4String fFilename;
  G4bool fUseHepMC;
  G4bool fUseCalib;
  G4bool fUseGPS;

  // Instance vars updated by messenger; mirrored into statics.
  G4bool   fUseCORSIKA;
  G4String fCORSIKAFile;
  G4int    fCORSIKASkip;
  G4String fCORSIKACenterMode;

  // Statics: set by the messenger after Build() has already run.
  static G4bool   sCORSIKAEnabled;
  static G4String sCORSIKAFile;
  static G4int    sCORSIKASkip;
  static G4String sCenterMode;
};

#endif
