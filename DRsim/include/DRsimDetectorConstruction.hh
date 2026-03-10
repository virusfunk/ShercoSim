#ifndef DRsimDetectorConstruction_h
#define DRsimDetectorConstruction_h 1

#include "DRsimMagneticField.hh"
#include "DRsimMaterials.hh"
#include "DRsimSiPMHit.hh"

#include "G4VUserDetectorConstruction.hh"
#include "G4Box.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4VSensitiveDetector.hh"
#include "G4VisAttributes.hh"
#include "G4GenericMessenger.hh"
#include "G4FieldManager.hh"
#include "G4ThreeVector.hh"

#include <math.h>

using namespace std;

class DRsimMagneticField;

class DRsimDetectorConstruction : public G4VUserDetectorConstruction {
public:
  DRsimDetectorConstruction();
  virtual ~DRsimDetectorConstruction();

  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();

  static int fNofModules;
  static int fNofRow;

  // World XY half-extent (mm) — set in Construct(), read by PrimaryGeneratorAction
  static G4double fgWorldHalfXY;
  static G4double GetWorldHalfXY() { return fgWorldHalfXY; }

  // Tower half-depth in z (mm) — front face at z = -fgTowerHalfZ
  static G4double fgTowerHalfZ;
  static G4double GetTowerHalfZ() { return fgTowerHalfZ; }

private:
  void DefineCommands();
  void DefineMaterials();
  G4Material* FindMaterial(G4String matName) { return fMaterials->GetMaterial(matName); }
  G4OpticalSurface* FindSurface(G4String surfName) { return fMaterials->GetOpticalSurface(surfName); }

  void ModuleBuild(G4LogicalVolume* ModuleLogical_[],
                   G4LogicalVolume* PMTGLogical_[],
                   G4LogicalVolume* PMTcellLogical_[],
                   G4LogicalVolume* PMTcathLogical_[],
                   std::vector<DRsimInterface::DRsimModuleProperty>& towerProps_);

  void FiberModuleBuild(G4LogicalVolume* ModuleLogical_[],
                        G4LogicalVolume* PMTGLogical_[],
                        G4LogicalVolume* PMTcellLogical_[],
                        G4LogicalVolume* PMTcathLogical_[],
                        std::vector<DRsimInterface::DRsimModuleProperty>& towerProps_);

  G4bool checkOverlaps;
  G4GenericMessenger* fMessenger;
  DRsimMaterials* fMaterials;

  static G4ThreadLocal DRsimMagneticField* fMagneticField;
  static G4ThreadLocal G4FieldManager* fFieldMgr;

  G4VisAttributes* fVisAttrOrange;
  G4VisAttributes* fVisAttrBlue;
  G4VisAttributes* fVisAttrGray;
  G4VisAttributes* fVisAttrGreen;
  G4VisAttributes* fVisAttrCyan;
  G4VisAttributes* fVisAttrYellow;
  G4VisAttributes* fVisAttrMagenta;

  G4double fFrontL;
  G4double fTowerDepth;
  G4String fGeometry;  // "slab" or "fiber"
  G4double fModuleH;
  G4double fModuleW;
  G4double fModuleSpacing;       // extra gap between modules within a supermodule [mm]
  G4int    fNSuperRow;           // supermodule rows (0 = disabled; total rows = nSuperRow×27)
  G4double fSuperModuleSpacing;  // extra gap between supermodules [mm]
  G4int fRandomSeed;

  G4double PMTT;
  G4double filterT;

  G4bool doPMT;
  G4bool fAllLS;           // if true, all modules use LAB (scintillator-only)
  G4bool fAllWater;        // if true, all modules use Water (Cherenkov-only)
  G4bool fFlipCheckerboard; // if true, invert C/S pattern: center module becomes S-type

  static const int kMaxModules = 100000; // supports up to 9×9 SM (59049) and beyond

  G4LogicalVolume* ModuleLogical[kMaxModules];
  G4LogicalVolume* PMTGLogical[kMaxModules];
  G4LogicalVolume* PMTcathLogical[kMaxModules];
  G4LogicalVolume* PMTcellLogical[kMaxModules];

  DRsimInterface::hitXY fTowerXY;
  std::vector<DRsimInterface::DRsimModuleProperty> fModuleProp;

  G4LogicalVolume* worldLogical;

  G4String setModuleName(int i) {
    return "Module" + std::to_string(i);
  }
};

#endif
