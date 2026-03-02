#include "DRsimActionInitialization.hh"
#include "DRsimPrimaryGeneratorAction.hh"
#include "DRsimRunAction.hh"
#include "DRsimEventAction.hh"
#include "DRsimSteppingAction.hh"

#include "G4GenericMessenger.hh"

using namespace std;

DRsimActionInitialization::DRsimActionInitialization(G4int seed, G4String filename)
: G4VUserActionInitialization(),
  fUseHepMC(false), fUseCalib(false), fUseGPS(false), fUseCORSIKA(false),
  fCORSIKAPath(""), fCORSIKAMessenger(nullptr)
{
  fSeed     = seed;
  fFilename = filename;

  DefineCommands();
}

DRsimActionInitialization::~DRsimActionInitialization() {
  if (fMessenger) delete fMessenger;
  if (fCORSIKAMessenger) delete fCORSIKAMessenger;
}

void DRsimActionInitialization::BuildForMaster() const {
  SetUserAction(new DRsimRunAction(fSeed, fFilename, fUseHepMC));
}

void DRsimActionInitialization::Build() const {
  SetUserAction(new DRsimPrimaryGeneratorAction(fSeed, fUseHepMC, fUseCalib,
                                                fUseGPS, fUseCORSIKA,
                                                fCORSIKAPath));
  SetUserAction(new DRsimRunAction(fSeed, fFilename, fUseHepMC));

  DRsimEventAction* eventAction = new DRsimEventAction();
  SetUserAction(eventAction);
  SetUserAction(new DRsimSteppingAction(eventAction));
}

void DRsimActionInitialization::DefineCommands() {
  fMessenger = new G4GenericMessenger(this, "/DRsim/action/",
                                      "action initialization control");

  G4GenericMessenger::Command& hepmcCmd =
      fMessenger->DeclareProperty("useHepMC", fUseHepMC, "use HepMC");
  hepmcCmd.SetParameterName("useHepMC", true);
  hepmcCmd.SetDefaultValue("False");

  G4GenericMessenger::Command& calibCmd =
      fMessenger->DeclareProperty("useCalib", fUseCalib, "use Calib");
  calibCmd.SetParameterName("useCalib", true);
  calibCmd.SetDefaultValue("False");

  G4GenericMessenger::Command& gpsCmd =
      fMessenger->DeclareProperty("useGPS", fUseGPS, "use GPS");
  gpsCmd.SetParameterName("useGPS", true);
  gpsCmd.SetDefaultValue("False");

  G4GenericMessenger::Command& corsCmd =
      fMessenger->DeclareProperty("useCORSIKA", fUseCORSIKA,
                                  "use CORSIKA binary DAT input (no CSV step)");
  corsCmd.SetParameterName("useCORSIKA", true);
  corsCmd.SetDefaultValue("False");

  // Separate /DRsim/corsika/ messenger so the path can be set before /run/initialize
  fCORSIKAMessenger = new G4GenericMessenger(this, "/DRsim/corsika/",
                                              "CORSIKA input control");
  G4GenericMessenger::Command& fileCmd =
      fCORSIKAMessenger->DeclareProperty("inputFile", fCORSIKAPath,
                                          "path to CORSIKA DAT binary file");
  fileCmd.SetParameterName("inputFile", false);
}
