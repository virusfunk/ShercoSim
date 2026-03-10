#ifndef DRsimSteppingAction_h
#define DRsimSteppingAction_h 1

#include "DRsimInterface.h"
#include "DRsimEventAction.hh"

#include "G4UserSteppingAction.hh"
#include "G4LogicalVolume.hh"
#include "G4Step.hh"
#include "G4TrackStatus.hh"

using namespace std;

class DRsimSteppingAction : public G4UserSteppingAction {
public:
  DRsimSteppingAction(DRsimEventAction* eventAction);
  virtual ~DRsimSteppingAction();
  virtual void UserSteppingAction(const G4Step*);

private:
  DRsimEventAction* fEventAction;
  DRsimInterface::DRsimEdepData fEdep;
  DRsimInterface::DRsimLeakageData fLeak;
  
  G4VPhysicalVolume* GetMotherTower(G4TouchableHandle touchable) { return touchable->GetVolume(touchable->GetHistoryDepth()-1); }

  G4int GetModuleNum(G4String towerName) {
    // Volume names: "Module0", "Module0_AlPV", "Module0_WaterPV", etc.
    // Extract the integer immediately following "Module"
    const std::string prefix = "Module";
    size_t pos = towerName.find(prefix);
    if (pos != std::string::npos) {
      size_t numStart = pos + prefix.size();
      std::string numStr;
      while (numStart < towerName.size() && std::isdigit((unsigned char)towerName[numStart]))
        numStr += towerName[numStart++];
      if (!numStr.empty()) return std::stoi(numStr);
    }
    return -1;
  }
};

#endif
