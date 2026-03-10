#include "DRsimDetectorConstruction.hh"
#include "DRsimSiPMSD.hh"

#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"

#include "G4SDManager.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4GeometryManager.hh"

#include "G4Colour.hh"
#include "G4SystemOfUnits.hh"

#include "Randomize.hh"

using namespace std;

G4ThreadLocal DRsimMagneticField* DRsimDetectorConstruction::fMagneticField = 0;
G4ThreadLocal G4FieldManager*     DRsimDetectorConstruction::fFieldMgr      = 0;

int    DRsimDetectorConstruction::fNofRow      = 1;
int    DRsimDetectorConstruction::fNofModules  = 1;   // updated in Construct()
G4double DRsimDetectorConstruction::fgWorldHalfXY = 200.;  // mm, updated in Construct()
G4double DRsimDetectorConstruction::fgTowerHalfZ  = 500.;  // mm, updated in Construct()

// ── ctor ──────────────────────────────────────────────────────────────────
DRsimDetectorConstruction::DRsimDetectorConstruction()
: G4VUserDetectorConstruction(), fMessenger(0), fMaterials(NULL),
  fModuleSpacing(0.), fNSuperRow(0), fSuperModuleSpacing(0.), fTowerDepth(1000.)
{
  DefineCommands();
  DefineMaterials();

  PMTT    = 0.3*mm;
  filterT = 0.01*mm;

  fVisAttrOrange  = new G4VisAttributes(G4Colour(1.0,0.5,0.,1.0));
  fVisAttrOrange->SetVisibility(true);
  fVisAttrBlue    = new G4VisAttributes(G4Colour(0.2,0.4,1.0,0.8));
  fVisAttrBlue->SetVisibility(true);
  fVisAttrGray    = new G4VisAttributes(G4Colour(0.3,0.3,0.3,0.3));
  fVisAttrGray->SetVisibility(true);
  fVisAttrGreen   = new G4VisAttributes(G4Colour(0.2,0.9,0.3,0.8));
  fVisAttrGreen->SetVisibility(true);
  fVisAttrCyan    = new G4VisAttributes(G4Colour(0.0,1.0,1.0));
  fVisAttrCyan->SetVisibility(true);
  fVisAttrYellow  = new G4VisAttributes(G4Colour(1.0,1.0,0.0));
  fVisAttrYellow->SetVisibility(true);
  fVisAttrMagenta = new G4VisAttributes(G4Colour(1.0,0.0,1.0));
  fVisAttrMagenta->SetVisibility(true);
}

DRsimDetectorConstruction::~DRsimDetectorConstruction() {
  delete fMessenger;
  delete fMaterials;
  delete fVisAttrOrange;  delete fVisAttrBlue;  delete fVisAttrGray;
  delete fVisAttrGreen;   delete fVisAttrCyan;  delete fVisAttrYellow;
  delete fVisAttrMagenta;
}

void DRsimDetectorConstruction::DefineMaterials() {
  fMaterials = DRsimMaterials::GetInstance();
}

// ── Construct ─────────────────────────────────────────────────────────────
G4VPhysicalVolume* DRsimDetectorConstruction::Construct() {
  G4GeometryManager::GetInstance()->OpenGeometry();
  G4PhysicalVolumeStore::GetInstance()->Clean();
  G4LogicalVolumeStore::GetInstance()->Clean();
  G4SolidStore::GetInstance()->Clean();

  checkOverlaps = false;

  // Supermodule mode: nSuperRow > 0 → total rows = nSuperRow × 27
  if (fNSuperRow > 0)
    fNofRow = fNSuperRow * 27;

  // Update static module count from current fNofRow
  fNofModules = fNofRow * fNofRow;

  // Cosmic_module: 37.5 × 37.5 mm outer size
  const G4double moduleUnitDimension = 37.5;   // mm
  const G4double modulePitch = (moduleUnitDimension + fModuleSpacing) * mm;
  G4double halfExtent;
  if (fNSuperRow > 0) {
    // Two-level grid: account for supermodule spacing
    G4double smSize  = 27 * modulePitch;
    G4double smPitch = smSize + fSuperModuleSpacing * mm;
    halfExtent = (fNSuperRow - 1) / 2.0 * smPitch + smSize / 2. + 100.*mm;
  } else {
    halfExtent = (fNofRow > 1)
        ? ((fNofRow - 1) / 2.0 * modulePitch + moduleUnitDimension / 2. * mm + 100.*mm)
        : 200.*mm;
  }

  fgWorldHalfXY = halfExtent / mm;   // store in mm for PrimaryGeneratorAction
  fgTowerHalfZ  = fTowerDepth / 2.;  // store in mm

  G4VSolid* worldSolid = new G4Box("worldBox",
                                    halfExtent, halfExtent, 10.*m);
  worldLogical = new G4LogicalVolume(worldSolid,
                                      FindMaterial("G4_Galactic"), "worldLogical");
  G4VPhysicalVolume* worldPhysical =
      new G4PVPlacement(0, G4ThreeVector(), worldLogical,
                        "worldPhysical", 0, false, 0, checkOverlaps);

  fFrontL  = 0.;
  fModuleH = moduleUnitDimension;
  fModuleW = moduleUnitDimension;
  doPMT    = true;

  // fTowerXY = {1, 1}: one SiPM per module; Water/LAB alternates by checkerboard
  fTowerXY = std::make_pair(1, 1);

  ModuleBuild(ModuleLogical, PMTGLogical, PMTcellLogical, PMTcathLogical, fModuleProp);

  return worldPhysical;
}

