#include "DRsimMaterials.hh"
#include "G4SystemOfUnits.hh"

#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>


DRsimMaterials* DRsimMaterials::fInstance = 0;

DRsimMaterials::DRsimMaterials() {
  fNistMan = G4NistManager::Instance();
  CreateMaterials();
}

DRsimMaterials::~DRsimMaterials() {}

DRsimMaterials* DRsimMaterials::GetInstance() {
  if (fInstance==0) fInstance = new DRsimMaterials();

  return fInstance;
}

G4Material* DRsimMaterials::GetMaterial(const G4String matName) {
  G4Material* mat = fNistMan->FindOrBuildMaterial(matName);

  if (!mat) mat = G4Material::GetMaterial(matName);
  if (!mat) {
    std::ostringstream o;
    o << "Material " << matName << " not found!";
    G4Exception("DRsimMaterials::GetMaterial","",FatalException,o.str().c_str());
  }

  return mat;
}

G4OpticalSurface* DRsimMaterials::GetOpticalSurface(const G4String surfName) {
  if (surfName == "SiPMsurf") return fSiPMSurf;
  if (surfName == "PMTsurf") return fPMTSurf;
  else if (surfName == "FilterSurf") return fFilterSurf;
  else if (surfName == "MirrorSurf") return fMirrorSurf;
  else if (surfName == "AlSurf") return fAlSurf;
  else {
    std::ostringstream o;
    o << "OpticalSurface " << surfName << " not found!";
    G4Exception("DRsimMaterials::GetOpticalSurface","",FatalException,o.str().c_str());
  }

  return nullptr;
}

