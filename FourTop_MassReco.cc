/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
////         Analysis code for search for Four Top Production.                           ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#define _USE_MATH_DEFINES

#include "TStyle.h"
#include "TPaveText.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "TRandom3.h"
#include "TRandom.h"
#include <iostream>
#include <map>
#include <cstdlib> 

//user code
#include "TopTreeProducer/interface/TRootRun.h"
#include "TopTreeProducer/interface/TRootEvent.h"
#include "TopTreeAnalysisBase/Selection/interface/SelectionTable.h"
#include "TopTreeAnalysisBase/Content/interface/AnalysisEnvironment.h"
#include "TopTreeAnalysisBase/Content/interface/Dataset.h"
#include "TopTreeAnalysisBase/Tools/interface/JetTools.h"
#include "TopTreeAnalysisBase/Tools/interface/PlottingTools.h"
#include "TopTreeAnalysisBase/Tools/interface/MultiSamplePlot.h"
#include "TopTreeAnalysisBase/Tools/interface/TTreeLoader.h"
#include "TopTreeAnalysisBase/Tools/interface/AnalysisEnvironmentLoader.h"
#include "TopTreeAnalysisBase/Tools/interface/JetCombiner.h"
#include "TopTreeAnalysisBase/Tools/interface/MVATrainer.h"
#include "TopTreeAnalysisBase/Tools/interface/MVAComputer.h"
#include "TopTreeAnalysisBase/Tools/interface/JetTools.h"

#include "TopTreeAnalysis/macros/Style.C"

#include "TGraphAsymmErrors.h"


using namespace std;
using namespace TopTree;
//using namespace reweight;

bool split_ttbar = false;

pair<float, vector<unsigned int> > MVAvals1;	
pair<float, vector<unsigned int> > MVAvals2;	
pair<float, vector<unsigned int> > MVAvals2ndPass;
int nMVASuccesses=0;
int nMatchedEvents=0;

/// Normal Plots (TH1F* and TH2F*)
map<string,TH1F*> histo1D;
map<string,TH2F*> histo2D;

/// MultiSamplePlot
map<string,MultiSamplePlot*> MSPlot;

/// MultiPadPlot
map<string,MultiSamplePlot*> MultiPadPlot;

//TGraphAsymmErrors
map<string,TGraphAsymmErrors*> tgraph;


struct HighestTCHEBtag{
    bool operator()( TRootJet* j1, TRootJet* j2 ) const{
    	return j1->btag_trackCountingHighEffBJetTags() > j2->btag_trackCountingHighEffBJetTags();
    }
};
struct HighestCVSBtag{
    bool operator()( TRootJet* j1, TRootJet* j2 ) const{
    	return j1->btag_combinedSecondaryVertexBJetTags() > j2->btag_combinedSecondaryVertexBJetTags();
    }
};

bool match;

int Factorial(int N);

