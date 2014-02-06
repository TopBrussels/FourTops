#include "TNtuple.h"
#include "TFile.h"
#include "TH1F.h"
#include <iostream>

int main(){

 float MVA, ScaleFactor,NormFactor, Luminosity, Njets, Ntags, HTb, HTX, HTH;
 Int_t nentries;

 TFile * f_ttjets = new TFile("QuadTuple_TTJets.root");
 TNtuple *craneen_ttjets = (TNtuple*)f_ttjets->Get("QuadTuple_TTJets");
 TH1F * h_ttjets = new TH1F("ttjets","ttjets", 15, -0.3,0.4);

 TFile * f_data = new TFile("QuadTuple_Data.root");
 TNtuple *craneen_data = (TNtuple*)f_data->Get("QuadTuple_Data");
 TH1F * h_data = new TH1F("data","data", 15, -0.3,0.4);
 Luminosity = 19600.9;

 nentries = (Int_t)craneen_ttjets->GetEntries();

 craneen_ttjets->SetBranchAddress("MVA",&MVA);
 craneen_ttjets->SetBranchAddress("ScaleFactor",&ScaleFactor);
 craneen_ttjets->SetBranchAddress("NormFactor",&NormFactor);
 craneen_ttjets->SetBranchAddress("Luminosity",&Luminosity);


 for (Int_t i = 0; i < nentries; i++){
   craneen_ttjets->GetEntry(i);
   h_ttjets->Fill(MVA,ScaleFactor*NormFactor*Luminosity);

   cout <<MVA<<endl;
}

 nentries = (Int_t)craneen_data->GetEntries();

 craneen_data->SetBranchAddress("MVA",&MVA);

 for (Int_t i = 0; i < nentries; i++){
   craneen_data->GetEntry(i);
   h_data->Fill(MVA);

   cout <<MVA<<endl;
}






 h_ttjets->SetFillColor(kRed);

 h_ttjets->Draw();
 h_data->Draw("E1psame");
}
