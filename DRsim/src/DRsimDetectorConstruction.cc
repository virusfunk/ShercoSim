#include "DRsimDetectorConstruction.hh"
#include "DRsimCellParameterisation.hh"
#include "DRsimFilterParameterisation.hh"
#include "DRsimMirrorParameterisation.hh"
#include "DRsimSiPMSD.hh"

#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4PVParameterised.hh"

#include "G4IntersectionSolid.hh"
#include "G4SDManager.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalBorderSurface.hh"
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

// ── ctor ──────────────────────────────────────────────────────────────────
DRsimDetectorConstruction::DRsimDetectorConstruction()
: G4VUserDetectorConstruction(), fMessenger(0), fMaterials(NULL),
  fModuleSpacing(0.)
{
  DefineCommands();
  DefineMaterials();

  clad_C_rMin = 0.49*mm;  clad_C_rMax = 0.50*mm;
  clad_C_Dz   = 2.5*m;    clad_C_Sphi = 0.;  clad_C_Dphi = 2.*M_PI;

  core_C_rMin = 0.*mm;    core_C_rMax = 0.49*mm;
  core_C_Dz   = 2.5*m;    core_C_Sphi = 0.;  core_C_Dphi = 2.*M_PI;

  clad_S_rMin = 0.485*mm; clad_S_rMax = 0.50*mm;
  clad_S_Dz   = 2.5*m;    clad_S_Sphi = 0.;  clad_S_Dphi = 2.*M_PI;

  core_S_rMin = 0.*mm;    core_S_rMax = 0.485*mm;
  core_S_Dz   = 2.5*m;    core_S_Sphi = 0.;  core_S_Dphi = 2.*M_PI;

  PMTT      = 0.3*mm;
  filterT   = 0.01*mm;
  reflectorT = 0.03*mm;

  fVisAttrOrange  = new G4VisAttributes(G4Colour(1.0,0.5,0.,1.0));
  fVisAttrOrange->SetVisibility(true);
  fVisAttrBlue    = new G4VisAttributes(G4Colour(0.,0.,1.0,1.0));
  fVisAttrBlue->SetVisibility(true);
  fVisAttrGray    = new G4VisAttributes(G4Colour(0.3,0.3,0.3,0.3));
  fVisAttrGray->SetVisibility(true);
  fVisAttrGreen   = new G4VisAttributes(G4Colour(0.3,0.7,0.3));
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

  // Update static module count from current fNofRow
  fNofModules = fNofRow * fNofRow;

  // World volume: large enough for the full array.
  // Pitch = moduleW + spacing; array half-extent = (N-1)/2 * pitch + moduleW/2
  float moduleUnitDimension = 122.;   // mm
  G4double pitch = (moduleUnitDimension + fModuleSpacing) * mm;
  G4double halfExtent = (fNofRow > 1)
      ? ((fNofRow - 1) / 2.0 * pitch + moduleUnitDimension / 2. * mm + 100.*mm)
      : 200.*mm;

  fgWorldHalfXY = halfExtent / mm;   // store in mm for PrimaryGeneratorAction

  G4VSolid* worldSolid = new G4Box("worldBox",
                                    halfExtent, halfExtent, 10.*m);
  worldLogical = new G4LogicalVolume(worldSolid,
                                      FindMaterial("G4_Galactic"), "worldLogical");
  G4VPhysicalVolume* worldPhysical =
      new G4PVPlacement(0, G4ThreeVector(), worldLogical,
                        "worldPhysical", 0, false, 0, checkOverlaps);

  fFrontL     = 0.;
  fTowerDepth = 100.;
  fModuleH    = moduleUnitDimension;
  fModuleW    = moduleUnitDimension;
  fFiberUnitH = 1.;

  doFiber     = true;
  doReflector = false;
  doPMT       = true;

  fiberUnit  = new G4Box("fiber_SQ",
                          (fFiberUnitH/2)*mm, (1./2)*mm, (fTowerDepth/2)*mm);
  fiberClad  = new G4Tubs("fiber",  0, clad_C_rMax, fTowerDepth/2., 0*deg, 360.*deg);
  fiberCoreC = new G4Tubs("fiberC", 0, core_C_rMax, fTowerDepth/2., 0*deg, 360.*deg);
  fiberCoreS = new G4Tubs("fiberS", 0, core_S_rMax, fTowerDepth/2., 0*deg, 360.*deg);

  dimCalc = new dimensionCalc();
  dimCalc->SetFrontL(fFrontL);
  dimCalc->SetTower_height(fTowerDepth);
  dimCalc->SetPMTT(PMTT + filterT);
  dimCalc->SetReflectorT(reflectorT);
  dimCalc->SetNofModules(fNofModules);
  dimCalc->SetNofRow(fNofRow);
  dimCalc->SetModuleHeight(fModuleH);
  dimCalc->SetModuleWidth(fModuleW);

  ModuleBuild(ModuleLogical, PMTGLogical, PMTfilterLogical, PMTcellLogical,
              PMTcathLogical, ReflectorMirrorLogical,
              fiberUnitIntersection, fiberCladIntersection,
              fiberCoreIntersection, fModuleProp);

  delete dimCalc;
  return worldPhysical;
}