int main (int argc, char *argv[])
{
    TRandom3* rand = new TRandom3();


  string btagger = "CSVM";
// b-tag scalefactor => TCHEL: data/MC scalefactor = 0.95 +- 0.10,    TCHEM: data/MC scalefactor = 0.94 +- 0.09
// mistag scalefactor => TCHEL: data/MC scalefactor = 1.11 +- 0.12,    TCHEM: data/MC scalefactor = 1.21 +- 0.17
  float scalefactorbtageff, mistagfactor;

  float workingpointvalue = 9999; //working points updated to 2012 BTV-POG recommendations.
 
  if(btagger == "TCHPM"  || btagger == "TCHET"  ||  btagger == "SSV" ){
    cout<<"This tagger ("<< btagger <<")is not commisioned in 2012, please use CSV, TCHP or JetProb"<<endl;
    exit(1); 
  }
  else if(btagger == "TCHPL")
     workingpointvalue = 1.470;
  else if(btagger == "TCHPT")
    workingpointvalue = 3.42;
  else if(btagger == "CSVL")
     workingpointvalue = .244;	
  else if(btagger == "CSVM")
    workingpointvalue = .679;
  else if(btagger == "CSVT")
    workingpointvalue = .898;

  clock_t start = clock();

 cout << "*************************************************************" << endl;
  cout << " Beginning of the program for the FourTop search ! "           << endl;
  cout << "*************************************************************" << endl;

  //SetStyle if needed
//  setTDRStyle();
  setGregStyle();
  //setMyStyle();

  string postfix = "_MassReco"; // to relabel the names of the output file

  ///////////////////////////////////////
  // Configuration
  ///////////////////////////////////////

  string channelpostfix = "";
  string xmlFileName = "";

  bool Electron = false; // use Electron channel?
  bool Muon = true; // use Muon channel?
  if(Electron && Muon){
	cout << "  --> Using both Muon and Electron channel? Choose only one ( since different samples/skims are required)!" << endl;
	exit(1);
  }

  if(Muon){
	cout << " --> Using the Muon channel..." << endl;
	channelpostfix = "_Mu";
	xmlFileName = "../config/myTopFCNCconfig_Muon.xml";
  }
  else if(Electron){
	cout << " --> Using the Electron channel..." << endl;
	channelpostfix = "_El";
	xmlFileName = "../config/myTopFCNCconfig_Electron.xml";
  }

    xmlFileName = "config/test_mconly.xml";
    
  const char *xmlfile = xmlFileName.c_str();
  cout << "used config file: " << xmlfile << endl;    

/////////////////////////////
/// AnalysisEnvironment
/////////////////////////////

  AnalysisEnvironment anaEnv;
  cout<<" - Loading environment ..."<<endl;
  AnalysisEnvironmentLoader anaLoad(anaEnv,xmlfile);
  int verbose = 2;//anaEnv.Verbose;

////////////////////////////////
//  Load datasets
////////////////////////////////
    
  TTreeLoader treeLoader;
  vector < Dataset* > datasets;
  cout << " - Loading datasets ..." << endl;
  treeLoader.LoadDatasets (datasets, xmlfile);
  float Luminosity = 5343.64; //pb^-1??
  vector<string> MVAvars;

  //A few bools to steer the MassReco and Event MVAs
  string MVAmethod = "BDT"; // MVAmethod to be used to get the good jet combi calculation (not for training! this is chosen in the jetcombiner class)
  bool TrainMVA = true; // If false, the previously trained MVA will be used to calculate stuff

    cout <<"Instantiating jet combiner..."<<endl;

  JetCombiner* jetCombiner = new JetCombiner(TrainMVA, Luminosity, datasets, MVAmethod, false);
    cout <<"Instantiated jet combiner..."<<endl;
    double bestTopMass1 =0.;
    double bestTopMass2 = 0.;
    double bestTopMass2ndPass = 0.;
    double bestTopPt =0.;
    
  for (unsigned int d = 0; d < datasets.size (); d++)
  {
     cout <<"found sample with equivalent lumi "<<  datasets[d]->EquivalentLumi() <<endl;
     string dataSetName = datasets[d]->Name();
   if(dataSetName.find("Data")<=0 || dataSetName.find("data")<=0 || dataSetName.find("DATA")<=0)
	{
		  Luminosity = datasets[d]->EquivalentLumi();
		  cout <<"found DATA sample with equivalent lumi "<<  datasets[d]->EquivalentLumi() <<endl;
		  break;
	 }
  }

  cout << "Rescaling to an integrated luminosity of "<< Luminosity <<" pb^-1" << endl;

    // for splitting the ttbar sample, it is essential to have the ttjets sample as the last
    //dataset loaded
    if (split_ttbar){
        int ndatasets = datasets.size() - 1 ;
        cout << " - splitting TTBar dataset ..." << ndatasets   << endl;
        vector<string> ttbar_filenames = datasets[ndatasets]->Filenames();
        cout <<"ttbar filenames =  "<< ttbar_filenames[0] <<endl;
        
        Dataset* ttbar_ll = new Dataset("TTJets_ll","tt + ll" , true, 633, 2, 2, 1, 213.4,ttbar_filenames );
        Dataset* ttbar_cc = new Dataset("TTJets_cc","tt + cc" , true, 633, 2, 2, 1, 6.9, ttbar_filenames );
        Dataset* ttbar_bb = new Dataset("TTJets_bb","tt + bb" , true, 633, 2, 2, 1, 4.8, ttbar_filenames );
        
        ttbar_ll->SetEquivalentLuminosity(223000.0);
        ttbar_cc->SetEquivalentLuminosity(223000.0);
        ttbar_bb->SetEquivalentLuminosity(223000.0);
        
        ttbar_ll->SetColor(kRed);
        ttbar_cc->SetColor(kRed-3);
        ttbar_bb->SetColor(kRed+2);
        
        
        datasets.pop_back();
        datasets.push_back(ttbar_bb);
        datasets.push_back(ttbar_cc);
        datasets.push_back(ttbar_ll);     
    }     
    
  //Output ROOT file
  string rootFileName ("FourTop"+postfix+channelpostfix+".root");
  TFile *fout = new TFile (rootFileName.c_str(), "RECREATE");

  //vector of objects
  cout << " - Variable declaration ..." << endl;
  vector < TRootVertex* >   vertex;
  vector < TRootMuon* >     init_muons;
  vector < TRootElectron* > init_electrons;
  vector < TRootJet* >      init_jets;
  vector < TRootMET* >      mets;

  //Global variable
  TRootEvent* event = 0;
    
    ////////////////////////////////////////////////////////////////////
    ////////////////// MultiSample plots  //////////////////////////////
    ////////////////////////////////////////////////////////////////////
    MSPlot["RhoCorrection"]              = new MultiSamplePlot(datasets, "RhoCorrection", 100, 0, 100, "#rho");
    MSPlot["NbOfVertices"]               = new MultiSamplePlot(datasets, "NbOfVertices", 40, 0, 40, "Nb. of vertices");
    //Muons
    MSPlot["NbOfIsolatedMuons"]          = new MultiSamplePlot(datasets, "NbOfIsolatedMuons", 5, 0, 5, "Nb. of isolated muons");
    MSPlot["NbOfIsolatedElectrons"]      = new MultiSamplePlot(datasets, "NbOfIsolatedElectrons", 5, 0, 5, "Nb. of isolated electrons");
    MSPlot["NbOfExtraIsolatedMuons"]     = new MultiSamplePlot(datasets, "NbOfExtraIsolatedMuons", 5, 0, 5, "Nb. of isolated muons");
    MSPlot["NbOfExtraIsolatedElectrons"] = new MultiSamplePlot(datasets, "NbOfExtraIsolatedElectrons", 5, 0, 5, "Nb. of isolated electrons");
    MSPlot["MuonRelIsolation"] = new MultiSamplePlot(datasets, "MuonRelIsolation", 50, 0, .25, "RelIso");
    MSPlot["MuonRelIsolation_PreTrig"] = new MultiSamplePlot(datasets, "MuonRelIsolation_PreTrig", 15, 0, .25, "RelIso");
    MSPlot["MuonRelIsolation_PostTrig"] = new MultiSamplePlot(datasets, "MuonRelIsolation_PreTrig", 15, 0, .25, "RelIso");

    MSPlot["MuonPt"]              = new MultiSamplePlot(datasets, "MuonPt", 35, 0, 300, "PT_{#mu}");
    MSPlot["TriggerMuonPt"]              = new MultiSamplePlot(datasets, "MuonPt", 35, 0, 300, "PT_{#mu}");
    MSPlot["MuonPt_PreTrig"]              = new MultiSamplePlot(datasets, "MuonPt_PreTrig", 35, 0, 300, "PT_{#mu}");
    MSPlot["MuonPt_PostTrig"]              = new MultiSamplePlot(datasets, "MuonPt_PostTrig", 35, 0, 300, "PT_{#mu}");
    
    MSPlot["Muond0_PreTrig"]              = new MultiSamplePlot(datasets, "Muond0_PreTrig", 35, 0, .05, "d0_{#mu}");
    MSPlot["Muond0_PostTrig"]              = new MultiSamplePlot(datasets, "Muond0_PostTrig", 35, 0, .05, "d0_{#mu}");
    
    MSPlot["MuonEta"]              = new MultiSamplePlot(datasets, "MuonEta", 25, -2.4, 2.4, "#eta_{#mu}");
    MSPlot["MuonPhi"]              = new MultiSamplePlot(datasets, "MuonPhi", 50, -4, 4, "#phi_{#mu}");
    MSPlot["MuonNValidHits"]              = new MultiSamplePlot(datasets, "MuonNValidHits", 30, 0, 30, "NValidHits_{#mu}");
    MSPlot["Muond0"]              = new MultiSamplePlot(datasets, "Muond0", 50, -0.05, 0.05, "d0_{#mu}");
    MSPlot["MuondZPVz"]              = new MultiSamplePlot(datasets, "MuondZPVz", 50, 0, .5, "dZPVZ_{#mu}");

    MSPlot["MuondRJets"]              = new MultiSamplePlot(datasets, "MuondRJets", 50, 0, 10, "dRJets_{#mu}");
    MSPlot["MuonNMatchedStations"]              = new MultiSamplePlot(datasets, "MuonNMatchedStations", 10, 0, 10, "NMatchedStations_{#mu}");
    MSPlot["MuonDistVzPVz"]              = new MultiSamplePlot(datasets, "MuonDistVzPVz", 50, 0 ,.3, "DistVzPVz_{#mu}");
    MSPlot["MuonDz"]              = new MultiSamplePlot(datasets, "MuonDz", 25, -.6 ,.6, "Dz_{#mu}");

    MSPlot["MuonTrackerLayersWithMeasurement"]    = new MultiSamplePlot(datasets, "MuonTrackerLayersWithMeasurement", 25, 0, 25, "nLayers");
    MSPlot["DiMuon_InvMass"]     = new MultiSamplePlot(datasets, "DiMuon_InvMass", 60, 0, 120, "DiMuon_InvMass");
    MSPlot["NbOfLooseMuon"]     = new MultiSamplePlot(datasets, "NbOfLooseMuon", 10, 0, 10, "Nb. of loose muons");
    
    //B-tagging discriminators
    MSPlot["BdiscBJetCand_CSV"]  = new MultiSamplePlot(datasets, "HighestBdisc_CSV", 75, 0, 1, "CSV b-disc.");
    MSPlot["HighestBdisc_m_ch_CSV"]            = new MultiSamplePlot(datasets, "HighestBdisc_mm_ch_CVS", 100, 0, 1, "CSV b-disc.");
    MSPlot["HighestBdisc_e_ch_CSV"]            = new MultiSamplePlot(datasets, "HighestBdisc_ee_ch_CVS", 100, 0, 1, "CSV b-disc.");
    
    
    MSPlot["bTagRatio13"]            = new MultiSamplePlot(datasets, "bTagRatio13", 25, 0, 1, "bTagRatio13");
    MSPlot["bTagRatio14"]            = new MultiSamplePlot(datasets, "bTagRatio14", 25, 0, 1, "bTagRatio14");

    //Jets
    MSPlot["NbOfSelectedJets"]                  = new MultiSamplePlot(datasets, "NbOfSelectedJets", 15, 0, 15, "Nb. of jets");
    MSPlot["NbOfSelectedLightJets"]                  = new MultiSamplePlot(datasets, "NbOfSelectedLightJets", 10, 0, 10, "Nb. of jets");
    MSPlot["NbOfSelectedBJets"]                  = new MultiSamplePlot(datasets, "NbOfSelectedBJets", 8, 0, 8, "Nb. of jets");
    MSPlot["JetEta"]                  = new MultiSamplePlot(datasets, "JetEta", 30,-3, 3, "Jet #eta");
    MSPlot["JetPhi"]                  = new MultiSamplePlot(datasets, "JetPhi", 50, -4,4 , "Jet #phi");
    
    MSPlot["NbOfBadTrijets"]                  = new MultiSamplePlot(datasets, "NbOfBadTriJets", 150, 0, 150, "Nb. of Bad Combs");
    
   
    
    MSPlot["TriJetMass_Matched"] = new MultiSamplePlot(datasets, "TriJetMassMatched", 100, 0, 1000, "m_{bjj}");
    MSPlot["TriJetMass_UnMatched"] = new MultiSamplePlot(datasets, "TriJetMassUnMatched", 100, 0, 1000, "m_{bjj}");

    
    MSPlot["MVA1TriJetMass"] = new MultiSamplePlot(datasets, "MVA1TriJetMass", 75, 0, 500, "m_{bjj}");
    MSPlot["MVA1DiJetMass"] = new MultiSamplePlot(datasets, "MVA1DiJetMass", 75, 0, 500, "m_{bjj}");
    MSPlot["MVA1PtRat"] = new MultiSamplePlot(datasets, "MVA1PtRat", 25, 0, 2, "P_{t}^{Rat}");
    MSPlot["MVA1BTag"] = new MultiSamplePlot(datasets, "MVA1BTag", 35, 0, 1, "BTag");
    MSPlot["MVA1AnThBh"] = new MultiSamplePlot(datasets, "MVA1AnThBh", 35, 0, 3.14, "AnThBh");
    MSPlot["MVA1AnThWh"] = new MultiSamplePlot(datasets, "MVA1AnThWh", 35, 0, 3.14, "AnThWh");


    MSPlot["MVA2TriJetMass"] = new MultiSamplePlot(datasets, "MVA2TriJetMass", 75, 0, 500, "m_{bjj}");
    MSPlot["MVA2ndPassTriJetMass"] = new MultiSamplePlot(datasets, "MVA2ndPassTriJetMass", 30, 0, 1000, "m_{bjj}");
    
    MSPlot["MVA1TriJetMassMatched"] = new MultiSamplePlot(datasets, "MVA1TriJetMassMatched", 75, 0, 500, "m_{bjj}");

    
    MSPlot["BDisc_Asym"] = new MultiSamplePlot(datasets, "BDisc_Asym", 100, -5, 5, "BDiscAsym");
    MSPlot["RPer"] = new MultiSamplePlot(datasets, "RPer", 100, 0, 18, "Rper");
    MSPlot["HTComb"] = new MultiSamplePlot(datasets, "HTComb", 100, 0, 800, "HTComb");
    MSPlot["WMassComb"] = new MultiSamplePlot(datasets, "WMassComb", 75, 0, 650, "WMassComb");

    MSPlot["WMt"] = new MultiSamplePlot(datasets, "WMt", 50, 0, 250, "W Transverse Mass");
    MSPlot["LepWMass"] = new MultiSamplePlot(datasets, "LepWMass", 50, 0, 200, "MuMET");
    MSPlot["LepWMass_g"] = new MultiSamplePlot(datasets, "LepWMass_g", 50, 0, 200, "MuMET");
    MSPlot["MuMetBMasses"] = new MultiSamplePlot(datasets, "MuMetBMasses", 50, 0, 600, "m_{muMETb}");
    MSPlot["MuMetBMasses_g"] = new MultiSamplePlot(datasets, "MuMetBMasses_g", 50, 0, 1000, "m_{muMETb}");
    MSPlot["MuMetBPt"] = new MultiSamplePlot(datasets, "MuMetBPt", 50, 0, 1000, "Pt_{muMETb}");
    MSPlot["MuMetBPt_g"] = new MultiSamplePlot(datasets, "MuMetBPt_g", 50, 0, 1000, "Pt_{muMETb}"); 
    MSPlot["MuMetBMasses_chi2"] = new MultiSamplePlot(datasets, "MuMetBMasses_chi2", 50, 0, 1000, "\\chi^{2}");
    MSPlot["SelectedJetPt"] = new MultiSamplePlot(datasets, "JetPt", 50, 0, 300, "PT_{jet}");

    MSPlot["SelectedJetPt_light"] = new MultiSamplePlot(datasets, "JetPt_light", 50, 0, 1000, "PT_{lightjet}");
    MSPlot["SelectedJetPt_b"] = new MultiSamplePlot(datasets, "JetPt_b", 50, 0, 1000, "PT_{bjet}");
    MSPlot["HT_SelectedJets"] = new MultiSamplePlot(datasets, "HT_SelectedJets", 50, 0, 1500, "HT");
    
    MSPlot["H"] = new MultiSamplePlot(datasets, "H", 50, 0, 3000, "H");
    MSPlot["HX"] = new MultiSamplePlot(datasets, "HX", 50, 0, 1500, "HX");
    
    MSPlot["HTH"] = new MultiSamplePlot(datasets, "HT/H", 50, 0, 1, "HT/H");
    MSPlot["HTXHX"] = new MultiSamplePlot(datasets, "HTX/HX", 50, 0, 1, "HTX/HX");
    
    
    MSPlot["MassDiff"] = new MultiSamplePlot(datasets, "MassDiff", 50, 0, 200, "MassDiff");
    MSPlot["BalancedMass"] = new MultiSamplePlot(datasets, "BalancedMass", 100, 0, 2000, "BalancedMass");

    
    MSPlot["PTBalTopEventX"] = new MultiSamplePlot(datasets, "PTBal_TopX", 35, 0, 500, "PTBal_TopX");
    MSPlot["PTBalTopSumJetX"] = new MultiSamplePlot(datasets, "PTBal_TopSumJetX", 35, 0, 500, "PTBal_TopSumJetX");
    MSPlot["PTBalTopMuMet"] = new MultiSamplePlot(datasets, "PTBal_TopMuMet", 35, 0, 600, "PTBal_TopMuMet");
    MSPlot["PTBalTopMuMetB"] = new MultiSamplePlot(datasets, "PTBal_TopMuMetB", 35, 0, 600, "PTBal_TopMuMetB");
    MSPlot["deltaMTopMuMet"] = new MultiSamplePlot(datasets, "deltaMTopMuMet", 75, -500, 500, "deltaMTopMuMet");

    MSPlot["deltaMTopMuMetB"] = new MultiSamplePlot(datasets, "deltaMTopMuMetB", 75, -500, 500, "deltaMTopMuMetB");

    MSPlot["HT_CombinationRatio"] = new MultiSamplePlot(datasets, "HT_CombinationRatio", 50, 0, 1, "HT_Ratio");
    MSPlot["MVA"] = new MultiSamplePlot(datasets, "MVA", 15, -0.3, 0.4, "BDT Disciminator");

    //Electrons
    MSPlot["ElectronPt"]              = new MultiSamplePlot(datasets, "ElectronPt", 50, 0, 100, "PT_{e}");
  

    //Plots of UCR variables
    //H sum for Selected Jets
    MSPlot["H_SelectedJets"]                  = new MultiSamplePlot(datasets, "H_SelectedJets", 50, 0, 2000, "H");
   
 
    ///////////////////
    // 1D histograms
    ///////////////////
histo1D["MatchedRecodTop_Mass"] = new TH1F("MatchedRecodTop_Mass","MatchedRecodTop_Mass;MatchedRecodTop_Mass;#events",40,100.,350.);
histo1D["MatchedTop_Mass"] = new TH1F("MatchedTop_Mass","MatchedTop_Mass;MatchedTop_Mass;#events",40,100,350.);
histo1D["MatchedRecodTop_Pt"] = new TH1F("MatchedRecodTop_Pt","MatchedRecodTop_Pt;MatchedRecodTop_Pt;#events",40,0.,700.);
histo1D["MatchedTop_Pt"] = new TH1F("MatchedTop_Pt","MatchedTop_Pt;MatchedTop_Pt;#events",40,0,700.);
histo1D["MatchedRecodTop_Eta"] = new TH1F("MatchedRecodTop_Eta","MatchedRecodTop_Eta;MatchedRecodTop_Eta;#events",10,-3.,-3.);
histo1D["MatchedTop_Eta"] = new TH1F("MatchedTop_Eta","MatchedTop_Eta;MatchedTop_Eta;#events",10,-3.,3.);
histo1D["MatchedRecodTop_Phi"] = new TH1F("MatchedRecodTop_Phi","MatchedRecodTop_Phi;MatchedRecodTop_Phi;#events",10,-3.14,3.14);
histo1D["MatchedTop_Phi"] = new TH1F("MatchedTop_Phi","MatchedTop_Phi;MatchedTop_Phi;#events",10,-3.14,3.14);

histo1D["MatchedRecodTop_NJets"] = new TH1F("MatchedRecodTop_NJets","MatchedRecodTop_NJets;MatchedRecodTop_NJets;#events",16,-0.5,15.5);
histo1D["MatchedTop_NJets"] = new TH1F("MatchedTop_NJets","MatchedTop_NJets;MatchedTop_NJets;#events",16,-0.5, 15.5);

    
    
    
    
    tgraph["e_Mreco_Mass"] = new TGraphAsymmErrors();
    tgraph["e_Mreco_Pt"] = new TGraphAsymmErrors();
    tgraph["e_Mreco_Eta"] = new TGraphAsymmErrors();
    tgraph["e_Mreco_Phi"] = new TGraphAsymmErrors();
    tgraph["e_Mreco_NJets"] = new TGraphAsymmErrors();

    
    
  //Plots
  string pathPNG_MVA = "MVAPlots_"+postfix+channelpostfix;
  string pathPNG = "FourTop"+postfix+channelpostfix;
  pathPNG += "_MSPlots/"; 	
  //pathPNG = pathPNG +"/";
  mkdir(pathPNG.c_str(),0777);
    


/////////////////////////////////
//////// Loop on datasets
/////////////////////////////////

  cout << " - Loop over datasets ... " << datasets.size () << " datasets !" << endl;
    
    
    
  for (unsigned int d = 0; d < datasets.size(); d++) //d < datasets.size()
  {
    if (verbose > 1){
      cout << "   Dataset " << d << " name : " << datasets[d]->Name () << " / title : " << datasets[d]->Title () << endl;
      cout << " - Cross section = " << datasets[d]->Xsection() << endl;
      cout << " - IntLumi = " << datasets[d]->EquivalentLumi() << "  NormFactor = " << datasets[d]->NormFactor() << endl;
      cout << " - Nb of events : " << datasets[d]->NofEvtsToRunOver() << endl;
    }
    //open files and load
    cout<<"Load Dataset"<<endl;
    treeLoader.LoadDataset (datasets[d], anaEnv);
		
    string previousFilename = "";
    int iFile = -1;
    
    string dataSetName = datasets[d]->Name();	
    

//////////////////////////////////////////////////
//      Loop on events
/////////////////////////////////////////////////

    int itrigger = -1, previousRun = -1;
   
    int start = 0;
    unsigned int end = datasets[d]->NofEvtsToRunOver();

    cout <<"Number of events = "<<  end  <<endl;
      
    bool debug = false;
    int event_start;
      
      
    if (verbose > 1) cout << " - Loop over events " << endl;
      
      int nBBBar, nCCBar, nLLBar;
      nBBBar=  nCCBar = nLLBar = 0;
      
      double MHT, MHTSig, STJet, EventMass, EventMassX , SumJetMass, SumJetMassX,H,HX , HT, HTX,HTH,HTXHX, sumpx_X, sumpy_X, sumpz_X, sume_X, sumpx, sumpy, sumpz, sume, jetpt,PTBalTopEventX,PTBalTopSumJetX , PTBalTopMuMet;

      double currentfrac =0.;
      double end_d = end;
      
      // to control waht fraction of the ttjets we run over(remember to alter the int. lumi of the sample)
      if(dataSetName=="TTJets") {
          event_start = 0;
//          event_start = 20000;
          end_d = 200000.;
          //end_d = 700000.;
          cout <<"N ttjets to run over =   "<< end_d <<endl;
          double currentLumi = datasets[d]->EquivalentLumi();
          cout <<"Old lumi =   "<< currentLumi  <<endl;
          //double frac = currentLumi/10.;
          //datasets[d]->SetEquivalentLuminosity(frac);
      }else if(dataSetName=="NP_overlay_TTTT"){
      event_start = 9000;
      end_d = end;
      }else{
          event_start = 0;
          end_d = end;
         // end_d =12000.;
      }
      
      cout <<"Starting event = = "<< event_start  << "Ending event = ="<< end_d  <<endl;

    
      
    for (unsigned int ievt = event_start; ievt < end_d ; ievt++)
    {
        
        
MHT = 0.,MHTSig = 0., STJet = 0., EventMass =0., EventMassX =0., SumJetMass = 0., SumJetMassX=0.  ,H = 0., HX =0., HT = 0., HTX = 0.,HTH=0.,HTXHX=0., sumpx_X = 0., sumpy_X= 0., sumpz_X =0., sume_X= 0. , sumpx =0., sumpy=0., sumpz=0., sume=0., jetpt =0., PTBalTopEventX = 0., PTBalTopSumJetX =0.;
        
        double ievt_d = ievt;
        currentfrac = ievt_d/end_d;
        
	if(ievt%1000 == 0)
		std::cout<<"Processing the "<<ievt<<"th event, time = "<< ((double)clock() - start) / CLOCKS_PER_SEC << " ("<<100*(ievt-start)/(end_d-start)<<"%)"<<flush<<"\r";



	//load event
	event = treeLoader.LoadEvent (ievt, vertex, init_muons, init_electrons, init_jets, mets);

        float rho = event->kt6PFJets_rho();

    
////////////////////////////////////////////////////////////////////////////
/// Splitting TTBar sample into tt +ll, tt+ cc and tt + bb /////////////////
////////////////////////////////////////////////////////////////////////////
        
    //load mcparticles to check jet flavour for ttjets events
    vector<TRootMCParticle*> mcParticles_flav;
    Int_t ttbar_flav = -1;

        double nExB,nExC,nExL;
        nExB = nExC = nExL = 0.;
        
    TRootGenEvent* genEvt_flav = 0;
    genEvt_flav = treeLoader.LoadGenEvent(ievt,false);
    treeLoader.LoadMCEvent(ievt, genEvt_flav, 0, mcParticles_flav,false); 
        
        
        if(  (dataSetName == "TTJets_ll" || dataSetName == "TTJets_cc" || dataSetName == "TTJets_bb" ) )
        {
        for(unsigned int p=0; p<mcParticles_flav.size(); p++) {
          
            if(mcParticles_flav[p]->status()==3 && abs(mcParticles_flav[p]->type())==5 && abs(mcParticles_flav[p]->motherType())!=6) {
               // ttbar_flav=2;
                nExB++;  
            }
            
            else if (mcParticles_flav[p]->status()==3 && abs(mcParticles_flav[p]->type())==4 && abs(mcParticles_flav[p]->motherType())!=6
                && abs(mcParticles_flav[p]->motherType())!=5 && abs(mcParticles_flav[p]->motherType())!=24
                ){
           // ttbar_flav=1;
                 nExC++; 
            }
            
            else if (mcParticles_flav[p]->status()==3 && abs(mcParticles_flav[p]->type())<4 && abs(mcParticles_flav[p]->motherType())!=6){
                // ttbar_flav=1;
                nExL++; 
            }

        }
            
     //   cout <<"TTBar flav composition : " << nExL  <<"  light, " << nExC <<"  C, " << nExB<< "  B" <<  endl;
            
     //   if (ttbar_flav != 1 && ttbar_flav != 2 ) ttbar_flav = 0;
       
            if (nExB >= 2.){
            ttbar_flav =2; 
            nBBBar++ ; //  bbbar
            }
            else if ( nExC >=2.) {
            ttbar_flav =1; 
            nCCBar++ ; //ccbar
            }
            else{
            ttbar_flav =0.; 
                nLLBar++;  //llbar   
            }
            
            if (ttbar_flav ==0 && (dataSetName == "TTJets_cc"  || dataSetName == "TTJets_bb"))  continue;
            if (ttbar_flav ==1 && (dataSetName == "TTJets_ll"  || dataSetName == "TTJets_bb" ))  continue;
            if (ttbar_flav ==2 && (dataSetName == "TTJets_ll"  || dataSetName == "TTJets_cc" ))  continue;
        
        }

//////////////////
//Loading Gen jets
//////////////////

	vector<TRootGenJet*> genjets;
	if( ! (dataSetName == "Data" || dataSetName == "data" || dataSetName == "DATA" ) )
	{

	  // loading GenJets as I need them for JER
	  		genjets = treeLoader.LoadGenJet(ievt);
	}

	// check which file in the dataset it is to have the HLTInfo right
	string currentFilename = datasets[d]->eventTree()->GetFile()->GetName();
                
	if(previousFilename != currentFilename)
	{

		previousFilename = currentFilename;
        	iFile++;
		cout<<"File changed!!! => iFile = "<<iFile<<endl;
	}
        
///////////////////////////////////////////
//  Trigger
///////////////////////////////////////////
	bool trigged = false;
    std::string filterName = "";
	int currentRun = event->runId();
	if(previousRun != currentRun)
	{
	  cout <<"What run? "<< currentRun<<endl;
      		previousRun = currentRun;
		if(Muon)
		{
                // semi-muon
                if(dataSetName.find("Data") == 0 || dataSetName.find("data") == 0 || dataSetName.find("DATA") == 0 )
                {
                    if( event->runId() >= 190456 && event->runId() <= 190738 ){
                        itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v11"), currentRun, iFile);
                        filterName = "hltL3crIsoL1sMu16Eta2p1L1f0L2f16QL3f24QL3crIsoFiltered10";
                    }
                    else if( event->runId() >= 190782 && event->runId() <= 193621){
                        itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v12"), currentRun, iFile);
                        filterName = "hltL3crIsoL1sMu16Eta2p1L1f0L2f16QL3f24QL3crIsoFiltered10";
                    }
                    else if(event->runId() >= 193834  && event->runId() <= 196531 ){
                        itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
                        filterName = "hltL3crIsoL1sMu16Eta2p1L1f0L2f16QL3f24QL3crIsoRhoFiltered0p15";
                    }
                    else if( event->runId() >= 198022  && event->runId() <= 199608){
                        itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v14"), currentRun, iFile);
                        filterName = "hltL3crIsoL1sMu16Eta2p1L1f0L2f16QL3f24QL3crIsoRhoFiltered0p15";
                    }
                    else if( event->runId() >= 199698 && event->runId() <= 209151){
                        itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v15"), currentRun, iFile);
                        filterName = "hltL3crIsoL1sMu16Eta2p1L1f0L2f16QL3f24QL3crIsoRhoFiltered0p15";
                    }
                    else{
                        cout << "Unknown run for SemiMu HLTpath selection: " << event->runId() << endl;
                        filterName = "hltL3crIsoL1sMu16Eta2p1L1f0L2f16QL3f24QL3crIsoRhoFiltered0p15";

                    }
                    if( itrigger == 9999 )
                    {
                        cout << "itriggerSemiMu == 9999 for SemiMu HLTpath selection: " << event->runId() << endl;
                      //  exit(-1);
                    }
                }
    
            else 
	   		{
                  if(dataSetName == "TTJets" || dataSetName == "TTJets_ll" || dataSetName == "TTJets_cc"  || dataSetName == "TTJets_bb" ||  dataSetName == "TTJets_ScaleUp" ||  dataSetName == "TTJets_ScaleDown" ||  dataSetName == "TTJets_MatchingUp"  ||  dataSetName == "TTJets_MatchingDown" ) itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
                
                else  if(dataSetName == "NP_overlay_TTTT") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"),currentRun, iFile);
                
                else  if(dataSetName == "T1TTTT") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"),currentRun, iFile);

      			else if (dataSetName == "W_1Jets") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
                
                else if (dataSetName == "W_2Jets") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
                
                else if (dataSetName == "W_3Jets") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
                
                else if (dataSetName == "W_4Jets") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);

                else if (dataSetName == "ZJets") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);

			    else if (dataSetName == "SingleTop_t_T") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
                else if (dataSetName == "SingleTop_t_TBar") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
			    else if (dataSetName == "SingleTop_s_T") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
                else if (dataSetName == "SingleTop_s_TBar") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
       			else if (dataSetName == "SingleTop_tW_T") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);
       			else if (dataSetName == "SingleTop_tW_TBar") itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile);

				else if (dataSetName == "MultiJet") {
                                                                   
	                 	  itrigger = treeLoader.iTrigger (string ("HLT_IsoMu24_eta2p1_v13"), currentRun, iFile); 
				
							  }
				       
  				if(itrigger == 9999)
				{
    			  		cerr << "NO VALID TRIGGER FOUND FOR THIS EVENT (" << dataSetName << ") IN RUN " << event->runId() << endl;
					//	exit(1);
				}
			}

		} //end if Muon
		else if(Electron)
		{
			if(dataSetName == "Data" || dataSetName == "data" || dataSetName == "DATA")
			{
			
			  if(event->runId() >= 190456 && event->runId() <= 190738)
				 itrigger = treeLoader.iTrigger (string ("HLT_Ele25_CaloIdVT_CaloIsoT_TrkIdT_TrkIsoT_TriCentralPFJet30_v8"), currentRun, iFile);
			  else if ( event->runId() >= 190762 && event->runId() <= 191511 )
			         itrigger = treeLoader.iTrigger (string ("HLT_Ele25_CaloIdVT_CaloIsoT_TrkIdT_TrkIsoT_TriCentralPFJet30_v9"), currentRun, iFile);
		  else if ( event->runId() >= 191512  )
			         itrigger = treeLoader.iTrigger (string ("HLT_Ele25_CaloIdVT_CaloIsoT_TrkIdT_TrkIsoT_TriCentralPFJet30_v10"), currentRun, iFile);
	  if(itrigger == 9999)
				{
    		  cout << "NO VALID TRIGGER FOUND FOR THIS EVENT (DATA) IN RUN " << event->runId() << endl;
		   // exit(1);
 	 			}		
			}
	   		else 
			  {
				if(dataSetName == "TTJets") itrigger = treeLoader.iTrigger (string ("HLT_Ele25_CaloIdVT_CaloIsoT_TrkIdT_TrkIsoT_TriCentralPFJet30_v8"), currentRun, iFile);
				else if (dataSetName == "WJets") itrigger = treeLoader.iTrigger (string ("HLT_Ele25_CaloIdVT_CaloIsoT_TrkIdT_TrkIsoT_TriCentralPFJet30_v8"), currentRun, iFile);
    
  				if(itrigger == 9999)
				{
    			  		cerr << "NO VALID TRIGGER FOUND FOR THIS EVENT (" << dataSetName << ") IN RUN " << event->runId() << endl;
					//	exit(1);
				}
			}
		} //end if Electron
	} //end previousRun != currentRun

        // scale factor for the event
        float scaleFactor = 1.;
        
