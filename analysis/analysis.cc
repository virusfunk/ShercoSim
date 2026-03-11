#include "RootInterface.h"
#include "RecoInterface.h"
#include "DRsimInterface.h"
#include "functions.h"

#include "TROOT.h"
#include "TStyle.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TPaveStats.h"
#include "TString.h"
#include "TLorentzVector.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <tuple>
#include "Riostream.h"


int main(int argc, char* argv[]) {

  TString filename = argv[1];
  float low = std::stof(argv[2]);
  float high = std::stof(argv[3]);
  bool doCalib = false;
  if (argc > 4) {
    std::string opt = argv[4];
    if (opt == "1") doCalib = true;
    else if (opt == "0") doCalib = false;
  }

  const int row = 27; 
  const int col = 27;
  int numModule = row * col;

  gStyle->SetOptFit(1);

  double ceren_cc, scint_cc;
  std::pair<double, double> fCalibs;
  if (!doCalib) {
    fCalibs = std::make_pair(1.0, 1.0);
  } else {
    std::ifstream in;
    in.open("calib.csv", std::ios::in);
    bool readOk = false;
    while (true) {
      in >> ceren_cc >> scint_cc;
      if ( !in.good() ) break;
      fCalibs = std::make_pair(ceren_cc, scint_cc);
      readOk = true;
    }
    in.close();
    if (!readOk) {
      fCalibs = std::make_pair(1.0, 1.0);
    }
  }

  TH1F* tEdep = new TH1F("Total_Edep","Total Energy deposit;MeV;Evt",100,low*1000.,high*1000.);
  tEdep->Sumw2(); tEdep->SetLineColor(kBlack); tEdep->SetLineWidth(2);
  TH1F* tE_C = new TH1F("E_C","Energy of Cerenkov.;GeV;Evt",100,low,high);
  tE_C->Sumw2(); tE_C->SetLineColor(kBlue); tE_C->SetLineWidth(2);
  TH1F* tE_S = new TH1F("E_S","Energy of Scintillation.;GeV;Evt",100,low,high);
  tE_S->Sumw2(); tE_S->SetLineColor(kRed); tE_S->SetLineWidth(2);
  TH1F* tE_SC = new TH1F("E_SC","E_{S}+E_{C};GeV;Evt",100,2.*low,2.*high);
  tE_SC->Sumw2(); tE_SC->SetLineColor(kBlack); tE_SC->SetLineWidth(2);
  TH1F* tEdep_oneTower = new TH1F("Tower_Edep","Total Energy deposit;MeV;Evt",200,0.,100.);  
  tEdep_oneTower->Sumw2(); tEdep_oneTower->SetLineColor(kBlack); tEdep_oneTower->SetLineWidth(2);
  TH1F* tCtime = new TH1F("Total_C_Time","Total timing of Cerenkov.;ns;Evt",150,0,30);
  tCtime->Sumw2(); tCtime->SetLineColor(kBlue); tCtime->SetLineWidth(2);
  TH1F* tStime = new TH1F("Total_S_Time","Total timing of Scintillation.;ns;Evt",150,0,30);
  tStime->Sumw2(); tStime->SetLineColor(kRed); tStime->SetLineWidth(2);
  TH1I* tChit = new TH1I("Total_C_Hit","Total hits of Cerenkov",100,0.,100000.) ;
  tChit->Sumw2(); tChit->SetLineColor(kBlue); tChit->SetLineWidth(2);
  TH1I* tShit = new TH1I("Total_S_Hit","Total hits of Scintillation",100,0.,100000.);
  tShit->Sumw2(); tShit->SetLineColor(kRed); tShit->SetLineWidth(2);
  TH1F* tP_leak = new TH1F("Pleak","Momentum leak;MeV;Evt",100,0.,1000.*high);
  tP_leak->Sumw2(); tP_leak->SetLineWidth(2);
  TH1F* tP_leak_nu = new TH1F("Pleak_nu","Neutrino energy leak;MeV;Evt",100,0.,1000.*high);
  tP_leak_nu->Sumw2(); tP_leak_nu->SetLineWidth(2);
  
  TH2F* tEdep_2D = new TH2F("Edep_2D",";;", col, 0, col, row, 0, row);
  TH2F* tHits_2D = new TH2F("Hits_2D",";;", col, 0, col, row, 0, row);
  TH2F* tE_2D = new TH2F("Energy_2D",";;", col, 0, col, row, 0, row);

  TH1F* tEdep_Towers[numModule];
  TH1F* tHits_Towers[numModule];
  TString nameEdep;
  TString nameHits;
  for (int i = 0; i < numModule; i++) {
    nameEdep = std::to_string(i) + "_Tower_Edep";
    nameHits = std::to_string(i) + "_Tower_Hits";
    tEdep_Towers[i] = new TH1F(nameEdep, ";MeV;Evt", 1000, 0., 1000000.);
    tHits_Towers[i] = new TH1F(nameHits, ";Npe;Evt", 1000, 0., 1000000.);
  }

  RootInterface<DRsimInterface::DRsimEventData>* drInterface = new RootInterface<DRsimInterface::DRsimEventData>("/your/path/ele_" + std::string(filename) + ".root", 1);
  drInterface->set("DRsim","DRsimEventData");

  unsigned int entries = drInterface->entries();
  while (drInterface->numEvt() < entries) {

    if (drInterface->numEvt() % 100 == 0) printf("Analyzing %dth event ...\n", drInterface->numEvt());

    DRsimInterface::DRsimEventData drEvt;
    drInterface->read(drEvt);

    float ftEdep = 0.;
    float Edep_Towers[numModule] = {0};
    float hits_Towers[numModule] = {0};
    float E_Towers[numModule] = {0};

    for (auto edepItr = drEvt.Edeps.begin(); edepItr != drEvt.Edeps.end(); ++edepItr) {
      auto edep = *edepItr;
      ftEdep += edep.Edep;

      int moduleNum = edep.ModuleNum;
      Edep_Towers[moduleNum] += edep.Edep;
    }

    float Pleak = 0.;
    float muak_nu = 0.;
    for (auto leak : drEvt.leaks) {
      TLorentzVector leak4vec;
      leak4vec.SetPxPyPzE(leak.px,leak.py,leak.pz,leak.E);
      if ( std::abs(leak.pdgId)==12 || std::abs(leak.pdgId)==14 || std::abs(leak.pdgId)==16 ) {
        muak_nu += leak4vec.P();
      } else {
        Pleak += leak4vec.P();
      }
    }
    tP_leak->Fill(Pleak);
    tP_leak_nu->Fill(muak_nu);

    float energy_C = 0.;
    float energy_S = 0.;
    int ftC_hits = 0;
    int ftS_hits = 0;

    for (auto towerItr = drEvt.towers.begin(); towerItr != drEvt.towers.end(); ++towerItr) {
      
      auto sipmItr = *towerItr;
      std::vector<DRsimInterface::DRsimSiPMData> sipmData = sipmItr.SiPMs;
      int nModule = sipmItr.ModuleNum;

      for (int i = 0; i < sipmData.size(); i++) {

        DRsimInterface::DRsimTimeStruct timeItr = sipmData[i].timeStruct;

        for(auto TmpItr = timeItr.begin(); TmpItr != timeItr.end(); ++TmpItr) {
          auto timeData = *TmpItr;

          if(DRsimInterface::IsCerenkov(nModule)) {
            tCtime->Fill((timeData.first.first + timeData.first.second)/2., timeData.second);
            ftC_hits += timeData.second;
            hits_Towers[nModule] += timeData.second;
            E_Towers[nModule] += timeData.second / fCalibs.first;
          } else {
            tStime->Fill((timeData.first.first + timeData.first.second)/2., timeData.second);
            ftS_hits += timeData.second;
            hits_Towers[nModule] += timeData.second;
            E_Towers[nModule] += timeData.second / fCalibs.second;
          }
        }
      }
    }

    energy_C += ftC_hits / fCalibs.first;
    energy_S += ftS_hits / fCalibs.second;

    tE_C->Fill(energy_C);
    tE_S->Fill(energy_S);
    tE_SC->Fill(energy_C + energy_S);
    tEdep->Fill(ftEdep);
    tChit->Fill(ftC_hits);
    tShit->Fill(ftS_hits);
    tEdep_oneTower->Fill(Edep_Towers[351]);

    for (int i = 0; i < numModule; i++) {
      int xIdx = (i / row);
      int yIdx = (i % row);
    
      tEdep_Towers[i]->Fill(Edep_Towers[i]);
      tHits_Towers[i]->Fill(hits_Towers[i]);
      tEdep_2D->Fill(xIdx, yIdx, Edep_Towers[i]/entries);
      tHits_2D->Fill(xIdx, yIdx, hits_Towers[i]/entries);
      tE_2D->Fill(xIdx, yIdx, E_Towers[i]/entries);
    }
  } // End of event loop

  std::vector<std::pair<int, double>> dataEdep;
  for (int i = 0; i < numModule; i++ ) {
    dataEdep.push_back(std::make_pair(i, tEdep_Towers[i]->GetMean()));
  }

  std::vector<std::pair<int, double>> dataHits;
  for (int i = 0; i < numModule; i++ ) {
    dataHits.push_back(std::make_pair(i, tHits_Towers[i]->GetMean()));
  }

  std::sort(dataEdep.begin(), dataEdep.end(), [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
    return a.first < b.first;
  });

  std::sort(dataHits.begin(), dataHits.end(), [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
    return a.first < b.first;
  });

  std::ofstream outEdep;
  outEdep.open("/your/path/ele_" + filename + "_Edep.csv", std::ios::out | std::ios::app);
  outEdep << "Total Edep : " << tEdep->GetMean() << " MeV" << std::endl;
  for (const auto& itr : dataEdep) {
    outEdep << "Module_" << (itr.first) << " " << itr.second << std::endl;
  }

  std::ofstream outHits;
  outHits.open("/your/path/ele_" + filename + "_Hits.csv", std::ios::out | std::ios::app);
  outHits << "Total Chits : " << tChit->GetMean() << std::endl;
  outHits << "Total Shits : " << tShit->GetMean() << std::endl;
  for (const auto& itr : dataHits) {
    if (itr.first % 2 == 0) {
      outHits << "C " << "Module_" << (itr.first) << " " << itr.second << std::endl;
    } else {
      outHits << "S " << "Module_" << (itr.first) << " " << itr.second << std::endl;
    }
  }

  TFile* file = new TFile("/your/path/ele_" + filename + "_fit.root", "RECREATE");
  TCanvas* c = new TCanvas("c","");

  c->SetLogy(1);
  tP_leak->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_Pleak.png");
  tP_leak_nu->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_Pleak_nu.png");
  c->SetLogy(0);

  tEdep->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_TotalEdep.png");
  tE_C->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_TotalE_C.png");
  tE_S->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_TotalE_S.png");
  tChit->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_TotalChit.png");
  tShit->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_TotalShit.png");
  tCtime->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_TotalCtime.png");
  tStime->Draw("Hist"); c->SaveAs("/your/path/ele_" + filename + "_TotalStime.png");

  tE_C->Write();
  tE_S->Write();
  tE_SC->Write();

  TF1* grE_C = new TF1("Cfit","gaus",low,high); grE_C->SetLineColor(kBlue);
  TF1* grE_S = new TF1("Sfit","gaus",low,high); grE_S->SetLineColor(kRed);
  TF1* grE_SC = new TF1("S+Cfit","gaus",2.*low,2.*high); grE_SC->SetLineColor(kBlack);
  tE_C->SetOption("p"); tE_C->Fit(grE_C,"R+&same");
  tE_S->SetOption("p"); tE_S->Fit(grE_S,"R+&same");
  tE_SC->SetOption("p"); tE_SC->Fit(grE_SC,"R+&same");

  tE_SC->Draw(""); c->SaveAs("/your/path/ele_" + filename + "_TotalE_SC.png");

  c->cd();
  tE_S->SetTitle("");
  tE_S->Draw(""); c->Update();
  TPaveStats* statsE_S = (TPaveStats*)c->GetPrimitive("stats");
  statsE_S->SetName("Scint");
  statsE_S->SetTextColor(kRed);
  statsE_S->SetX1NDC(.7);
  statsE_S->SetY1NDC(.4); statsE_S->SetY2NDC(.7);

  tE_C->Draw("sames"); c->Update();
  TPaveStats* statsE_C = (TPaveStats*)c->GetPrimitive("stats");
  statsE_C->SetName("Cerenkov");
  statsE_C->SetTextColor(kBlue);
  statsE_C->SetX1NDC(.7);
  statsE_C->SetY1NDC(.7); statsE_C->SetY2NDC(1.);
  c->SaveAs("/your/path/ele_" + filename + "_Ecs.png");

  gStyle->SetPaintTextFormat("4.1f");
  c->cd();
  c->SetRightMargin(0.2);
  c->SetLeftMargin(0.15);
  tEdep_2D->GetXaxis()->SetLabelFont(42);
  tEdep_2D->GetYaxis()->SetLabelFont(42);
  tHits_2D->GetXaxis()->SetLabelFont(42);
  tHits_2D->GetYaxis()->SetLabelFont(42);
  tE_2D->GetXaxis()->SetLabelFont(42);
  tE_2D->GetYaxis()->SetLabelFont(42);

  // For 4 by 5
  // c->SetCanvasSize(1200,1200);
  // tEdep_2D->SetMarkerSize(1.2);

  // For 27 by 27
  c->SetCanvasSize(1800,1400);
  tEdep_2D->SetMarkerSize(0.4);
  tEdep_2D->GetXaxis()->SetLabelSize(0.025);
  tEdep_2D->GetYaxis()->SetLabelSize(0.025);
  tHits_2D->SetMarkerSize(0.4);
  tHits_2D->GetXaxis()->SetLabelSize(0.025);
  tHits_2D->GetYaxis()->SetLabelSize(0.025);
  tE_2D->SetMarkerSize(0.4);
  tE_2D->GetXaxis()->SetLabelSize(0.025);
  tE_2D->GetYaxis()->SetLabelSize(0.025);

  for (int i = 1; i <= col; i++) {
    tEdep_2D->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    tHits_2D->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    tE_2D->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
  }
  for (int i = 1; i <= row; i++) {
    tEdep_2D->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
    tHits_2D->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
    tE_2D->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
  }

  tEdep_2D->Draw("COL0Z text");
  tEdep_2D->SetStats(0);
  c->SaveAs("/your/path/ele_" + filename + "_Edep2D.pdf");

  c->SetLogz(1);

  tEdep_2D->Draw("COL0Z TEXT"); 
  tEdep_2D->SetStats(0);
  c->SaveAs("/your/path/ele_" + filename + "_Edep2D_Log.pdf"); 

  tHits_2D->Draw("COL0Z TEXT"); 
  tHits_2D->SetStats(0);
  c->SaveAs("/your/path/ele_" + filename + "_Hits2D_Log.pdf"); 

  tE_2D->Draw("COL0Z TEXT"); 
  tE_2D->SetStats(0);
  c->SaveAs("/your/path/ele_" + filename + "_E2D_Log.pdf"); 
  
  c->SetLogz(0);
  tEdep->SetOption("HIST");
  tEdep_oneTower->SetOption("HIST");

  grE_C->Write();
  grE_S->Write();
  grE_SC->Write();

  file->Close();
}
