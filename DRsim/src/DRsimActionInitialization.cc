#include "DRsimActionInitialization.hh"
#include "DRsimPrimaryGeneratorAction.hh"
#include "DRsimRunAction.hh"
#include "DRsimEventAction.hh"
#include "DRsimSteppingAction.hh"

#include "G4GenericMessenger.hh"

using namespace std;

// Static storage — updated by messenger AFTER Build() has run.
G4bool   DRsimActionInitialization::sCORSIKAEnabled = false;
G4String DRsimActionInitialization::sCORSIKAFile    = "";
G4int    DRsimActionInitialization::sCORSIKASkip    = 0;
G4bool   DRsimActionInitialization::sCenterCore     = false;
G4String DRsimActionInitialization::sCenterMode     = "none";

DRsimActionInitialization::DRsimActionInitialization(G4int seed, G4String filename)
: G4VUserActionInitialization(),
  fUseCORSIKA(false), fCORSIKAFile(""), fCORSIKASkip(0), fCORSIKACenterCore(false),
  fCORSIKACenterMode("none"), fCORSIKAMessenger(nullptr)
{
  fSeed = seed;
  fFilename = filename;

  DefineCommands();
}

DRsimActionInitialization::~DRsimActionInitialization() {
  if (fMessenger) delete fMessenger;
  if (fCORSIKAMessenger) delete fCORSIKAMessenger;
}

void DRsimActionInitialization::BuildForMaster() const {
  SetUserAction(new DRsimRunAction(fSeed,fFilename,fUseHepMC));
}

void DRsimActionInitialization::Build() const {
  // Note: Build() is called immediately by SetUserInitialization(), before the
  // macro runs. CORSIKA settings are passed as defaults here and read lazily
  // from the static getters in GeneratePrimaries() after the macro sets them.
  DRsimPrimaryGeneratorAction* pga = new DRsimPrimaryGeneratorAction(fSeed,fUseHepMC,fUseCalib,fUseGPS,false,"");
  SetUserAction(pga);
  SetUserAction(new DRsimRunAction(fSeed,fFilename,fUseHepMC));

  DRsimEventAction* eventAction = new DRsimEventAction();
  SetUserAction(eventAction);

  SetUserAction(new DRsimSteppingAction(eventAction));
}

void DRsimActionInitialization::DefineCommands() {
  fMessenger = new G4GenericMessenger(this, "/DRsim/action/", "action initialization control");
  G4GenericMessenger::Command& ioCmd = fMessenger->DeclareProperty("useHepMC",fUseHepMC,"use HepMC");
  ioCmd.SetParameterName("useHepMC",true);
  ioCmd.SetDefaultValue("False");

  G4GenericMessenger::Command& calibCmd = fMessenger->DeclareProperty("useCalib",fUseCalib,"use Calib");
  calibCmd.SetParameterName("useCalib",true);
  calibCmd.SetDefaultValue("False");

  G4GenericMessenger::Command& gpsCmd = fMessenger->DeclareProperty("useGPS",fUseGPS,"use GPS");
  gpsCmd.SetParameterName("useGPS",true);
  gpsCmd.SetDefaultValue("False");

  // useCORSIKA sets both the instance var AND the static (read by PGA lazily).
  G4GenericMessenger::Command& corsikaModeCmd = fMessenger->DeclareMethod(
      "useCORSIKA", &DRsimActionInitialization::SetUseCORSIKACmd, "use CORSIKA input file");
  corsikaModeCmd.SetParameterName("useCORSIKA",true);
  corsikaModeCmd.SetDefaultValue("False");

  // Separate messenger for CORSIKA parameters — all set static vars too.
  fCORSIKAMessenger = new G4GenericMessenger(this, "/DRsim/corsika/", "CORSIKA input control");

  G4GenericMessenger::Command& corsikaFileCmd = fCORSIKAMessenger->DeclareMethod(
      "inputFile", &DRsimActionInitialization::SetCORSIKAFileCmd,
      "CORSIKA DAT file path");
  corsikaFileCmd.SetParameterName("inputFile",true);
  corsikaFileCmd.SetDefaultValue("");

  G4GenericMessenger::Command& corsikaSkipCmd = fCORSIKAMessenger->DeclareMethod(
      "skipEvents", &DRsimActionInitialization::SetCORSIKASkipCmd,
      "Skip N CORSIKA events before first read");
  corsikaSkipCmd.SetParameterName("skipEvents",true);
  corsikaSkipCmd.SetDefaultValue("0");

  G4GenericMessenger::Command& corsikaCenterCmd = fCORSIKAMessenger->DeclareMethod(
      "centerCore", &DRsimActionInitialization::SetCenterCoreCmd,
      "Legacy: True sets centerMode=core, False sets centerMode=none");
  corsikaCenterCmd.SetParameterName("centerCore",true);
  corsikaCenterCmd.SetDefaultValue("False");

  G4GenericMessenger::Command& centerModeCmd = fCORSIKAMessenger->DeclareMethod(
      "centerMode", &DRsimActionInitialization::SetCenterModeCmd,
      "Shower centering: none | core (EVTH x/y) | energy (highest-p particle) | random (uniform scatter)");
  centerModeCmd.SetParameterName("centerMode",true);
  centerModeCmd.SetDefaultValue("none");
}

void DRsimActionInitialization::SetUseCORSIKACmd(G4bool v) {
  fUseCORSIKA    = v;
  sCORSIKAEnabled = v;
}

void DRsimActionInitialization::SetCORSIKAFileCmd(G4String v) {
  fCORSIKAFile = v;
  sCORSIKAFile = v;
  // Also treat setting a non-empty file as implicit useCORSIKA=true
  if (!v.empty()) { fUseCORSIKA = true; sCORSIKAEnabled = true; }
}

void DRsimActionInitialization::SetCORSIKASkipCmd(G4int v) {
  fCORSIKASkip = v;
  sCORSIKASkip = v;
}

void DRsimActionInitialization::SetCenterCoreCmd(G4bool v) {
  fCORSIKACenterCore = v;
  sCenterCore = v;
  // Legacy: map bool to the new mode string
  fCORSIKACenterMode = v ? "core" : "none";
  sCenterMode        = v ? "core" : "none";
}

void DRsimActionInitialization::SetCenterModeCmd(G4String v) {
  if (v != "none" && v != "core" && v != "energy" && v != "random") {
    G4cerr << "[CORSIKA] Unknown centerMode \"" << v
           << "\"; choose: none | core | energy | random" << G4endl;
    return;
  }
  fCORSIKACenterMode = v;
  sCenterMode        = v;
  // Keep legacy bool in sync
  sCenterCore        = (v == "core");
}