///////////////////////////////////////////////////////////
//   Event selection
///////////////////////////////////////////////////////////
	       
	// Apply trigger selection
	trigged = treeLoader.EventTrigged (itrigger);
	if (debug)cout<<"triggered? Y/N?  "<< trigged  <<endl;


	//Applying trigger selection again with 2012 Muon+Jets triggers.
	if(!trigged)		   continue;

        
	// Declare selection instance
	Selection selection(init_jets, init_muons, init_electrons, mets);

	// Define object selection cuts
	selection.setJetCuts(30.,2.5,0.01,1.,0.98,0,0);//Pt, Eta, EMF, n90Hits, fHPD, dRJetElectron, DRJets

    selection.setElectronCuts();//	selection.setElectronCuts(30.,2.5,0.1,0.02,0.,999.,0,1); //Pt,Eta,RelIso,d0,MVAId,DistVzPVz, DRJets, MaxMissingHits
	selection.setLooseElectronCuts(20,2.5,0.2,0.);

	//selection.setMuonCuts(26.,2.1,.12,0,0.2,0,1,0.5,5,1 ); //Pt,Eta,RelIso,NValidMuHits,d0, dRJets, NMatchedStations,DistVzPVz,NTrackerLayersWithMeas
    selection.setMuonCuts(26.0,2.1,0.12,0.2,999.,1,0.5,5,0 );
    selection.setLooseMuonCuts(10,2.5,0.2);
	  
	//Select objects 
	//	vector<TRootElectron*> selectedElectrons_NoIso = selection.GetSelectedElectrons(20,2.4,999.,vertex[0]);
	//vector<TRootElectron*> selectedElectrons        = selection.GetSelectedElectrons(vertex[0]);
   
    vector<TRootElectron*> selectedElectrons        = selection.GetSelectedElectrons();
	vector<TRootElectron*> selectedExtraElectrons;
	vector<TRootMuon*>     selectedMuons_NoIso      = selection.GetSelectedMuons(26,2.4,999.); 
	vector<TRootMuon*>     selectedMuons            = selection.GetSelectedMuons(vertex[0]);
	vector<TRootMuon*>     selectedExtraMuons;
	vector<TRootJet*>      selectedJets             = selection.GetSelectedJets(true); // ApplyJetId
    vector<TRootJet*>      selectedJets2ndPass;
    vector<TRootJet*>   MVASelJets1;
    vector<TRootJet*>      selectedSoftJets         = selection.GetSelectedJets(20.,2.5, selectedMuons, 0., true); // ApplyJetId
	vector<TRootMuon*>     selectedLooseMuons       = selection.GetSelectedLooseMuons();
    vector<TRootElectron*> selectedLooseElectrons   = selection.GetSelectedElectrons(); // VBTF ID
    vector<TRootJet*>      selectedBJets; // B-Jets
    vector<TRootJet*>      selectedLightJets; // light-Jets
    vector<TRootJet*>       selectedCSVOrderedJets     = selection.GetSelectedJets(true); //CSV ordered Jet collection added by JH

	//order jets wrt to Pt, then set bool corresponding to RefSel cuts.
    sort(selectedJets.begin(),selectedJets.end(),HighestPt()); //order muons wrt Pt.                                                                    
    sort(selectedCSVOrderedJets.begin(), selectedCSVOrderedJets.end(), HighestCVSBtag()); //order Jets wrt CSVtag

    int JetCut =0;
    int nMu = selectedMuons.size();
    int nEl = selectedElectrons.size();

	// Apply primary vertex selection
	bool isGoodPV = selection.isPVSelected(vertex, 4, 24., 2);
        if(!isGoodPV) continue;

        
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Applying baseline offline event selection here
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //if (debug)	cout <<" applying baseline event selection..."<<endl;
        
    if (!(selectedJets.size() >= 6)) continue;
        
        int nbadcombs = Factorial(selectedJets.size())    /  (  (Factorial(3)) * (Factorial(  selectedJets.size() - 3     ) )       ) ;
        

	//if (debug) cout<<" jet1 pt =  "<<selectedJets[0]->Pt() << "   "<< " jet2 pt =  "<<selectedJets[1]->Pt() << "   "<< " jet2 pt =  "<<selectedJets[2]->Pt() << "   "<< " jet3 pt =  "<<selectedJets[3]->Pt() << "  JetCut?"  << JetCut  <<endl;
         
        bool isTagged =false;
        int seljet;
        
	if(Muon){

  //  if (debug) cout <<"Number of Muons, Jets, BJets, JetCut  ===>  "<< selectedMuons.size() <<"  "  << selectedJets.size()   <<"  " <<  selectedBJets.size()   <<"  "<<JetCut  <<endl;

        double temp_HT = 0.;
        int nTags = 0;
        selectedBJets.clear();
        
        for (Int_t seljet0 =0; seljet0 < selectedJets.size(); seljet0++ ){
            temp_HT += selectedJets[seljet0]->Pt();
            if (selectedJets[seljet0]->btag_combinedSecondaryVertexBJetTags() > workingpointvalue) selectedBJets.push_back(selectedJets[seljet0]);
        }
        
        if (debug)	cout <<" temp HT = "<< temp_HT <<endl;

	  //Apply the selection
	  if  (  !( selectedJets.size() >= 6 && selectedBJets.size() >= 2  &&  nMu == 1 && temp_HT >= 400.)) continue;
        
        if (debug)	cout <<" passed baseline event selection..."<<endl;

        vector<TLorentzVector> selectedMuonTLV;
        selectedMuonTLV.push_back(*selectedMuons[0]);

        vector<TLorentzVector*> selectedMuonTLV_JC;
        selectedMuonTLV_JC.push_back(selectedMuons[0]);
        
        float reliso = (selectedMuons[0]->chargedHadronIso()+selectedMuons[0]->neutralHadronIso()+selectedMuons[0]->photonIso())/selectedMuons[0]->Pt();
        
        vector<TLorentzVector> mcParticlesTLV, selectedJetsTLV, mcMuonsTLV;

        vector<TRootMCParticle*> mcParticlesMatching_;
        bool muPlusFromTop = false, muMinusFromTop = false;
        bool elPlusFromTop = false, elMinusFromTop = false;
        
        pair<unsigned int, unsigned int> leptonicBJet_, hadronicBJet_, hadronicWJet1_, hadronicWJet2_; //First index is the JET number, second one is the parton
        leptonicBJet_ = hadronicBJet_ = hadronicWJet1_ = hadronicWJet2_ = pair<unsigned int,unsigned int>(9999,9999);

        
////////////////////////////////////////////////////////////////////////////////////
//// Getting Gen Event
////////////////////////////////////////////////////////////////////////////////////
        vector<TRootMCParticle*> mcParticles;

         if(dataSetName != "data" && dataSetName != "Data" && dataSetName != "Data"){
        vector<TRootMCParticle*> mcParticles;
        vector<TRootMCParticle*> mcTops;
             
        mcParticlesMatching_.clear();
        mcParticlesTLV.clear();
        selectedJetsTLV.clear();
                     
             
        int leptonPDG, muonPDG = 13, electronPDG = 11;
        leptonPDG = muonPDG;
             
             
        TRootGenEvent* genEvt = 0;
        genEvt = treeLoader.LoadGenEvent(ievt,false);
        sort(selectedJets.begin(),selectedJets.end(),HighestPt()); 
        treeLoader.LoadMCEvent(ievt, genEvt, 0, mcParticles,false);  
    
        if (debug) cout <<"size   "<< mcParticles.size()<<endl;

         
             //Pick out MCParticles of interest
             for (Int_t i=0; i<mcParticles.size(); i++){
                 if( mcParticles[i]->status() != 3) continue;
                 
                 if( (abs(mcParticles[i]->type())< 6||mcParticles[i]->type()==21)&&mcParticles[i]->status() ==3){
                     mcParticlesTLV.push_back(*mcParticles[i]);
                     mcParticlesMatching_.push_back(mcParticles[i]);
                 }
                 
                 // check if there is a mu +/- or a el +/-
                 if( mcParticles[i]->type() == leptonPDG && mcParticles[i]->motherType() == -24 && mcParticles[i]->grannyType() == -6 )
                 {
                     //if(muMinusFromTop) cerr<<"muMinusFromTop was already true"<<endl;
                     if(leptonPDG==muonPDG) muMinusFromTop = true;
                     else if(leptonPDG==electronPDG) elMinusFromTop = true;
                 }
                 if( mcParticles[i]->type() == -leptonPDG && mcParticles[i]->motherType() == 24 && mcParticles[i]->grannyType() == 6 )
                 {
                     //if(muPlusFromTop) cerr<<"muPlusFromTop was already true"<<endl;
                     if(leptonPDG==muonPDG) muPlusFromTop = true;
                     else if(leptonPDG==electronPDG) elPlusFromTop = true;
                 }
                 
             }
         
         }

        

//////////////////////////////////////
////Filling histograms / plotting
//////////////////////////////////////

        MSPlot["NbOfVertices"]->Fill(vertex.size(), datasets[d], true, Luminosity*scaleFactor);
        
/////////////////
////Muons
/////////////////
        
	    MSPlot["NbOfIsolatedMuons"]->Fill(selectedMuons.size(), datasets[d], true, Luminosity*scaleFactor);
	    MSPlot["NbOfIsolatedElectrons"]->Fill(selectedElectrons.size(), datasets[d], true, Luminosity*scaleFactor);

        if (debug) cout <<"filling muid   "<< selectedMuons[0]->Pt()<<endl;
        
	  //Fill Muon ID plots
        double muzPVz = fabs(selectedMuons[0]->vz() - vertex[0]->Z());
        MSPlot["Muond0"]->Fill(selectedMuons[0]->d0(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["MuonDistVzPVz"]->Fill(muzPVz, datasets[d], true, Luminosity*scaleFactor );
        MSPlot["MuonDz"]->Fill( selectedMuons[0]->dz() , datasets[d], true, Luminosity*scaleFactor );
        MSPlot["MuonPt"]->Fill(selectedMuons[0]->Pt(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["MuonEta"]->Fill(selectedMuons[0]->Eta(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["MuonPhi"]->Fill(selectedMuons[0]->Phi(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["MuonNValidHits"]->Fill(selectedMuons[0]->nofValidHits(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["Muond0"]->Fill(selectedMuons[0]->d0(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["MuonDistVzPVz"]->Fill(muzPVz, datasets[d], true, Luminosity*scaleFactor );
        MSPlot["MuonNMatchedStations"]->Fill(selectedMuons[0]->nofMatchedStations(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["MuonTrackerLayersWithMeasurement"]->Fill(selectedMuons[0]->nofTrackerLayersWithMeasurement(), datasets[d], true, Luminosity*scaleFactor);
        MSPlot["MuonRelIsolation"]->Fill(reliso, datasets[d], true, Luminosity*scaleFactor);
        
      if (debug) cout <<"filled all muID plots  .."<<endl;

        if(dataSetName != "data" && dataSetName != "DATA" && dataSetName != "Data") {
            for(unsigned int i=0; i<mcParticles.size(); i++) {
                if( abs(mcParticles[i]->type()) == 13 && mcParticles[i]->status() == 3) { //Matrix Element Level Muon
                    mcMuonsTLV.push_back(*mcParticles[i]);
                }
            }

            JetPartonMatching muonMatching = JetPartonMatching(mcMuonsTLV, selectedMuonTLV, 2, true, true, 0.3);
            
            for(unsigned int i=0; i<mcMuonsTLV.size(); i++) {
                int matchedMuonNumber = muonMatching.getMatchForParton(i, 0);
                if(matchedMuonNumber != -1) MSPlot["LeptonTruth"]->Fill(2, datasets[d], true, Luminosity*scaleFactor);
                else MSPlot["LeptonTruth"]->Fill(1, datasets[d], true, Luminosity*scaleFactor);
            }
        }
        

/////////////
////Jets
/////////////
	  MSPlot["NbOfSelectedJets"]->Fill(selectedJets.size(), datasets[d], true, Luminosity*scaleFactor);
	  MSPlot["NbOfSelectedLightJets"]->Fill(selectedLightJets.size(), datasets[d], true, Luminosity*scaleFactor);
      MSPlot["NbOfSelectedBJets"]->Fill(selectedBJets.size(), datasets[d], true, Luminosity*scaleFactor);
	  //  MSPlot["RhoCorrection"]->Fill(event->kt6PFJetsPF2PAT_rho(), datasets[d], true, Luminosity*scaleFactor);
	  if (debug) cout <<"per jet plots.."<<endl;

	//plots to to inspire staggered Jet Pt selection
  //    if (selectedJets.size()>=4) MSPlot["4thJetPt"]->Fill(selectedJets[3]->Pt(), datasets[d], true, Luminosity*scaleFactor);
 //     if (selectedJets.size()>=5) MSPlot["5thJetPt"]->Fill(selectedJets[4]->Pt(), datasets[d], true, Luminosity*scaleFactor);
 //     if (selectedJets.size()>=6) MSPlot["6thJetPt"]->Fill(selectedJets[5]->Pt(), datasets[d], true, Luminosity*scaleFactor);
//      if (selectedJets.size()>=7) MSPlot["7thJetPt"]->Fill(selectedJets[6]->Pt(), datasets[d], true, Luminosity*scaleFactor);
	
    if (debug) cout <<"got muons and mets"<<endl;
        
/////////////////////////////////
/// Find indices of jets from Tops
////////////////////////////////
            
        for(unsigned int i=0; i<selectedJets.size(); i++) selectedJetsTLV.push_back(*selectedJets[i]);
        JetPartonMatching matching = JetPartonMatching(mcParticlesTLV, selectedJetsTLV, 2, true, true, 0.3);
        
        vector< pair<unsigned int, unsigned int> > JetPartonPair;
        for(unsigned int i=0; i<mcParticlesTLV.size(); i++) //loop through mc particles and find matched jets
        {
            int matchedJetNumber = matching.getMatchForParton(i, 0);
            if(matchedJetNumber != -1)
                JetPartonPair.push_back( pair<unsigned int, unsigned int> (matchedJetNumber, i) );// Jet index, MC Particle index
        }
        
       if (debug) cout <<"n sel jets  "<<selectedJets.size()  << "   n mc particles tlv : "<< mcParticlesTLV.size() << " jet parton pari size :   "<< JetPartonPair.size()<<"  "<< muPlusFromTop<<muMinusFromTop<<endl;
        
        for(unsigned int i=0; i<JetPartonPair.size(); i++)//looping through matched jet-parton pairs
        {
            unsigned int j = JetPartonPair[i].second;	  //get index of matched mc particle
        
            if( fabs(mcParticlesMatching_[j]->type()) < 5 )
            {
                if( ( muPlusFromTop && mcParticlesMatching_[j]->motherType() == -24 && mcParticlesMatching_[j]->grannyType() == -6 )
                   || ( muMinusFromTop && mcParticlesMatching_[j]->motherType() == 24 && mcParticlesMatching_[j]->grannyType() == 6 ) )
                {
                    if(hadronicWJet1_.first == 9999)
                    {
                        hadronicWJet1_ = JetPartonPair[i];
                       // MCPermutation[0] = JetPartonPair[i].first;
                    }
                    else if(hadronicWJet2_.first == 9999)
                    {
                        hadronicWJet2_ = JetPartonPair[i];
                        //MCPermutation[1] = JetPartonPair[i].first;
                    }
                    //else cerr<<"Found a third jet coming from a W boson which comes from a top quark..."<<endl;
                }
            }
            else if( fabs(mcParticlesMatching_[j]->type()) == 5 )
            {
               
                if(  ( muPlusFromTop && mcParticlesMatching_[j]->motherType() == -6) || ( muMinusFromTop && mcParticlesMatching_[j]->motherType() == 6 ) )
                {
                    hadronicBJet_ = JetPartonPair[i];
                    //MCPermutation[2] = JetPartonPair[i].first;
                }
                else if((muPlusFromTop && mcParticlesMatching_[j]->motherType() == 6) ||  ( muMinusFromTop &&mcParticlesMatching_[j]->motherType() == -6) )
                {
                    leptonicBJet_ = JetPartonPair[i];
                    //MCPermutation[3] = JetPartonPair[i].first;
                }
            }
        }
 
  //  cout <<"  "<<endl;
   if (debug) cout <<"Indices of matched jets are :  "<< hadronicBJet_.first<<"  "<< hadronicWJet1_.first  <<" " << hadronicWJet2_.first <<endl;
        
/////////////////////////////////
/// TMVA for mass reconstruction
////////////////////////////////

    jetCombiner->ProcessEvent_SingleHadTop(datasets[d], mcParticles_flav, selectedJets, selectedMuonTLV_JC[0], genEvt_flav, scaleFactor);
if (debug) cout <<"Processing event with jetcombiner :  "<< endl;
      
if(!TrainMVA){
        MVAvals1 = jetCombiner->getMVAValue(MVAmethod, 1); // 1 means the highest MVA value
        selectedJets2ndPass.clear();
       MVASelJets1.clear();

    //make vector of jets excluding thise selected by 1st pass of mass reco
    for (Int_t seljet1 =0; seljet1 < selectedJets.size(); seljet1++ ){
        if (seljet1 == MVAvals1.second[0] || seljet1 == MVAvals1.second[1] || seljet1 == MVAvals1.second[2]){
           MVASelJets1.push_back(selectedJets[seljet1]);
            continue;
       
        }
        selectedJets2ndPass.push_back(selectedJets[seljet1]);
    }
    
            jetCombiner->ProcessEvent_SingleHadTop(datasets[d], mcParticles_flav, selectedJets2ndPass,  selectedMuonTLV_JC[0], genEvt_flav, scaleFactor);
            MVAvals2ndPass = jetCombiner->getMVAValue(MVAmethod, 1);

           // cout <<"sel jets 1st pass = "<< selectedJets.size() << "sel jets 2nd pass =  " << selectedJets2ndPass.size() << endl;
    
    //check data-mc agreement of kin. reco. variables.
    float mindeltaR =100.;
    float mindeltaR_temp =100.;
    int wj1;
    int wj2;
    int bj1;
    
    //define the jets from W as the jet pair with smallest deltaR
    for (int m=0; m<MVASelJets1.size(); m++) {
        for (int n=0; n<MVASelJets1.size(); n++) {
            if(n==m) continue;
            TLorentzVector lj1  = *MVASelJets1[m];
            TLorentzVector lj2  = *MVASelJets1[n];
            mindeltaR_temp  = lj1.DeltaR(lj2);
            if (mindeltaR_temp < mindeltaR){
                mindeltaR = mindeltaR_temp;
                wj1 = m;
                wj2 = n;
            }
        }
    }
    // find the index of the jet not chosen as a W-jet
    for (unsigned int p=0; p<MVASelJets1.size(); p++) {
        if(p!=wj1 && p!=wj2) bj1 = p;
    }
    
    
    //now that putative b and W jets are chosen, calculate the six kin. variables.

    TLorentzVector Wh = *MVASelJets1[wj1]+*MVASelJets1[wj2];
    TLorentzVector Bh = *MVASelJets1[bj1];
    TLorentzVector Th = Wh+Bh;
    
    double TriJetMass = Th.M();

    double DiJetMass = Wh.M();
    //DeltaR
    float AngleThWh = fabs(Th.DeltaPhi(Wh));
    float AngleThBh = fabs(Th.DeltaPhi(Bh));

    float btag = MVASelJets1[bj1]->btag_combinedSecondaryVertexBJetTags();
    
    double PtRat = (( *MVASelJets1[0] + *MVASelJets1[1] + *MVASelJets1[2] ).Pt())/( MVASelJets1[0]->Pt() + MVASelJets1[1]->Pt() + MVASelJets1[2]->Pt() );
    
    
      MSPlot["MVA1TriJetMass"]->Fill(TriJetMass,  datasets[d], true, Luminosity*scaleFactor );
      MSPlot["MVA1DiJetMass"]->Fill(DiJetMass,  datasets[d], true, Luminosity*scaleFactor );
      MSPlot["MVA1BTag"]->Fill(btag,  datasets[d], true, Luminosity*scaleFactor );
      MSPlot["MVA1PtRat"]->Fill(PtRat,  datasets[d], true, Luminosity*scaleFactor );
      MSPlot["MVA1AnThWh"]->Fill(AngleThWh,  datasets[d], true, Luminosity*scaleFactor );
      MSPlot["MVA1AnThBh"]->Fill(AngleThBh,  datasets[d], true, Luminosity*scaleFactor );

    
    
    
         bestTopMass1 =( *selectedJets[MVAvals1.second[0]] + *selectedJets[MVAvals1.second[1]] + *selectedJets[MVAvals1.second[2]]).M();
         bestTopMass2ndPass =( *selectedJets[MVAvals2ndPass.second[0]] + *selectedJets[MVAvals2ndPass.second[1]] + *selectedJets[MVAvals2ndPass.second[2]]).M();

         bestTopPt =( *selectedJets[MVAvals1.second[0]] + *selectedJets[MVAvals1.second[1]] + *selectedJets[MVAvals1.second[2]]).Pt();

       // cout <<"Indices of best MVA jets are :  "<< MVAvals1.second[0] <<"  "<< MVAvals1.second[1]  <<" " << MVAvals1.second[2]<<endl;
            
         //   cout <<"MVA Mass 1 = "<< bestTopMass1 << " MVA Mass 2 = "<< bestTopMass2ndPass << endl;
    
   // cout <<"   "<<endl;
    
        MSPlot["MVA1TriJetMass"]->Fill(bestTopMass1,  datasets[d], true, Luminosity*scaleFactor );
        MSPlot["MVA2ndPassTriJetMass"]->Fill(bestTopMass2ndPass,  datasets[d], true, Luminosity*scaleFactor );

       if (debug)  cout <<"MVA Mass 1 = "<< bestTopMass1 << " MVA Mass 2 = "<< bestTopMass2 << endl;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /////  Calculating how well the MVA jet selection is doing: Fraction of ttbar events            ////
        ////    where the jets selected by the TMVA massReco match the true jets from the hadronic top) ////
        ////////////////////////////////////////////////////////////////////////////////////////////////////
    
    bool top_recod = false;
        if(   ( hadronicBJet_.first == MVAvals1.second[0] || hadronicBJet_.first == MVAvals1.second[1] || hadronicBJet_.first == MVAvals1.second[2]   )  && ( hadronicWJet1_.first == MVAvals1.second[0] || hadronicWJet1_.first == MVAvals1.second[1] || hadronicWJet1_.first == MVAvals1.second[2]   )    && ( hadronicWJet2_.first == MVAvals1.second[0] || hadronicWJet2_.first == MVAvals1.second[1] || hadronicWJet2_.first == MVAvals1.second[2]   )      ){
            nMVASuccesses++;
            top_recod = true;
            if (debug)  cout <<"top is reco'd..."<< endl;

        }
    
    
            if (debug)  cout <<"checked mva success..."<< endl;
            double matchedMass, unmatchedMass, matchedPt, matchedEta, matchedPhi;
            
            // Pick some random jets to investigate wrong combinations...
            int badjet1 =-1 , badjet2 =-1 , badjet3=-1;
            
           while((badjet1==-1)||(badjet2==-1)||(badjet3==-1)) {
                Int_t rand_jet_index  = rand->Integer(selectedJets.size());
                if ((rand_jet_index != hadronicBJet_.first) ||(rand_jet_index != hadronicWJet1_.first ) || (rand_jet_index != hadronicWJet2_.first)){

                    if (badjet1== -1) {
                        badjet1 = rand_jet_index;
                        continue;
                    }
                    if ((badjet2== -1) && (rand_jet_index!= badjet1)) {
                        badjet2 = rand_jet_index;
                        continue;
    
                    }
                    if ((badjet3== -1)  && (rand_jet_index!= badjet1) &&( rand_jet_index!= badjet2)) {
                        badjet3 = rand_jet_index;
                        continue;
   
                    }
                }
            } 
            unmatchedMass =( *selectedJets[badjet1] + *selectedJets[badjet2] + *selectedJets[badjet3]).M();

            MSPlot["TriJetMass_UnMatched"]->Fill(unmatchedMass,  datasets[d], true, Luminosity*scaleFactor );

           // cout <<"Indices of matched jets are :  "<< hadronicBJet_.first<<"  "<< hadronicWJet1_.first  <<" " << hadronicWJet2_.first <<endl;
           // cout <<"Indices of random  jets are :  "<<badjet1 <<"  "<< badjet2  <<" " << badjet3  <<endl;
            //cout<<" "<<endl;
        
        if(   ( hadronicBJet_.first != 9999 )  && ( hadronicWJet1_.first != 9999   )    && ( hadronicWJet2_.first != 9999  )      ){
            nMatchedEvents++;
            if (debug) cout <<"matched..." <<endl;
            
            matchedMass =( *selectedJets[hadronicBJet_.first] + *selectedJets[hadronicWJet1_.first] + *selectedJets[hadronicWJet2_.first]).M();
            matchedPt =( *selectedJets[hadronicBJet_.first] + *selectedJets[hadronicWJet1_.first] + *selectedJets[hadronicWJet2_.first]).Pt();
             matchedPhi =( *selectedJets[hadronicBJet_.first] + *selectedJets[hadronicWJet1_.first] + *selectedJets[hadronicWJet2_.first]).Phi();
             matchedEta =( *selectedJets[hadronicBJet_.first] + *selectedJets[hadronicWJet1_.first] + *selectedJets[hadronicWJet2_.first]).Eta();

            MSPlot["MVA1TriJetMassMatched"]->Fill(bestTopMass1,  datasets[d], true, Luminosity*scaleFactor );
            MSPlot["TriJetMass_Matched"]->Fill(matchedMass,  datasets[d], true, Luminosity*scaleFactor );

            histo1D["MatchedTop_Mass"]->Fill(matchedMass);
            histo1D["MatchedTop_Pt"]->Fill(matchedPt);
            histo1D["MatchedTop_Eta"]->Fill(matchedEta);
            histo1D["MatchedTop_Phi"]->Fill(matchedPhi);
            histo1D["MatchedTop_NJets"]->Fill(selectedJets.size());


            if(top_recod){
            histo1D["MatchedRecodTop_Mass"]->Fill(matchedMass);
            histo1D["MatchedRecodTop_Pt"]->Fill(matchedPt);
            histo1D["MatchedRecodTop_Eta"]->Fill(matchedEta);
            histo1D["MatchedRecodTop_Phi"]->Fill(matchedPhi);
            histo1D["MatchedRecodTop_NJets"]->Fill(selectedJets.size());

            }
            
            
            
            }
        }
        
        TLorentzVector mumet;
        
        MEzCalculator NuPz;
        NuPz.SetMET(*mets[0]);
        NuPz.SetMuon(*selectedMuons[0]);

        
        
        //Form Lep W
        double   mumpx =   selectedMuons[0]->Px() + mets[0]->Px();
        double   mumpy =   selectedMuons[0]->Py() + mets[0]->Py();
        double   mumpz =   selectedMuons[0]->Pz() + NuPz.Calculate();
        double   mumpe =   selectedMuons[0]->E()  + mets[0]->E();
        mumet.SetPx(mumpx);
        mumet.SetPy(mumpy);
        mumet.SetPz(mumpz);
        mumet.SetE(mumpe);
        
        
        TLorentzVector muMETHadTop;
        TLorentzVector muMETB;
        TLorentzVector muMETBHadTop;
        
        TLorentzVector bestMVATop;
        if(!TrainMVA) bestMVATop =( *selectedJets[MVAvals1.second[0]] + *selectedJets[MVAvals1.second[1]] + *selectedJets[MVAvals1.second[2]]);
        
        
        if(!TrainMVA){
        muMETHadTop =( *selectedJets[MVAvals1.second[0]] + *selectedJets[MVAvals1.second[1]] + *selectedJets[MVAvals1.second[2]]  + mumet);
        muMETB =( *selectedJets2ndPass[0]  + mumet);
        muMETBHadTop =( *selectedJets[MVAvals1.second[0]] + *selectedJets[MVAvals1.second[1]] + *selectedJets[MVAvals1.second[2]]  + mumet + *selectedJets2ndPass[0] );
        }
        
        double deltaMTopMuMetB = muMETB.M() - bestMVATop.M() ;

        double deltaMTopMuMet = mumet.M() - bestMVATop.M() ;
        
        
        
    //'Twin peaks' kinematic reconstruction - investigating method of kinematic reconstruction
    // of events to deduce their consistency with the pair-production of heavy objects and subsequent
    //decay to boosted topologies.
        
    
        pair<float, float > pseudo_axis;//eta, phi values corresponding to the axis which divides the event
        // into two hemispheres

        double npoints_phi = 15.;
        double npoints_eta = 15.;
        
        pair<double, double > eta_range(-2.5, 0.);
        pair<double, double > phi_range(-3.14, 0.);
        
        double phi_slice = (fabs(phi_range.first - phi_range.second))/npoints_phi;
        double eta_slice = (fabs(eta_range.first - eta_range.second))/npoints_eta;
        TLorentzVector h1; //hemisphere one
        TLorentzVector h2; //hemisphere two
        
        double mass_diff = 100000.;
        double min_mass_diff = 100000.;
        double balanced_mass = 0.;
        
        
       // for (double i = eta_range.first; i < eta_range.second; i = i + eta_slice) {
            for (double j = phi_range.first; j < phi_range.second; j = j + phi_slice) {
            //now calculate the mass of each hemisphere
                
               // cout<<"looping ..."<< i << "  "  <<  j  <<endl;
                 h1 = (0.,0.,0.,0.); //hemisphere one
                 h2 = (0.,0.,0.,0.); //hemisphere two

                
                double low_phi = phi_range.first + (j*phi_slice);
                double hi_phi = low_phi + 3.14;
               // double low_eta = eta_range.first + (i*eta_slice);
               // double hi_eta = low_eta + 2.5;

                
                for(Int_t seljet2 =0; seljet2 < selectedJets.size(); seljet2++ ){
                    
                    if ( (selectedJets[seljet2]->Phi() > low_phi   ) && (selectedJets[seljet2]->Phi() < hi_phi)  ){  //&& (selectedJets[seljet2]->Eta() > low_eta  )  && (selectedJets[seljet2]->Eta() < hi_eta  ))    {
                        
                        h1 =( h1 + *selectedJets[seljet2]);
                        
                    }else{
                    
                        h2 =( h2 + *selectedJets[seljet2]);

                    }
                    
                
                }//end jet loop
                
                //add mumet
                // if ( (mumet.Phi() > low_phi   ) && (mumet.Phi() < hi_phi)  && (mumet.Eta() > low_eta  )  && (mumet.Eta() < hi_eta  ))    {
                
                 if ( (mumet.Phi() > low_phi   ) && (mumet.Phi() < hi_phi) )    {
                    
                    h1 =( h1 + mumet);
                    
                }else{
                    h2 =( h2 + mumet);
                }
                

                //Now calculate mass balance
                mass_diff = fabs(h1.M() - h2.M());
               // cout<<"masss diff  ..."<<  mass_diff  <<endl;

                
                if(mass_diff < min_mass_diff){

                    min_mass_diff = mass_diff;
                    balanced_mass = (h1.M() + h2.M())/2.;
                }
                
           // }
        }
      //  cout<<"  "<<endl;
      //  cout<<" min masss diff  ..."<<  min_mass_diff  <<endl;
      //  cout<<"  "<<endl;
        
        MSPlot["MassDiff"]->Fill(min_mass_diff, datasets[d], true, Luminosity*scaleFactor);
        MSPlot["BalancedMass"]->Fill(balanced_mass, datasets[d], true, Luminosity*scaleFactor);
        
	}
	else if(Electron){
	  //	MSPlot["1stLeadingElectronRelIsolation"]->Fill(selectedElectrons_NoIso[0]->relativePfIso(), datasets[d], true, Luminosity*scaleFactor);

	}
    
    }//loop on events
    
    if (debug)cout <<"N BBar = = " << nBBBar <<"  N CCBar = = " << nCCBar <<"  N LLBar = =  " << nLLBar << endl;
      
    
    
      //important: free memory
      treeLoader.UnLoadDataset();
      
      double nMVASuccessesd = nMVASuccesses;
      double nMatchedEventsd = nMatchedEvents;
      
      cout <<"Efficiency of MVA jet selection = = "<<  nMVASuccessesd/nMatchedEventsd   <<endl;
      
  } //loop on datasets
    

    
  //Once everything is filled ...
  cout << " We ran over all the data ;-)" << endl;
  
  ///////////////////
  // Writing
  //////////////////
  cout << " - Writing outputs to the files ..." << endl;
    if(histo1D["MatchedRecodTop_Mass"] && histo1D["MatchedTop_Mass"]   ){
        
        tgraph["e_Mreco_Mass"]->BayesDivide(histo1D["MatchedRecodTop_Mass"],histo1D["MatchedTop_Mass"] );
        tgraph["e_Mreco_Pt"]->BayesDivide(histo1D["MatchedRecodTop_Pt"],histo1D["MatchedTop_Pt"] );
        tgraph["e_Mreco_Eta"]->BayesDivide(histo1D["MatchedRecodTop_Eta"],histo1D["MatchedTop_Eta"] );
        tgraph["e_Mreco_Phi"]->BayesDivide(histo1D["MatchedRecodTop_Phi"],histo1D["MatchedTop_Phi"] );
        tgraph["e_Mreco_NJets"]->BayesDivide(histo1D["MatchedRecodTop_NJets"],histo1D["MatchedTop_NJets"] );

    }

    
    fout->cd();
    

    
    TDirectory* tEffdir = fout->mkdir("TGraph");
    tEffdir->cd();
    
    for(map<string,TGraphAsymmErrors*>::const_iterator ig = tgraph.begin(); ig != tgraph.end(); ig++)
    {
        
        TGraphAsymmErrors* test = ig->second ;
        string grname = ig->first;
        string grtitle = "RecoEfficiency_" + grname;
        test->SetTitle(grtitle.c_str());
        
        test->Write(grtitle.c_str());
    }

    
   TFile *foutmva = new TFile ("foutMVA.root","RECREATE");
   
    cout <<" after cd .."<<endl;
    
    //Output ROOT file
    string mvarootFileName ("MVA"+postfix+channelpostfix+".root");
      
    string pathPNGJetCombi = pathPNG + "JetCombination/";
    mkdir(pathPNGJetCombi.c_str(),0777);
    
   
    
    if(TrainMVA)jetCombiner->Write(foutmva, true, pathPNGJetCombi.c_str());
       
  for(map<string,MultiSamplePlot*>::const_iterator it = MSPlot.begin(); it != MSPlot.end(); it++)
    {
        string name = it->first;
        
        MultiSamplePlot *temp = it->second;
        TH1F *tempHisto_data;
        TH1F *tempHisto_TTTT;

        
	temp->Draw_wSysUnc(false ,"ScaleFilesEl", name, true, true, true, true, true,100.,true); // merge TT/QCD/W/Z/ST/
	//Draw(bool addRandomPseudoData = false, string label = string("CMSPlot"), bool mergeTT = false, bool mergeQCD = false, bool mergeW = false, bool mergeZ = false, bool mergeST = false, int scaleNPSignal = 1, bool addRatio = false, bool mergeVV = false, bool mergeTTV = false);
      cout <<" Writing plot:  name = "<< name<<"  path = "<< pathPNG  <<endl;
      temp->Write(fout, name, true, pathPNG, "pdf");
    
  }

    TDirectory* th1dir = fout->mkdir("Histos1D");
    th1dir->cd();
    for(map<std::string,TH1F*>::const_iterator it = histo1D.begin(); it != histo1D.end(); it++)
    {
        
        TH1F *temp = it->second;
        temp->Write();
        //TCanvas* tempCanvas = TCanvasCreator(temp, it->first);
        //tempCanvas->SaveAs( (pathPNG+it->first+".png").c_str() );
    }

    
  delete fout;

  cout << "It took us " << ((double)clock() - start) / CLOCKS_PER_SEC << " to run the program" << endl;
  cout << "********************************************" << endl;
  cout << "           End of the program !!            " << endl;
  cout << "********************************************" << endl;

  return 0;
}

int Factorial(int N = 1)
{
    int fact = 1;
    for( int i=1; i<=N; i++ )
        fact = fact * i;  // OR fact *= i;
    return fact;
}


