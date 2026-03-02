#include <iostream>
#include "DRsimDetectorConstruction.hh"
#include "DRsimActionInitialization.hh"

#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#else
#include "G4RunManager.hh"
#endif

#include "G4UImanager.hh"
#include "G4OpticalPhysics.hh"
#include "FTFP_BERT.hh"
#include "Randomize.hh"

#ifdef G4VIS_USE
#include "G4VisExecutive.hh"
#endif

#ifdef G4UI_USE
#include "G4UIExecutive.hh"
#endif

int main(int argc, char** argv) {
#ifdef G4UI_USE
  G4UIExecutive* ui = 0;
  if (argc == 1) ui = new G4UIExecutive(argc, argv);
#endif

  G4int    seed     = 0;
  G4String filename = "output/DRsim";
  if (argc > 2) seed     = atoi(argv[2]);
  if (argc > 3) filename = argv[3];

  CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine);
  CLHEP::HepRandom::setTheSeed(seed);

#ifdef G4MULTITHREADED
  G4MTRunManager* runManager = new G4MTRunManager;
#else
  G4RunManager* runManager = new G4RunManager;
#endif

  runManager->SetUserInitialization(new DRsimDetectorConstruction());

  G4VModularPhysicsList* physicsList = new FTFP_BERT;
  G4OpticalPhysics* opticalPhysics = new G4OpticalPhysics();
  physicsList->RegisterPhysics(opticalPhysics);
  // Geant4 ≥ 11.0 removed Configure() / SetTrackSecondariesFirst();
  // Cherenkov and Scintillation are enabled by default in G4OpticalPhysics.
  runManager->SetUserInitialization(physicsList);

  runManager->SetUserInitialization(new DRsimActionInitialization(seed, filename));

#ifdef G4VIS_USE
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();
#endif

  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  if (argc != 1) {
    G4String command  = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command + fileName);
  } else {
#ifdef G4UI_USE
#ifdef G4VIS_USE
    UImanager->ApplyCommand("/control/execute init_vis.mac");
#else
    UImanager->ApplyCommand("/control/execute init.mac");
#endif
    if (ui->IsGUI()) { UImanager->ApplyCommand("/control/execute gui.mac"); }
    ui->SessionStart();
    delete ui;
#endif
  }

#ifdef G4VIS_USE
  delete visManager;
#endif
  delete runManager;
  return 0;
}