void DRsimMaterials::CreateMaterials() {
  fNistMan->FindOrBuildMaterial("G4_Galactic");
  fNistMan->FindOrBuildMaterial("G4_AIR");

  G4String symbol;
  G4double a, z, density;
  G4int ncomponents, natoms;
  G4Element* H  = new G4Element("Hydrogen",symbol="H" , z=1., a=1.01*g/mole);
  G4Element* C  = new G4Element("Carbon"  ,symbol="C" , z=6., a=12.01*g/mole);
  G4Element* N  = new G4Element("Nitrogen",symbol="N" , z=7., a=14.01*g/mole);
  G4Element* O  = new G4Element("Oxygen"  ,symbol="O" , z=8., a=16.00*g/mole);
  G4Element* F  = new G4Element("Fluorine",symbol="F" , z=9., a=18.9984*g/mole);

  // PPO
  fPPO = new G4Material("PPO", density=1.094*g/cm3, 4);
  fPPO->SetChemicalFormula("FLUOR");
  fPPO->AddElement(C, 15);
  fPPO->AddElement(H, 11);
  fPPO->AddElement(N, 1);
  fPPO->AddElement(O, 1);

  G4double fPPOMol = C->GetA()*15 + H->GetA()*11 + N->GetA()*1 + O->GetA()*1;
  G4MaterialPropertiesTable* mpPPO = new G4MaterialPropertiesTable();
  mpPPO->AddConstProperty("PPOMol", fPPOMol/g, true);
  fPPO->SetMaterialPropertiesTable(mpPPO);

  // Bis-MSB
  fBisMSB = new G4Material("Bis-MSB", density=1.3*g/cm3, 2);
  fBisMSB->SetChemicalFormula("WLS"); // Wavelength Shifter
  fBisMSB->AddElement(C, 24);
  fBisMSB->AddElement(H, 22);

  G4double fBisMol = C->GetA()*24 + H->GetA()*22;
  G4MaterialPropertiesTable* mpBisMSB = new G4MaterialPropertiesTable();
  mpBisMSB->AddConstProperty("BisMol", fBisMol/g, true);
  fBisMSB->SetMaterialPropertiesTable(mpBisMSB);
  
  // LAB
  fLAB = new G4Material("LAB", density=0.863*g/cm3, 4, kStateLiquid);
  fLAB->AddElement(C, 6);
  fLAB->AddElement(H, 5);
  fLAB->AddElement(C, 12);
  fLAB->AddElement(H, 25);
  
  G4double fLABMol = C->GetA()*18 + H->GetA()*30;
  G4MaterialPropertiesTable* mpLAB = new G4MaterialPropertiesTable();
  mpLAB->AddConstProperty("LABMol", fLABMol/g, true);
  fLAB->SetMaterialPropertiesTable(mpLAB);

  // LS
  fLS = new G4Material("LS", density=0.865*g/cm3, 3, kStateLiquid);
  fLS->AddMaterial(fLAB, 99.697*perCent);
  fLS->AddMaterial(fPPO, 0.3*perCent);
  fLS->AddMaterial(fBisMSB, 0.003*perCent);

  // Water 
  fWater = new G4Material("Water", density=1*g/cm3, 2, kStateLiquid);
  fWater->AddElement(H, 2);
  fWater->AddElement(O, 1);  

  fCu = new G4Material("Copper"  , z = 29., a = 63.546 * g/mole, density = 8.96  * g/cm3);
  fW  = new G4Material("Tungsten", z = 74., a = 183.84 * g/mole, density = 19.30 * g/cm3);
  fFe = new G4Material("Iron"    , z = 26., a = 55.845 * g/mole, density = 7.874 * g/cm3);
  fPb = new G4Material("Lead"    , z = 82., a = 207.2  * g/mole, density = 11.35 * g/cm3);

  fSi = new G4Material("Silicon", z=14., a=28.09*g/mole, density=2.33*g/cm3);
  fAl = new G4Material("Aluminum", z=13., a=26.98*g/mole, density=2.699*g/cm3);

  fVacuum = G4Material::GetMaterial("G4_Galactic");
  fAir = G4Material::GetMaterial("G4_AIR");

  fFluoPoly = new G4Material("FluorinatedPolymer", density=1.43*g/cm3, ncomponents=2);
  fFluoPoly->AddElement(C, 2);
  fFluoPoly->AddElement(F, 2);

  fGlass = new G4Material("Glass", density=1.032*g/cm3, 2);
  fGlass->AddElement(C, 91.533*perCent);
  fGlass->AddElement(H, 8.467*perCent);

  fPS = new G4Material("Polystyrene", density=1.05*g/cm3, ncomponents=2);
  fPS->AddElement(C, natoms=8);
  fPS->AddElement(H, natoms=8);

  fPMMA = new G4Material("PMMA", density= 1.19*g/cm3, ncomponents=3);
  fPMMA->AddElement(C, natoms=5);
  fPMMA->AddElement(H, natoms=8);
  fPMMA->AddElement(O, natoms=2);

  fGelatin = new G4Material("Gelatin", density=1.27*g/cm3, ncomponents=4);
  fGelatin->AddElement(C, natoms=102);
  fGelatin->AddElement(H, natoms=151);
  fGelatin->AddElement(N, natoms=31);
  fGelatin->AddElement(O, natoms=39);

  G4MaterialPropertiesTable* mpAir;
  G4MaterialPropertiesTable* mpPS;
  G4MaterialPropertiesTable* mpPMMA;
  G4MaterialPropertiesTable* mpFluoPoly;
  G4MaterialPropertiesTable* mpGlass;
  G4MaterialPropertiesTable* mpSiPM;
  G4MaterialPropertiesTable* mpPMT;
  G4MaterialPropertiesTable* mpFilter;
  G4MaterialPropertiesTable* mpFilterSurf;
  G4MaterialPropertiesTable* mpMirror;
  G4MaterialPropertiesTable* mpMirrorSurf;
  G4MaterialPropertiesTable* mpLS;
  G4MaterialPropertiesTable* mpWater;
  G4MaterialPropertiesTable* mpAlSurf;

  G4double opEn[] = { // from 900nm to 300nm with 25nm step
    1.37760*eV, 1.41696*eV, 1.45864*eV, 1.50284*eV, 1.54980*eV, 1.59980*eV, 1.65312*eV, 1.71013*eV,
    1.77120*eV, 1.83680*eV, 1.90745*eV, 1.98375*eV, 2.06640*eV, 2.15625*eV, 2.25426*eV, 2.36160*eV,
    2.47968*eV, 2.61019*eV, 2.75520*eV, 2.91728*eV, 3.09960*eV, 3.30625*eV, 3.54241*eV, 3.81490*eV, 4.13281*eV
  };

  const G4int nEnt = sizeof(opEn) / sizeof(G4double);

  G4double RI_LS[nEnt] = {
    1.47571, 1.47571, 1.47571, 1.47571, 1.47571, 1.47646, 1.47721, 1.47796,
    1.47871, 1.47987, 1.48103, 1.48218, 1.48334, 1.48527, 1.48721, 1.48914,
    1.49107, 1.49466, 1.49824, 1.50183, 1.50541, 1.51329, 1.52118, 1.52906, 1.53694 
  };

  G4double AbsLen_LS[nEnt] = {
    3.923*m, 3.923*m, 3.923*m, 3.923*m, 3.923*m, 3.172*m, 1.204*m, 4.772*m,
    7.061*m, 13.49*m, 6.377*m, 13.32*m, 15.74*m, 21.93*m, 25.25*m, 31.47*m,
    35.02*m, 38.78*m, 23.99*m, 9.086*m, 0.078*m, 0.002*m, 0.001*m, 0.001*m, 0.001*m
  };

  G4double scintFast_LS[nEnt] = {
    0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00,
    0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.01, 0.02,
    0.06, 0.18, 0.49, 1.00, 0.93, 0.05, 0.01, 0.00, 0.00
  };

  G4double reemit_LS[nEnt] = {
    0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00,
    0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00,
    0.00, 0.00, 0.00, 0.18, 0.99, 0.99, 0.99, 1.00, 1.00
  };

  mpLS = new G4MaterialPropertiesTable();
  mpLS->AddProperty("RINDEX", opEn, RI_LS, nEnt);
  mpLS->AddProperty("ABSLENGTH",opEn,AbsLen_LS,nEnt);
  // Geant4 11.x: use SCINTILLATIONCOMPONENT1 instead of FASTCOMPONENT
  mpLS->AddProperty("SCINTILLATIONCOMPONENT1",opEn,scintFast_LS,nEnt);
  mpLS->AddProperty("WLSCOMPONENT", opEn,reemit_LS,nEnt);
  // Geant4 11.x: use SCINTILLATIONTIMECONSTANT1 instead of FASTTIMECONSTANT
  mpLS->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0*ns);
  mpLS->AddConstProperty("SCINTILLATIONYIELD", 9656./MeV);
  mpLS->AddConstProperty("RESOLUTIONSCALE",1.0);
  fLS->SetMaterialPropertiesTable(mpLS);
  fLS->GetIonisation()->SetBirksConstant(0.117*mm/MeV);

  // Water Refractive Index
  G4double opEn_RI_Water[] = { // from 800nm to 200nm with 50nm step
    1.54980*eV, 1.65312*eV, 1.77120*eV, 1.90745*eV, 2.06640*eV, 2.25426*eV,
    2.47968*eV, 2.75520*eV, 3.09960*eV, 3.54241*eV, 4.13281*eV, 4.95937*eV, 6.19921*eV
  };
  
  const G4int RIEnt_Water = sizeof(opEn_RI_Water) / sizeof(G4double);

  G4double RI_Water[RIEnt_Water] = { 
    1.3292, 1.32986, 1.33065, 1.33165, 1.33293, 1.33458,
    1.33676, 1.3397, 1.34378, 1.34978, 1.35942, 1.37761, 1.42516
  };

  // Water Absorption length
  G4double opEn_Abs_Water[] = {1.54980*eV, 6.19921*eV};
  const G4int AbsEnt_Water = sizeof(opEn_Abs_Water) / sizeof(G4double);
  G4double AbsLen_Water[AbsEnt_Water] = {10.*m, 10.*m};

  mpWater = new G4MaterialPropertiesTable();
  mpWater->AddProperty("RINDEX",opEn_RI_Water,RI_Water,RIEnt_Water);
  mpWater->AddProperty("ABSLENGTH",&(opEn_Abs_Water[0]),&(AbsLen_Water[0]),AbsEnt_Water);
  fWater->SetMaterialPropertiesTable(mpWater);

  // G4double opEn[] = { // from 900nm to 300nm with 25nm step
  //   1.37760*eV, 1.41696*eV, 1.45864*eV, 1.50284*eV, 1.54980*eV, 1.59980*eV, 1.65312*eV, 1.71013*eV,
  //   1.77120*eV, 1.83680*eV, 1.90745*eV, 1.98375*eV, 2.06640*eV, 2.15625*eV, 2.25426*eV, 2.36160*eV,
  //   2.47968*eV, 2.61019*eV, 2.75520*eV, 2.91728*eV, 3.09960*eV, 3.30625*eV, 3.54241*eV, 3.81490*eV, 4.13281*eV
  // };

  // const G4int nEnt = sizeof(opEn) / sizeof(G4double);

  // Aluminium Reflection - Surface logical
  G4double AlRef[nEnt]; std::fill_n(AlRef, nEnt, 0.95);
  G4double AlEff[nEnt]; std::fill_n(AlEff, nEnt, 0.);

  mpAlSurf = new G4MaterialPropertiesTable();
  mpAlSurf->AddProperty("TRANSMITTANCE",opEn,AlEff,nEnt);
  mpAlSurf->AddProperty("REFLECTIVITY",opEn,AlRef,nEnt);

  fAlSurf = new G4OpticalSurface("AlSurf",glisur,polished,dielectric_metal);
  fAlSurf->SetMaterialPropertiesTable(mpAlSurf);

  G4double RI_Air[nEnt]; std::fill_n(RI_Air,nEnt,1.0);
  mpAir = new G4MaterialPropertiesTable();
  mpAir->AddProperty("RINDEX",opEn,RI_Air,nEnt);
  fAir->SetMaterialPropertiesTable(mpAir);
  
  G4double RI_PMMA[nEnt] = {
    1.48329, 1.48355, 1.48392, 1.48434, 1.48467, 1.48515, 1.48569, 1.48628,
    1.48677, 1.48749, 1.48831, 1.48899, 1.49000, 1.49119, 1.49219, 1.49372,
    1.49552, 1.49766, 1.49953, 1.50252, 1.50519, 1.51000, 1.51518, 1.52182, 1.53055
  };
  G4double AbsLen_PMMA[nEnt] = {
    0.414*m, 0.543*m, 0.965*m, 2.171*m, 2.171*m, 3.341*m, 4.343*m, 1.448*m,
    4.343*m, 14.48*m, 21.71*m, 8.686*m, 28.95*m, 54.29*m, 43.43*m, 48.25*m,
    54.29*m, 48.25*m, 43.43*m, 28.95*m, 21.71*m, 4.343*m, 2.171*m, 0.869*m, 0.434*m
  };

  mpPMMA = new G4MaterialPropertiesTable();
  mpPMMA->AddProperty("RINDEX",opEn,RI_PMMA,nEnt);
  mpPMMA->AddProperty("ABSLENGTH",opEn,AbsLen_PMMA,nEnt);
  fPMMA->SetMaterialPropertiesTable(mpPMMA);

  G4double RI_FluoPoly[nEnt]; std::fill_n(RI_FluoPoly, nEnt, 1.42);
  mpFluoPoly = new G4MaterialPropertiesTable();
  mpFluoPoly->AddProperty("RINDEX",opEn,RI_FluoPoly,nEnt);
  fFluoPoly->SetMaterialPropertiesTable(mpFluoPoly);

  G4double RI_PS[nEnt] = {
    1.57483, 1.57568, 1.57644, 1.57726, 1.57817, 1.57916, 1.58026, 1.58148,
    1.58284, 1.58435, 1.58605, 1.58796, 1.59013, 1.59328, 1.59621, 1.59960,
    1.60251, 1.60824, 1.61229, 1.62032, 1.62858, 1.63886, 1.65191, 1.66888, 1.69165
  };
  G4double AbsLen_PS[nEnt] = {
    2.714*m, 3.102*m, 3.619*m, 4.343*m, 5.791*m, 7.896*m, 4.343*m, 7.896*m,
    5.429*m, 36.19*m, 17.37*m, 36.19*m, 5.429*m, 28.95*m, 21.71*m, 14.48*m,
    12.41*m, 8.686*m, 7.238*m, 1.200*m, 0.200*m, 0.500*m, 0.200*m, 0.100*m, 0.100*m
  };
  G4double scintFast_PS[nEnt] = {
    0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00,
    0.00, 0.00, 0.00, 0.00, 0.00, 0.03, 0.07, 0.13,
    0.33, 0.63, 1.00, 0.50, 0.00, 0.00, 0.00, 0.00, 0.00
  };
  mpPS = new G4MaterialPropertiesTable();
  mpPS->AddProperty("RINDEX",opEn,RI_PS,nEnt);
  mpPS->AddProperty("ABSLENGTH",opEn,AbsLen_PS,nEnt);
  // Geant4 11.x: use SCINTILLATIONCOMPONENT1 instead of FASTCOMPONENT
  mpPS->AddProperty("SCINTILLATIONCOMPONENT1",opEn,scintFast_PS,nEnt);
  mpPS->AddConstProperty("SCINTILLATIONYIELD",10./keV);
  mpPS->AddConstProperty("RESOLUTIONSCALE",1.0);
  // Geant4 11.x: use SCINTILLATIONTIMECONSTANT1 instead of FASTTIMECONSTANT
  mpPS->AddConstProperty("SCINTILLATIONTIMECONSTANT1",2.8*ns);
  fPS->SetMaterialPropertiesTable(mpPS);
  fPS->GetIonisation()->SetBirksConstant(0.126*mm/MeV);

  G4double RI_Glass[nEnt]; std::fill_n(RI_Glass, nEnt, 1.52);
  G4double Abslength_Glass[nEnt]; std::fill_n(Abslength_Glass, nEnt, 420.*cm);
  mpGlass = new G4MaterialPropertiesTable();
  mpGlass->AddProperty("RINDEX",opEn,RI_Glass,nEnt);
  mpGlass->AddProperty("ABSLENGTH",opEn,Abslength_Glass,nEnt);
  fGlass->SetMaterialPropertiesTable(mpGlass);

  // TODO: Change to our PMT info.
  G4double refl_SiPM[nEnt]; std::fill_n(refl_SiPM, nEnt, 0.);
  G4double eff_SiPM[nEnt] = {
    0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09, 0.10,
    0.11, 0.13, 0.15, 0.17, 0.19, 0.20, 0.22, 0.23,
    0.24, 0.25, 0.24, 0.23, 0.21, 0.20, 0.17, 0.14, 0.10
  };

  mpSiPM = new G4MaterialPropertiesTable();
  mpSiPM->AddProperty("REFLECTIVITY",opEn,refl_SiPM,nEnt);
  mpSiPM->AddProperty("EFFICIENCY",opEn,eff_SiPM,nEnt);
  fSiPMSurf = new G4OpticalSurface("SiPMsurf",glisur,polished,dielectric_metal);
  fSiPMSurf->SetMaterialPropertiesTable(mpSiPM);

  G4double refl_PMT[nEnt]; std::fill_n(refl_PMT, nEnt, 0.);
  G4double eff_PMT[nEnt] = {
    0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00,
    0.00, 0.00, 0.01, 0.01, 0.02, 0.04, 0.07, 0.12,
    0.20, 0.24, 0.29, 0.33, 0.34, 0.35, 0.35, 0.33, 0.29
  };
  mpPMT = new G4MaterialPropertiesTable();
  mpPMT->AddProperty("REFLECTIVITY",opEn,refl_PMT,nEnt);
  mpPMT->AddProperty("EFFICIENCY",opEn,eff_PMT,nEnt);
  fPMTSurf = new G4OpticalSurface("PMTSurf",glisur,polished,dielectric_metal);
  fPMTSurf->SetMaterialPropertiesTable(mpPMT);

  G4double filterEff[nEnt] = {
    0.913, 0.913, 0.913, 0.913, 0.913, 0.913, 0.913, 0.913, 
    0.913, 0.912, 0.910, 0.907, 0.904, 0.899, 0.884, 0.692, 
    0.015, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000
  };
  
  G4double filterRef[nEnt]; std::fill_n(filterRef,nEnt,0.);
  G4double RI_gel[nEnt]; std::fill_n(RI_gel,nEnt,1.52);
  mpFilter = new G4MaterialPropertiesTable();
  mpFilter->AddProperty("RINDEX",opEn,RI_gel,nEnt);
  fGelatin->SetMaterialPropertiesTable(mpFilter);
  mpFilterSurf = new G4MaterialPropertiesTable();
  mpFilterSurf->AddProperty("TRANSMITTANCE",opEn,filterEff,nEnt);
  mpFilterSurf->AddProperty("REFLECTIVITY",opEn,filterRef,nEnt);
  fFilterSurf = new G4OpticalSurface("FilterSurf",glisur,polished,dielectric_dielectric);
  fFilterSurf->SetMaterialPropertiesTable(mpFilterSurf);

  G4double MirrorRef[nEnt]; std::fill_n(MirrorRef, nEnt, 0.9);
  G4double MirrorEff[nEnt]; std::fill_n(MirrorEff, nEnt, 0.);

  mpMirrorSurf = new G4MaterialPropertiesTable();
  mpMirrorSurf->AddProperty("TRANSMITTANCE",opEn,MirrorEff,nEnt);
  mpMirrorSurf->AddProperty("REFLECTIVITY",opEn,MirrorRef,nEnt);
  fMirrorSurf = new G4OpticalSurface("MirrorSurf",glisur,polished,dielectric_metal);
  fMirrorSurf->SetMaterialPropertiesTable(mpMirrorSurf);
}