// ── ConstructSDandField ───────────────────────────────────────────────────
void DRsimDetectorConstruction::ConstructSDandField() {
  G4SDManager* SDman = G4SDManager::GetSDMpointer();

  if (doPMT) {
    for (int i = 0; i < fNofModules; i++) {
      DRsimSiPMSD* SiPMSDmodule =
          new DRsimSiPMSD("Module_" + std::to_string(i),
                           "ModuleHC_" + std::to_string(i),
                           fModuleProp.at(i));
      SDman->AddNewDetector(SiPMSDmodule);
      // SD on PMTcellLogical: photon entering glass cell is detected.
      PMTcellLogical[i]->SetSensitiveDetector(SiPMSDmodule);
    }
  }
}

// ── ModuleBuild ───────────────────────────────────────────────────────────
// Cosmic_module geometry (37.5 × 37.5 × fTowerDepth mm) — from Cosmic_module.pdf:
//   Fe absorber:    6.00 mm walls (outermost)
//   Al liner:       0.25 mm walls inside Fe
//   Active volume:  25 × 25 mm (Water or LS), centered
//   Checkerboard:   (i_row+j_col)%2==0 → Water (Cherenkov), else → LS (Scintillation)
//   1 SiPM (25×25 mm glass + Si cathode) per module at back face
//
// Supermodule layout (when fNSuperRow > 0):
//   One supermodule = 27 × 27 modules
//   Array = fNSuperRow × fNSuperRow supermodules
//   Total modules = (fNSuperRow×27)²
//
void DRsimDetectorConstruction::ModuleBuild(
    G4LogicalVolume* ModuleLogical_[],
    G4LogicalVolume* PMTGLogical_[],
    G4LogicalVolume* PMTcellLogical_[],
    G4LogicalVolume* PMTcathLogical_[],
    std::vector<DRsimInterface::DRsimModuleProperty>& ModuleProp_)
{
  // Geometry constants (Cosmic_module.pdf)
  // 37.5 mm total = 25 mm active + 2×(0.25 mm Al + 6 mm Fe) per side
  const G4double modHalf   = (fTowerDepth / 2.) * mm;
  const G4double alThick   = 0.25 * mm;
  const G4double feThick   = 6.0  * mm;
  const G4double feHalfXY  = (fModuleW / 2.) * mm;             // 18.75 mm — Fe outermost
  const G4double alHalfXY  = feHalfXY - feThick;               // 12.75 mm — Al liner inside Fe
  const G4double actHalf   = alHalfXY - alThick;               // 12.50 mm → 25×25 mm active
  const G4double sipmStackH = PMTT + filterT;
  const G4double slabHalf  = modHalf - sipmStackH / 2.;
  const G4double slabZ     = -sipmStackH / 2.;
  const G4double cellZ     = modHalf - sipmStackH / 2.;
  const G4double cathDz    = (PMTT - filterT) / 2.;

  const G4double modulePitch = (fModuleW + fModuleSpacing) * mm;
  // Supermodule layout constants (only used when fNSuperRow > 0)
  const G4int    modsPerSM = 27;
  const G4double smSize    = modsPerSM * modulePitch;
  const G4double smPitch   = smSize + fSuperModuleSpacing * mm;

  for (int i = 0; i < fNofModules; i++) {
    G4String mName = setModuleName(i);

    // Global grid position
    int i_row = i / fNofRow;
    int j_col = i % fNofRow;

    // Module centre coordinates
    G4double cx, cy;
    if (fNSuperRow > 0) {
      // Two-level: supermodule position + local position within supermodule
      int sm_row  = i_row / modsPerSM;
      int sm_col  = j_col / modsPerSM;
      int loc_row = i_row % modsPerSM;
      int loc_col = j_col % modsPerSM;
      G4double sm_cx = (sm_row - (fNSuperRow - 1) / 2.0) * smPitch;
      G4double sm_cy = (sm_col - (fNSuperRow - 1) / 2.0) * smPitch;
      cx = sm_cx + (loc_row - (modsPerSM - 1) / 2.0) * modulePitch;
      cy = sm_cy + (loc_col - (modsPerSM - 1) / 2.0) * modulePitch;
    } else {
      cx = (i_row - (fNofRow - 1) / 2.0) * modulePitch;
      cy = (j_col - (fNofRow - 1) / 2.0) * modulePitch;
    }

    // Checkerboard: even → Water (Cherenkov), odd → LS (Scintillation)
    bool isWater = ((i_row + j_col) % 2 == 0);
    G4Material* activeMat = isWater ? FindMaterial("Water") : FindMaterial("LS");
    G4String    activeTag = isWater ? "_Water" : "_LS";

    // ── Fe absorber (outermost, 6 mm walls) ──────────────────────────────
    auto* feSolid = new G4Box("FeModule", feHalfXY, feHalfXY, modHalf);
    ModuleLogical_[i] = new G4LogicalVolume(feSolid, FindMaterial("Iron"), mName);
    new G4PVPlacement(0, G4ThreeVector(cx, cy, 0.), ModuleLogical_[i], mName,
                      worldLogical, false, 0, checkOverlaps);
    ModuleLogical_[i]->SetVisAttributes(fVisAttrOrange);

    // ── Al liner (0.25 mm walls inside Fe) ───────────────────────────────
    auto* alSolid = new G4Box("AlModule", alHalfXY, alHalfXY, modHalf);
    auto* alLog   = new G4LogicalVolume(alSolid, FindMaterial("Aluminum"), mName + "_Al");
    new G4PVPlacement(0, G4ThreeVector(), alLog, mName + "_AlPV",
                      ModuleLogical_[i], false, 0, checkOverlaps);
    alLog->SetVisAttributes(fVisAttrGray);

    // ── Active volume (25×25 mm, Water or LS) inside Al ──────────────────
    auto* actSolid = new G4Box("ActiveVol", actHalf, actHalf, slabHalf);
    auto* actLog   = new G4LogicalVolume(actSolid, activeMat, mName + activeTag);
    new G4PVPlacement(0, G4ThreeVector(0., 0., slabZ),
                      actLog, mName + activeTag + "PV", alLog, false, 0, checkOverlaps);
    actLog->SetVisAttributes(isWater ? fVisAttrBlue : fVisAttrGreen);

    // ── SiPM: 25×25 mm glass cell + Si photocathode inside Al ────────────
    auto* cellSolid = new G4Box("PMTcellSolid", actHalf, actHalf, sipmStackH / 2.);
    PMTcellLogical_[i] = new G4LogicalVolume(cellSolid, FindMaterial("Glass"),
                                              "PMTcellLogical_");
    new G4PVPlacement(0, G4ThreeVector(0., 0., cellZ),
                      PMTcellLogical_[i], "PMTcellPhysical",
                      alLog, false, 0, checkOverlaps);

    auto* cathSolid = new G4Box("PMTcathSolid", actHalf, actHalf, filterT / 2.);
    PMTcathLogical_[i] = new G4LogicalVolume(cathSolid, FindMaterial("Silicon"),
                                              "PMTcathLogical_");
    new G4PVPlacement(0, G4ThreeVector(0., 0., cathDz),
                      PMTcathLogical_[i], "PMTcathPhysical",
                      PMTcellLogical_[i], false, 0, checkOverlaps);
    new G4LogicalSkinSurface("Photocath_surf", PMTcathLogical_[i], FindSurface("SiPMsurf"));
    PMTcathLogical_[i]->SetVisAttributes(fVisAttrGreen);

    // ── Module property record ────────────────────────────────────────────
    DRsimInterface::DRsimModuleProperty prop;
    prop.towerXY   = fTowerXY;   // {1, 1}
    prop.ModuleNum = i;
    ModuleProp_.push_back(prop);

    PMTGLogical_[i] = nullptr;
  }
}