// ── ConstructSDandField ───────────────────────────────────────────────────
void DRsimDetectorConstruction::ConstructSDandField() {
  G4SDManager* SDman = G4SDManager::GetSDMpointer();

  if (doPMT) {
    for (int i = 0; i < fNofModules; i++) {
      DRsimSiPMSD* SiPMSDmodule =
          new DRsimSiPMSD("Module" + std::to_string(i),
                           "ModuleC" + std::to_string(i),
                           fModuleProp.at(i));
      SDman->AddNewDetector(SiPMSDmodule);
      // SD must be on PMTcellLogical (the glass cell BEFORE the photocathode).
      // G4OpBoundaryProcess detects photons at the PMTcell→PMTcath surface boundary;
      // the step's PreStepPoint is in PMTcell, so that is the volume whose SD is invoked.
      PMTcellLogical[i]->SetSensitiveDetector(SiPMSDmodule);
    }
  }
}

// ── ModuleBuild ───────────────────────────────────────────────────────────
// Place fNofModules fiber-calorimeter towers in a rectangular grid.
// Centre-to-centre pitch = fModuleW + fModuleSpacing [mm].
void DRsimDetectorConstruction::ModuleBuild(
    G4LogicalVolume* ModuleLogical_[],
    G4LogicalVolume* PMTGLogical_[],
    G4LogicalVolume* PMTfilterLogical_[],
    G4LogicalVolume* PMTcellLogical_[],
    G4LogicalVolume* PMTcathLogical_[],
    G4LogicalVolume* ReflectorMirrorLogical_[],
    std::vector<G4LogicalVolume*> fiberUnitIntersection_[],
    std::vector<G4LogicalVolume*> fiberCladIntersection_[],
    std::vector<G4LogicalVolume*> fiberCoreIntersection_[],
    std::vector<DRsimInterface::DRsimModuleProperty>& ModuleProp_)
{
  G4double pitch = (fModuleW + fModuleSpacing) * mm;

  for (int i = 0; i < fNofModules; i++) {
    moduleName = setModuleName(i);

    // Rectangular grid: row = i/fNofRow, col = i%fNofRow
    G4double cx = (i / fNofRow - (fNofRow - 1) / 2.0) * pitch;
    G4double cy = (i % fNofRow - (fNofRow - 1) / 2.0) * pitch;
    G4ThreeVector modOrigin(cx, cy, 0.);
    G4ThreeVector pmtOrigin(cx, cy, (fTowerDepth / 2. + (PMTT + filterT) / 2.) * mm);

    // Module absorber volume (copper)
    module = new G4Box("Module",
                        (fModuleH/2.)*mm, (fModuleW/2.)*mm, (fTowerDepth/2.)*mm);
    ModuleLogical_[i] = new G4LogicalVolume(module, FindMaterial("Copper"), moduleName);
    new G4PVPlacement(0, modOrigin, ModuleLogical_[i], moduleName,
                      worldLogical, false, 0, checkOverlaps);

    // SiPM read-out layer (placed in world just behind the module)
    if (doPMT) {
      pmtg = new G4Box("PMTG",
                        (fModuleH/2.)*mm, (fModuleW/2.)*mm, (PMTT + filterT)/2.*mm);
      PMTGLogical_[i] = new G4LogicalVolume(pmtg, FindMaterial("G4_AIR"), moduleName);
      new G4PVPlacement(0, pmtOrigin, PMTGLogical_[i], moduleName,
                        worldLogical, false, 0, checkOverlaps);
    }

    // Fibers
    FiberImplement(i, ModuleLogical_,
                   fiberUnitIntersection_, fiberCladIntersection_,
                   fiberCoreIntersection_);

    // Module property record (used by SiPMSD)
    DRsimInterface::DRsimModuleProperty prop;
    prop.towerXY   = fTowerXY;
    prop.ModuleNum = i;
    ModuleProp_.push_back(prop);

    if (doPMT) {
      // SiPM active layer (inside PMTG)
      G4VSolid* SiPMlayerSolid = new G4Box("SiPMlayerSolid",
          (fModuleH/2.)*mm, (fModuleW/2.)*mm, (PMTT/2.)*mm);
      G4LogicalVolume* SiPMlayerLogical = new G4LogicalVolume(
          SiPMlayerSolid, FindMaterial("G4_AIR"), "SiPMlayerLogical");
      new G4PVPlacement(0, G4ThreeVector(0., 0., filterT/2.),
                        SiPMlayerLogical, "SiPMlayerPhysical",
                        PMTGLogical_[i], false, 0, checkOverlaps);

      // Optical filter layer (inside PMTG)
      G4VSolid* filterlayerSolid = new G4Box("filterlayerSolid",
          (fModuleH/2.)*mm, (fModuleW/2.)*mm, (filterT/2.)*mm);
      G4LogicalVolume* filterlayerLogical = new G4LogicalVolume(
          filterlayerSolid, FindMaterial("Glass"), "filterlayerLogical");
      new G4PVPlacement(0, G4ThreeVector(0., 0., -PMTT/2.),
                        filterlayerLogical, "filterlayerPhysical",
                        PMTGLogical_[i], false, 0, checkOverlaps);

      // Individual SiPM cells (parameterised)
      G4VSolid* PMTcellSolid = new G4Box("PMTcellSolid",
          1.2/2.*mm, 1.2/2.*mm, PMTT/2.*mm);
      PMTcellLogical_[i] = new G4LogicalVolume(
          PMTcellSolid, FindMaterial("Glass"), "PMTcellLogical_");

      DRsimCellParameterisation* PMTcellParam =
          new DRsimCellParameterisation(fFiberX, fFiberY, fFiberWhich);
      G4PVParameterised* PMTcellPhysical =
          new G4PVParameterised("PMTcellPhysical", PMTcellLogical_[i],
                                SiPMlayerLogical, kXAxis,
                                fTowerXY.first * fTowerXY.second, PMTcellParam);

      // Photocathode (silicon, sensitive)
      G4VSolid* PMTcathSolid = new G4Box("PMTcathSolid",
          1.2/2.*mm, 1.2/2.*mm, filterT/2.*mm);
      PMTcathLogical_[i] = new G4LogicalVolume(
          PMTcathSolid, FindMaterial("Silicon"), "PMTcathLogical_");
      new G4PVPlacement(0, G4ThreeVector(0., 0., (PMTT - filterT)/2.*mm),
                        PMTcathLogical_[i], "PMTcathPhysical",
                        PMTcellLogical_[i], false, 0, checkOverlaps);
      new G4LogicalSkinSurface("Photocath_surf",
                                PMTcathLogical_[i], FindSurface("SiPMSurf"));

      // Optical filter cells (parameterised)
      G4VSolid* filterSolid = new G4Box("filterSolid",
          1.2/2.*mm, 1.2/2.*mm, filterT/2.*mm);
      PMTfilterLogical_[i] = new G4LogicalVolume(
          filterSolid, FindMaterial("Gelatin"), "PMTfilterLogical_");

      int filterNo = (int)(fTowerXY.first * fTowerXY.second) / 2;
      if (fTowerXY.first % 2 == 1) filterNo++;

      DRsimFilterParameterisation* filterParam =
          new DRsimFilterParameterisation(fFiberX, fFiberY, fFiberWhich);
      G4PVParameterised* filterPhysical =
          new G4PVParameterised("filterPhysical", PMTfilterLogical_[i],
                                filterlayerLogical, kXAxis, filterNo, filterParam);
      new G4LogicalBorderSurface("filterSurf",
                                  filterPhysical, PMTcellPhysical,
                                  FindSurface("FilterSurf"));

      PMTcathLogical_[i]->SetVisAttributes(fVisAttrGreen);
      PMTfilterLogical_[i]->SetVisAttributes(fVisAttrOrange);
    }
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
  rowCmd.SetRange("nRow >= 1 && nRow <= 10");

  G4GenericMessenger::Command& spacingCmd =
      fMessenger->DeclarePropertyWithUnit("moduleSpacing", "mm", fModuleSpacing,
                                          "centre-to-centre extra gap between modules [mm]");
  spacingCmd.SetParameterName("moduleSpacing", true);
  spacingCmd.SetDefaultValue("0.");
}

// ── FiberImplement ────────────────────────────────────────────────────────
void DRsimDetectorConstruction::FiberImplement(
    G4int i, G4LogicalVolume* ModuleLogical__[],
    std::vector<G4LogicalVolume*> fiberUnitIntersection__[],
    std::vector<G4LogicalVolume*> fiberCladIntersection__[],
    std::vector<G4LogicalVolume*> fiberCoreIntersection__[])
{
  fFiberX.clear();
  fFiberY.clear();
  fFiberWhich.clear();

  int NofFiber = (int)(fModuleW / 1.5);
  int NofPlate = (int)(fModuleH / 1.5);
  fBottomEdge  = fmod(fModuleW, 1.5) / 2.;
  fLeftEdge    = fmod(fModuleH, 1.5) / 2.;

  fTowerXY = std::make_pair(NofPlate, NofFiber);

  G4bool fWhich = false;
  for (int k = 0; k < NofPlate; k++) {
    for (int j = 0; j < NofFiber; j++) {
      G4float fX = -fModuleH*mm/2 + k*1.5*mm + 0.75*mm + fBottomEdge*mm;
      G4float fY = -fModuleW*mm/2 + j*1.5*mm + 0.75*mm + fLeftEdge*mm;
      fWhich = !fWhich;
      fFiberX.push_back(fX);
      fFiberY.push_back(fY);
      fFiberWhich.push_back(fWhich);
    }
    if (NofFiber % 2 == 0) { fWhich = !fWhich; }
  }

  if (doFiber) {
    for (unsigned int j = 0; j < fFiberX.size(); j++) {
      if (!fFiberWhich.at(j)) {   // Cherenkov fibre (PMMA core)
        tfiberCladIntersection = new G4IntersectionSolid(
            "fiberClad", fiberClad, module, 0,
            G4ThreeVector(-fFiberX.at(j), -fFiberY.at(j), 0.));
        fiberCladIntersection__[i].push_back(
            new G4LogicalVolume(tfiberCladIntersection,
                                FindMaterial("FluorinatedPolymer"), name));
        new G4PVPlacement(0, G4ThreeVector(fFiberX.at(j), fFiberY.at(j), 0),
                          fiberCladIntersection__[i].at(j), name,
                          ModuleLogical__[i], false, j, checkOverlaps);

        tfiberCoreIntersection = new G4IntersectionSolid(
            "fiberCore", fiberCoreC, module, 0,
            G4ThreeVector(-fFiberX.at(j), -fFiberY.at(j), 0.));
        fiberCoreIntersection__[i].push_back(
            new G4LogicalVolume(tfiberCoreIntersection,
                                FindMaterial("PMMA"), name));
        new G4PVPlacement(0, G4ThreeVector(0., 0., 0.),
                          fiberCoreIntersection__[i].at(j), name,
                          fiberCladIntersection__[i].at(j), false, j,
                          checkOverlaps);

        fiberCladIntersection__[i].at(j)->SetVisAttributes(fVisAttrGray);
        fiberCoreIntersection__[i].at(j)->SetVisAttributes(fVisAttrBlue);

      } else {                    // Scintillation fibre (polystyrene core)
        tfiberCladIntersection = new G4IntersectionSolid(
            "fiberClad", fiberClad, module, 0,
            G4ThreeVector(-fFiberX.at(j), -fFiberY.at(j), 0.));
        fiberCladIntersection__[i].push_back(
            new G4LogicalVolume(tfiberCladIntersection,
                                FindMaterial("PMMA"), name));
        new G4PVPlacement(0, G4ThreeVector(fFiberX.at(j), fFiberY.at(j), 0),
                          fiberCladIntersection__[i].at(j), name,
                          ModuleLogical__[i], false, j, checkOverlaps);

        tfiberCoreIntersection = new G4IntersectionSolid(
            "fiberCore", fiberCoreS, module, 0,
            G4ThreeVector(-fFiberX.at(j), -fFiberY.at(j), 0.));
        fiberCoreIntersection__[i].push_back(
            new G4LogicalVolume(tfiberCoreIntersection,
                                FindMaterial("Polystyrene"), name));
        new G4PVPlacement(0, G4ThreeVector(0., 0., 0.),
                          fiberCoreIntersection__[i].at(j), name,
                          fiberCladIntersection__[i].at(j), false, j,
                          checkOverlaps);

        fiberCladIntersection__[i].at(j)->SetVisAttributes(fVisAttrGray);
        fiberCoreIntersection__[i].at(j)->SetVisAttributes(fVisAttrOrange);
      }
    }
  }
}
