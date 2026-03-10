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
G4String DRsimActionInitialization::sCenterMode     = "core";

DRsimActionInitialization::DRsimActionInitialization(G4int seed, G4String filename)
: G4VUserActionInitialization(),
  fMessenger(nullptr), fCORSIKAMessenger(nullptr),
  fUseHepMC(false), fUseCalib(false), fUseGPS(false),
  fUseCORSIKA(false), fCORSIKAFile(""), fCORSIKASkip(0), fCORSIKACenterMode("core")
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
  SetUserAction(new DRsimPrimaryGeneratorAction(fSeed,fUseHepMC,fUseCalib,fUseGPS));
  SetUserAction(new DRsimRunAction(fSeed,fFilename,fUseHepMC));

  DRsimEventAction* eventAction = new DRsimEventAction();
  SetUserAction(eventAction);

  SetUserAction(new DRsimSteppingAction(eventAction));
}

// ── Setter methods (called by messenger, also update statics) ──────────────
void DRsimActionInitialization::SetUseCORSIKA(G4bool v) {
  fUseCORSIKA    = v;
  sCORSIKAEnabled = v;
}

void DRsimActionInitialization::SetCORSIKAFile(G4String v) {
  fCORSIKAFile = v;
  sCORSIKAFile = v;
  // Setting a non-empty file implicitly enables CORSIKA mode
  if (!v.empty()) { fUseCORSIKA = true; sCORSIKAEnabled = true; }
}

void DRsimActionInitialization::SetCORSIKASkip(G4int v) {
  fCORSIKASkip = v;
  sCORSIKASkip = v;
}

void DRsimActionInitialization::SetCenterMode(G4String v) {
  fCORSIKACenterMode = v;
  sCenterMode        = v;
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

  // useCORSIKA: use DeclareMethod so that the setter also updates the static.
  G4GenericMessenger::Command& corsikaModeCmd =
      fMessenger->DeclareMethod("useCORSIKA",
                                 &DRsimActionInitialization::SetUseCORSIKA,
                                 "use CORSIKA input file");
  corsikaModeCmd.SetParameterName("useCORSIKA",true);
  corsikaModeCmd.SetDefaultValue("False");

  // Separate messenger for CORSIKA parameters.
  fCORSIKAMessenger = new G4GenericMessenger(this, "/DRsim/corsika/", "CORSIKA input control");

  G4GenericMessenger::Command& corsikaFileCmd =
      fCORSIKAMessenger->DeclareMethod("inputFile",
                                        &DRsimActionInitialization::SetCORSIKAFile,
                                        "CORSIKA DAT file path");
  corsikaFileCmd.SetParameterName("inputFile",true);
  corsikaFileCmd.SetDefaultValue("");

  G4GenericMessenger::Command& corsikaSkipCmd =
      fCORSIKAMessenger->DeclareMethod("skipEvents",
                                        &DRsimActionInitialization::SetCORSIKASkip,
                                        "Skip N CORSIKA events before first read");
  corsikaSkipCmd.SetParameterName("skipEvents",true);
  corsikaSkipCmd.SetDefaultValue("0");

  G4GenericMessenger::Command& centerModeCmd =
      fCORSIKAMessenger->DeclareMethod("centerMode",
                                        &DRsimActionInitialization::SetCenterMode,
                                        "Shower centering: none | core | energy | random");
  centerModeCmd.SetParameterName("centerMode",true);
  centerModeCmd.SetDefaultValue("core");
}