// ── DefineCommands ────────────────────────────────────────────────────────
void DRsimDetectorConstruction::DefineCommands() {
  fMessenger = new G4GenericMessenger(this, "/detector/",
                                      "Detector geometry control");

  G4GenericMessenger::Command& rowCmd =
      fMessenger->DeclareProperty("nRow", fNofRow,
                                  "number of module rows (array = nRow x nRow)");
  rowCmd.SetParameterName("nRow", true);
  rowCmd.SetDefaultValue("1");
  rowCmd.SetRange("nRow >= 1 && nRow <= 200");

  G4GenericMessenger::Command& spacingCmd =
      fMessenger->DeclarePropertyWithUnit("moduleSpacing", "mm", fModuleSpacing,
                                          "extra gap between modules [mm]");
  spacingCmd.SetParameterName("moduleSpacing", true);
  spacingCmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& smRowCmd =
      fMessenger->DeclareProperty("nSuperRow", fNSuperRow,
                                  "supermodule rows (0=disabled; total rows = nSuperRow×27)");
  smRowCmd.SetParameterName("nSuperRow", true);
  smRowCmd.SetDefaultValue("0");
  smRowCmd.SetRange("nSuperRow >= 0 && nSuperRow <= 10");

  G4GenericMessenger::Command& smSpacingCmd =
      fMessenger->DeclarePropertyWithUnit("superModuleSpacing", "mm", fSuperModuleSpacing,
                                          "extra gap between supermodules [mm]");
  smSpacingCmd.SetParameterName("superModuleSpacing", true);
  smSpacingCmd.SetDefaultValue("0.");

  G4GenericMessenger::Command& depthCmd =
      fMessenger->DeclarePropertyWithUnit("towerDepth", "mm", fTowerDepth,
                                          "module depth [mm]");
  depthCmd.SetParameterName("towerDepth", true);
  depthCmd.SetDefaultValue("1000.");
}
