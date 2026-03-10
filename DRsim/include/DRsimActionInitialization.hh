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
  static G4bool   GetCenterCore()   { return sCenterCore; }
  // centerMode: "none" | "core" (EVTH x/y) | "energy" (highest-p particle) | "random" (uniform scatter)
  static G4String GetCenterMode()   { return sCenterMode; }

private:
  void DefineCommands();
  void SetUseCORSIKACmd(G4bool v);
  void SetCORSIKAFileCmd(G4String v);
  void SetCORSIKASkipCmd(G4int v);
  void SetCenterCoreCmd(G4bool v);
  void SetCenterModeCmd(G4String v);

  G4GenericMessenger* fMessenger;
  G4GenericMessenger* fCORSIKAMessenger;
  G4int fSeed;
  G4String fFilename;
  G4bool fUseHepMC;
  G4bool fUseCalib;
  G4bool fUseGPS;

  // These are instance vars updated by the messenger; mirrored into statics.
  G4bool   fUseCORSIKA;
  G4String fCORSIKAFile;
  G4int    fCORSIKASkip;
  G4bool   fCORSIKACenterCore;   // legacy
  G4String fCORSIKACenterMode;   // "none" | "core" | "energy" | "random"

  // Statics: set by the messenger after Build() has already run.
  static G4bool   sCORSIKAEnabled;
  static G4String sCORSIKAFile;
  static G4int    sCORSIKASkip;
  static G4bool   sCenterCore;    // legacy bool (maps to sCenterMode)
  static G4String sCenterMode;    // "none" | "core" | "energy" | "random"
};

#endif
