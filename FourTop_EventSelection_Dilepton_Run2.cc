//////////////////////////////////////////////////////////////////////////////
////         Analysis code for search for Four Top Production.                  ////
////////////////////////////////////////////////////////////////////////////////////

// ttbar @ NLO 13 TeV:
//all-had ->679 * .46 = 312.34
//semi-lep ->679 *.45 = 305.55
//di-lep-> 679* .09 = 61.11

//ttbar @ NNLO 8 TeV:
//all-had -> 245.8 * .46 = 113.068
//semi-lep-> 245.8 * .45 = 110.61
//di-lep ->  245.8 * .09 = 22.122

#define _USE_MATH_DEFINES
#include "TStyle.h"
#include "TPaveText.h"
#include "TTree.h"
#include "TNtuple.h"
#include <TMatrixDSym.h>
#include <TMatrixDSymEigen.h>
#include <TVectorD.h>
#include <ctime>

#include <cmath>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include "TRandom3.h"
#include "TRandom.h"
#include "TProfile.h"
#include <iostream>
#include <map>
#include <cstdlib>

//user code
#include "TopTreeProducer/interface/TRootRun.h"
#include "TopTreeProducer/interface/TRootEvent.h"
#include "TopTreeAnalysisBase/Selection/interface/SelectionTable.h"
#include "TopTreeAnalysisBase/Selection/interface/Run2Selection.h"
//#include "TopTreeAnalysisBase/Selection/interface/FourTopSelectionTable.h"

#include "TopTreeAnalysisBase/Content/interface/AnalysisEnvironment.h"
#include "TopTreeAnalysisBase/Content/interface/Dataset.h"
#include "TopTreeAnalysisBase/Tools/interface/JetTools.h"
#include "TopTreeAnalysisBase/Tools/interface/PlottingTools.h"
#include "TopTreeAnalysisBase/Tools/interface/MultiSamplePlot.h"
#include "TopTreeAnalysisBase/Tools/interface/TTreeLoader.h"
#include "TopTreeAnalysisBase/Tools/interface/AnalysisEnvironmentLoader.h"
#include "TopTreeAnalysisBase/Reconstruction/interface/JetCorrectorParameters.h"
#include "TopTreeAnalysisBase/Reconstruction/interface/JetCorrectionUncertainty.h"
#include "TopTreeAnalysisBase/Reconstruction/interface/MakeBinning.h"
#include "TopTreeAnalysisBase/MCInformation/interface/LumiReWeighting.h"
#include "TopTreeAnalysisBase/MCInformation/interface/JetPartonMatching.h"
#include "TopTreeAnalysisBase/Reconstruction/interface/MEzCalculator.h"
#include "TopTreeAnalysisBase/Tools/interface/LeptonTools.h"

#include "TopTreeAnalysisBase/Reconstruction/interface/TTreeObservables.h"

//This header file is taken directly from the BTV wiki. It contains
// to correctly apply an event level Btag SF. It is not yet on CVS
// as I hope to merge the functionality into BTagWeigtTools.h
//#include "TopTreeAnalysisBase/Tools/interface/BTagSFUtil.h"
#include "TopTreeAnalysisBase/Tools/interface/BTagWeightTools.h"


#include "TopTreeAnalysisBase/Tools/interface/JetCombiner.h"
#include "TopTreeAnalysisBase/Tools/interface/MVATrainer.h"
#include "TopTreeAnalysisBase/Tools/interface/MVAComputer.h"
#include "TopTreeAnalysisBase/Tools/interface/JetTools.h"

using namespace std;
using namespace TopTree;
using namespace reweight;

bool split_ttbar = false;
bool debug = false;
float topness;

pair<float, vector<unsigned int> > MVAvals1;
pair<float, vector<unsigned int> > MVAvals2;
pair<float, vector<unsigned int> > MVAvals2ndPass;
pair<float, vector<unsigned int> > MVAvals3rdPass;

int nMVASuccesses=0;
int nMatchedEvents=0;

/// Normal Plots (TH1F* and TH2F*)
map<string,TH1F*> histo1D;
map<string,TH2F*> histo2D;
map<string,TProfile*> histoProfile;

/// MultiSamplePlot
map<string,MultiSamplePlot*> MSPlot;

/// MultiPadPlot
map<string,MultiSamplePlot*> MultiPadPlot;

struct HighestTCHEBtag
{
    bool operator()( TRootJet* j1, TRootJet* j2 ) const
    {
        return j1->btag_trackCountingHighEffBJetTags() > j2->btag_trackCountingHighEffBJetTags();
    }
};
struct HighestCVSBtag
{
    bool operator()( TRootJet* j1, TRootJet* j2 ) const
    {
        return j1->btag_combinedInclusiveSecondaryVertexV2BJetTags() > j2->btag_combinedInclusiveSecondaryVertexV2BJetTags();
    }
};

bool match;

//To cout the Px, Py, Pz, E and Pt of objects
int Factorial(int N);
float Sphericity(vector<TLorentzVector> parts );
float Centrality(vector<TLorentzVector> parts);
float ElectronRelIso(TRootElectron* el, float rho);

int main (int argc, char *argv[])
{

    //Checking Passed Arguments to ensure proper execution of MACRO
    if(argc < 14)
    {
        std::cerr << "TOO FEW INPUTs FROM XMLFILE.  CHECK XML INPUT FROM SCRIPT.  " << argc << " ARGUMENTS HAVE BEEN PASSED." << std::endl;
        return 1;
    }

    //Placing arguments in properly typed variables for Dataset creation

    const string dName              = argv[1];
    const string dTitle             = argv[2];
    const int color                 = strtol(argv[4], NULL, 10);
    const int ls                    = strtol(argv[5], NULL, 10);
    const int lw                    = strtol(argv[6], NULL, 10);
    const float normf               = strtod(argv[7], NULL);
    const float EqLumi              = strtod(argv[8], NULL);
    const float xSect               = strtod(argv[9], NULL);
    const float PreselEff           = strtod(argv[10], NULL);
    string fileName                 = argv[11];
    const int startEvent            = strtol(argv[argc-2], NULL, 10);
    const int endEvent              = strtol(argv[argc-1], NULL, 10);
    vector<string> vecfileNames;
    for(int args = 11; args < argc-2; args++)
    {
        vecfileNames.push_back(argv[args]);
    }




    cout << "---Dataset accepted from command line---" << endl;
    cout << "Dataset Name: " << dName << endl;
    cout << "Dataset Title: " << dTitle << endl;
    cout << "Dataset color: " << color << endl;
    cout << "Dataset ls: " << ls << endl;
    cout << "Dataset lw: " << lw << endl;
    cout << "Dataset normf: " << normf << endl;
    cout << "Dataset EqLumi: " << EqLumi << endl;
    cout << "Dataset xSect: " << xSect << endl;
    cout << "Dataset File Name: " << vecfileNames[0] << endl;
    cout << "Beginning Event: " << startEvent << endl;
    cout << "Ending Event: " << endEvent << endl;
    cout << "----------------------------------------" << endl;
//    cin.get();



    ofstream eventlist;
    eventlist.open ("interesting_events_mu.txt");

    int passed = 0;
    int ndefs =0;
    int negWeights = 0;
    float weightCount = 0.0;
    int eventCount = 0;

    string btagger = "CSVL";
    float scalefactorbtageff, mistagfactor;
    float workingpointvalue = 0.679; //working points updated to 2012 BTV-POG recommendations.
    bool bx25 = false;

    if(btagger == "CSVL")
        workingpointvalue = .244;
    else if(btagger == "CSVM")
        workingpointvalue = .679;
    else if(btagger == "CSVT")
        workingpointvalue = .898;

    clock_t start = clock();

    //BTagWeightTools * bTool = new BTagWeightTools("SFb-pt_NOttbar_payload_EPS13.txt", "CSVM") ;

    int doJESShift = 0; // 0: off 1: minus 2: plus
    cout << "doJESShift: " << doJESShift << endl;

    int doJERShift = 0; // 0: off (except nominal scalefactor for jer) 1: minus 2: plus
    cout << "doJERShift: " << doJERShift << endl;

    int dobTagEffShift = 0; //0: off (except nominal scalefactor for btag eff) 1: minus 2: plus
    cout << "dobTagEffShift: " << dobTagEffShift << endl;

    int domisTagEffShift = 0; //0: off (except nominal scalefactor for mistag eff) 1: minus 2: plus
    cout << "domisTagEffShift: " << domisTagEffShift << endl;

    cout << "*************************************************************" << endl;
    cout << " Beginning of the program for the FourTop search ! "           << endl;
    cout << "*************************************************************" << endl;


    string postfix = "_Run2_TopTree_Study_" + dName; // to relabel the names of the output file

    if (doJESShift == 1)
        postfix= postfix+"_JESMinus";
    if (doJESShift == 2)
        postfix= postfix+"_JESPlus";
    if (doJERShift == 1)
        postfix= postfix+"_JERMinus";
    if (doJERShift == 2)
        postfix= postfix+"_JERPlus";
    if (dobTagEffShift == -1)
        postfix= postfix+"_bTagMinus";
    if (dobTagEffShift == 1)
        postfix= postfix+"_bTagPlus";
    if(domisTagEffShift == -1)
        postfix= postfix+"_misTagMinus";
    if(domisTagEffShift == 1)
        postfix= postfix+"_misTagPlus";

    ///////////////////////////////////////
    // Configuration
    ///////////////////////////////////////

    string channelpostfix = "";
    string xmlFileName = "";

    //Setting Lepton Channels (Setting both flags true will select Muon-Electron Channel when dilepton is also true)
    bool dilepton = true;
    bool Muon = true;
    bool Electron = true;

    if(dName.find("MuElSkim") != std::string::npos)
    {
        Muon = true;
        Electron = true;
    }
    else if(dName.find("MuMuSkim") != std::string::npos)
    {
        Muon = true;
        Electron = false;
    }
    else if(dName.find("ElElSkim") != std::string::npos)
    {
        Muon = false;
        Electron = true;
    }
    else cout << "Boolean setting by name failed" << endl;

    if(Muon && Electron && dilepton)
    {
        cout << " --> Using the Muon-Electron channel..." << endl;
        channelpostfix = "_MuEl";
        xmlFileName = "config/Run2_Samples.xml";
    }
    else if(Muon && !Electron && dilepton)
    {
        cout << " --> Using the Muon-Muon channel..." << endl;
        channelpostfix = "_MuMu";
        xmlFileName = "config/Run2_Samples.xml";
    }
    else if(!Muon && Electron && dilepton)
    {
        cout << " --> Using the Electron-Electron channel..." << endl;
        channelpostfix = "_ElEl";
        xmlFileName = "config/Run2_Samples.xml";
    }
    else
    {
        cerr<<"Correct Di-lepton Channel not selected."<<endl;
        exit(1);
    }

    bool TrainMVA = false; // If false, the previously trained MVA will be used to calculate stuff
    bool trainEventMVA = false; // If false, the previously trained MVA will be used to calculate stuff
    bool computeEventMVA = false;


    const char *xmlfile = xmlFileName.c_str();
    cout << "used config file: " << xmlfile << endl;

    /////////////////////////////
    //  Set up AnalysisEnvironment
    /////////////////////////////

    AnalysisEnvironment anaEnv;
    cout<<" - Creating environment ..."<<endl;
//    AnalysisEnvironmentLoader anaLoad(anaEnv,xmlfile);
    anaEnv.PrimaryVertexCollection = "PrimaryVertex";
    anaEnv.JetCollection = "PFJets_slimmedJets";
    anaEnv.FatJetCollection = "FatJets_slimmedJetsAK8";
    anaEnv.METCollection = "PFMET_slimmedMETs";
    anaEnv.MuonCollection = "Muons_slimmedMuons";
    anaEnv.ElectronCollection = "Electrons_slimmedElectrons";
    anaEnv.GenJetCollection   = "GenJets_slimmedGenJets";
    anaEnv.TrackMETCollection = "";
    anaEnv.GenEventCollection = "GenEvent";
    anaEnv.NPGenEventCollection = "NPGenEvent";
    anaEnv.MCParticlesCollection = "MCParticles";
    anaEnv.loadFatJetCollection = true;
    anaEnv.loadGenJetCollection = false;
    anaEnv.loadGenEventCollection = false;
    anaEnv.loadNPGenEventCollection = false;
    anaEnv.loadMCParticles = true;
    anaEnv.loadTrackMETCollection = false;
    anaEnv.JetType = 2;
    anaEnv.METType = 2;
    int verbose = 2;//anaEnv.Verbose;



    ////////////////////////////////
    //  Load datasets
    ////////////////////////////////

    TTreeLoader treeLoader;
    vector < Dataset* > datasets;
    cout << " - Creating Dataset ..." << endl;
    Dataset* theDataset = new Dataset(dName, dTitle, true, color, ls, lw, normf, xSect, vecfileNames);
    theDataset->SetEquivalentLuminosity(EqLumi*normf);
    datasets.push_back(theDataset);
    float Luminosity = 2000; //pb^-1??
    vector<string> MVAvars;

    MVAvars.push_back("topness");
    MVAvars.push_back("muonpt");
    MVAvars.push_back("muoneta");
    MVAvars.push_back("HTH");
    MVAvars.push_back("HTRat");
    MVAvars.push_back("HTb");
    MVAvars.push_back("nLtags");
    MVAvars.push_back("nMtags");
    MVAvars.push_back("nTtags");
    MVAvars.push_back("nJets");
    MVAvars.push_back("Jet3Pt");
    MVAvars.push_back("Jet4Pt");
    MVAvars.push_back("HT2M");
    MVAvars.push_back("EventSph");
//    MVAvars.push_back("EventCen");
    MVAvars.push_back("DiLepSph");
//    MVAvars.push_back("DiLepCen");
//    MVAvars.push_back("TopDiLepSph");
//    MVAvars.push_back("TopDiLepCen");

    MVAComputer* Eventcomputer_;

    if(dilepton && Muon && Electron)
    {
        Eventcomputer_ = new MVAComputer("BDT","MVA/MasterMVA_MuEl_13thJuly.root","MasterMVA_MuEl_13thJuly",MVAvars, "_MuElJuly13th2015");
    }
    else if(dilepton && Muon && !Electron)
    {
//        Eventcomputer_ = new MVAComputer("BDT","MVA/MasterMVA_MuMu_9thJuly.root","MasterMVA_MuMu_9thJuly",MVAvars, "_MuMuJuly9th2015");
        Eventcomputer_ = new MVAComputer("BDT","MVA/MasterMVA_MuEl_13thJuly.root","MasterMVA_MuEl_13thJuly",MVAvars, "_MuElJuly13th2015");

    }
    else if(dilepton && !Muon && Electron)
    {
//        Eventcomputer_ = new MVAComputer("BDT","MVA/MasterMVA_ElEl_9thJuly.root","MasterMVA_ElEl_9thJuly",MVAvars, "_ElElJuly9th2015");
        Eventcomputer_ = new MVAComputer("BDT","MVA/MasterMVA_MuEl_13thJuly.root","MasterMVA_MuEl_13thJuly",MVAvars, "_MuElJuly13th2015");

    }

    cout << " Initialized Eventcomputer_" << endl;



    string dataSetName;

    string MVAmethod = "BDT"; // MVAmethod to be used to get the good jet combi calculation (not for training! this is chosen in the jetcombiner class)

    cout <<"Instantiating jet combiner..."<<endl;

    JetCombiner* jetCombiner = new JetCombiner(TrainMVA, Luminosity, datasets, MVAmethod, false);
    cout <<"Instantiated jet combiner..."<<endl;


    /////////////////////////////////
    //  Loop over Datasets
    /////////////////////////////////

    cout <<"found sample with equivalent lumi "<<  theDataset->EquivalentLumi() <<endl;
    dataSetName = theDataset->Name();
    if(dataSetName.find("Data")<=0 || dataSetName.find("data")<=0 || dataSetName.find("DATA")<=0)
    {
        Luminosity = theDataset->EquivalentLumi();
        cout <<"found DATA sample with equivalent lumi "<<  theDataset->EquivalentLumi() <<endl;
    }

    cout << "Rescaling to an integrated luminosity of "<< Luminosity <<" pb^-1" << endl;
    int ndatasets = datasets.size() - 1 ;

    double currentLumi;
    double newlumi;

    //Output ROOT file
    string outputDirectory("MACRO_Output"+channelpostfix);
    mkdir(outputDirectory.c_str(),0777);
    string rootFileName (outputDirectory+"/FourTop"+postfix+channelpostfix+".root");
    TFile *fout = new TFile (rootFileName.c_str(), "RECREATE");

    //vector of objects
    cout << " - Variable declaration ..." << endl;
    vector < TRootVertex* >   vertex;
    vector < TRootMuon* >     init_muons;
    vector < TRootElectron* > init_electrons;
    vector < TRootJet* >      init_jets;
    vector < TRootJet* >      init_fatjets;
    vector < TRootMET* >      mets;

    //Global variable
    TRootEvent* event = 0;

    ////////////////////////////////////////////////////////////////////
    ////////////////// MultiSample plots  //////////////////////////////
    ////////////////////////////////////////////////////////////////////

    MSPlot["NbOfVertices"]                                  = new MultiSamplePlot(datasets, "NbOfVertices", 60, 0, 60, "Nb. of vertices");
    //Muons
    MSPlot["MuonPt"]                                        = new MultiSamplePlot(datasets, "MuonPt", 30, 0, 300, "PT_{#mu}");
    MSPlot["MuonEta"]                                       = new MultiSamplePlot(datasets, "MuonEta", 40,-4, 4, "Muon #eta");
    MSPlot["MuonRelIsolation"]                              = new MultiSamplePlot(datasets, "MuonRelIsolation", 10, 0, .25, "RelIso");
    MSPlot["InitMuonCutFlow"]                               = new MultiSamplePlot(datasets, "InitMuonCutFlow", 12, 0, 12, "CutNumber");
    //Electrons
    MSPlot["ElectronRelIsolation"]                          = new MultiSamplePlot(datasets, "ElectronRelIsolation", 10, 0, .25, "RelIso");
    MSPlot["ElectronPt"]                                    = new MultiSamplePlot(datasets, "ElectronPt", 30, 0, 300, "PT_{e}");
    MSPlot["ElectronEta"]                                   = new MultiSamplePlot(datasets, "ElectronEta", 40,-4, 4, "Jet #eta");
    MSPlot["NbOfElectronsPreSel"]                           = new MultiSamplePlot(datasets, "NbOfElectronsPreSel", 10, 0, 10, "Nb. of electrons");
    //Init Electron Plots
    MSPlot["InitElectronPt"]                                = new MultiSamplePlot(datasets, "InitElectronPt", 30, 0, 300, "PT_{e}");
    MSPlot["InitElectronEta"]                               = new MultiSamplePlot(datasets, "InitElectronEta", 40, -4, 4, "#eta");
    MSPlot["NbOfElectronsInit"]                             = new MultiSamplePlot(datasets, "NbOfElectronsInit", 10, 0, 10, "Nb. of electrons");
    MSPlot["InitElectronRelIsolation"]                      = new MultiSamplePlot(datasets, "InitElectronRelIsolation", 10, 0, .25, "RelIso");
    MSPlot["InitElectronSuperClusterEta"]                   = new MultiSamplePlot(datasets, "InitElectronSuperClusterEta", 10, 0, 2.5, "#eta");
    MSPlot["InitElectrondEtaI"]                             = new MultiSamplePlot(datasets, "InitElectrondEtaI", 20, 0, .05, "#eta");
    MSPlot["InitElectrondPhiI"]                             = new MultiSamplePlot(datasets, "InitElectrondPhiI", 20, 0, .2, "#phi");
    MSPlot["InitElectronHoverE"]                            = new MultiSamplePlot(datasets, "InitElectronHoverE", 10, 0, .15, "H/E");
    MSPlot["InitElectrond0"]                                = new MultiSamplePlot(datasets, "InitElectrond0", 20, 0, .1, "d0");
    MSPlot["InitElectrondZ"]                                = new MultiSamplePlot(datasets, "InitElectrondZ", 10, 0, .25, "dZ");
    MSPlot["InitElectronEminusP"]                           = new MultiSamplePlot(datasets, "InitElectronEminusP", 10, 0, .25, "1/GeV");
    MSPlot["InitElectronConversion"]                        = new MultiSamplePlot(datasets, "InitElectronConversion", 2, 0, 2, "Conversion Pass");
    MSPlot["InitElectronMissingHits"]                       = new MultiSamplePlot(datasets, "InitElectronMissingHits", 10, 0, 10, "MissingHits");
    MSPlot["InitElectronCutFlow"]                           = new MultiSamplePlot(datasets, "InitElectronCutFlow", 12, 0, 12, "CutNumber");
    MSPlot["InitElectronDiagRelIso"]                        = new MultiSamplePlot(datasets, "InitElectronDiagRelIso", 100, 0, 1, "RelIso");
    MSPlot["InitElectronDiagChIso"]                         = new MultiSamplePlot(datasets, "InitElectronDiagChIso", 100, 0, 1, "RelIso");
    MSPlot["InitElectronDiagNIso"]                          = new MultiSamplePlot(datasets, "InitElectronDiagNIso", 100, 0, 1, "RelIso");
    MSPlot["InitElectronDiagPhIso"]                         = new MultiSamplePlot(datasets, "InitElectronDiagPhIso", 100, 0, 1, "RelIso");
    MSPlot["InitElectronDiagPUChIso"]                       = new MultiSamplePlot(datasets, "InitElectronDiagPUChIso", 100, 0, 1, "RelIso");

    //B-tagging discriminators
    MSPlot["BdiscBJetCand_CSV"]                             = new MultiSamplePlot(datasets, "BdiscBJetCand_CSV", 20, 0, 1, "CSV b-disc.");
    //Jets
    MSPlot["JetEta"]                                        = new MultiSamplePlot(datasets, "JetEta", 40,-4, 4, "Jet #eta");
    MSPlot["3rdJetPt"]                                      = new MultiSamplePlot(datasets, "3rdJetPt", 30, 0, 300, "PT_{jet3}");
    MSPlot["4thJetPt"]                                      = new MultiSamplePlot(datasets, "4thJetPt", 30, 0, 300, "PT_{jet4}");
    MSPlot["5thJetPt"]                                      = new MultiSamplePlot(datasets, "5thJetPt", 30, 0, 300, "PT_{jet5}");
    MSPlot["6thJetPt"]                                      = new MultiSamplePlot(datasets, "6thJetPt", 30, 0, 300, "PT_{jet6}");
    MSPlot["HT_SelectedJets"]                               = new MultiSamplePlot(datasets, "HT_SelectedJets", 30, 0, 1500, "HT");
    MSPlot["HTExcess2M"]                                    = new MultiSamplePlot(datasets, "HTExcess2M", 30, 0, 1500, "HT_{Excess 2 M b-tags}");
    MSPlot["HTH"]                                           = new MultiSamplePlot(datasets, "HTH", 20, 0, 1, "HTH");
    //MET
    MSPlot["MET"]                                           = new MultiSamplePlot(datasets, "MET", 70, 0, 700, "MET");
    MSPlot["METCutAcc"]                                     = new MultiSamplePlot(datasets, "METCutAcc", 30, 0, 150, "MET cut pre-jet selection");
    MSPlot["METCutRej"]                                     = new MultiSamplePlot(datasets, "METCutRej", 30, 0, 150, "MET cut pre-jet selection");
    //Topology Plots
    MSPlot["TotalSphericity"]                               = new MultiSamplePlot(datasets, "TotalSphericity", 20, 0, 1, "Sphericity_{all}");
    MSPlot["TotalCentrality"]                               = new MultiSamplePlot(datasets, "TotalCentrality", 20, 0, 1, "Centrality_{all}");
    MSPlot["DiLepSphericity"]                               = new MultiSamplePlot(datasets, "DiLepSphericity", 20, 0, 1, "Sphericity_{ll}");
    MSPlot["DiLepCentrality"]                               = new MultiSamplePlot(datasets, "DiLepCentrality", 20, 0, 1, "Centrality_{ll}");
    MSPlot["TopDiLepSphericity"]                            = new MultiSamplePlot(datasets, "TopDiLepSphericity", 20, 0, 1, "Sphericity_{tll}");
    MSPlot["TopDiLepCentrality"]                            = new MultiSamplePlot(datasets, "TopDiLepCentrality", 20, 0, 1, "Centrality_{tll}");
    //MVA Top Roconstruction Plots
    MSPlot["MVA1TriJet"]                                    = new MultiSamplePlot(datasets, "MVA1TriJet", 30, -1.0, 0.2, "MVA1TriJet");
    MSPlot["MVA1TriJetMass"]                                = new MultiSamplePlot(datasets, "MVA1TriJetMass", 75, 0, 500, "m_{bjj}");
    MSPlot["MVA1DiJetMass"]                                 = new MultiSamplePlot(datasets, "MVA1DiJetMass", 75, 0, 500, "m_{bjj}");
    MSPlot["MVA1PtRat"]                                     = new MultiSamplePlot(datasets, "MVA1PtRat", 25, 0, 2, "P_{t}^{Rat}");
    MSPlot["MVA1BTag"]                                      = new MultiSamplePlot(datasets, "MVA1BTag", 35, 0, 1, "BTag");
    MSPlot["MVA1AnThBh"]                                    = new MultiSamplePlot(datasets, "MVA1AnThBh", 35, 0, 3.14, "AnThBh");
    MSPlot["MVA1AnThWh"]                                    = new MultiSamplePlot(datasets, "MVA1AnThWh", 35, 0, 3.14, "AnThWh");
    MSPlot["MVA1dPhiThDiLep"]                               = new MultiSamplePlot(datasets, "MVA1dPhiThDiLep", 35, 0, 3.14, "dPhiThDiLep");
    MSPlot["MVA1dRThDiLep"]                                 = new MultiSamplePlot(datasets, "MVA1dRThDiLep", 35, 0, 3.14, "dRThDiLep");

    //ZMass window plots
    MSPlot["ZMassWindowWidthAcc"]                           = new MultiSamplePlot(datasets, "ZMassWindowWidthAcc", 20, 0, 100, "Z mass window width");
    MSPlot["ZMassWindowWidthRej"]                           = new MultiSamplePlot(datasets, "ZMassWindowWidthRej", 20, 0, 100, "Z mass window width");
    MSPlot["DiLepMass"]                                     = new MultiSamplePlot(datasets, "DiLepMass", 30, 0, 150, "m_{ll}");

    //n-1 Cut Plots
    MSPlot["NMinusOne"]                                     = new MultiSamplePlot(datasets, "NMinusOne", 8, 0, 8, "CutNumber");

    ///////////////////
    // 1D histograms //
    ///////////////////

    ///////////////////
    // 2D histograms //
    ///////////////////
    histo2D["HTLepSep"] = new TH2F("HTLepSep","dR_{ll}:HT",50,0,1000, 20, 0,4);

    //Plots
    string pathPNG = "MSPlots_FourTop"+postfix+channelpostfix;
    pathPNG += "_MSPlots/";
    //pathPNG = pathPNG +"/";
    mkdir(pathPNG.c_str(),0777);

    cout <<"Making directory :"<< pathPNG  <<endl;
    vector<string> CutsselecTable;
    if(dilepton)
    {
        /////////////////////////////////
        // Selection table: Dilepton + jets
        /////////////////////////////////
        if(Muon && Electron)
        {
            CutsselecTable.push_back(string("initial"));
            CutsselecTable.push_back(string("Event cleaning and Trigger"));
            CutsselecTable.push_back(string("Exactly 1 Loose Isolated Muon"));
            CutsselecTable.push_back(string("Exactly 1 Loose Electron"));
            CutsselecTable.push_back(string("At least 4 Jets"));
            CutsselecTable.push_back(string("At least 1 CSVM Jet"));
            CutsselecTable.push_back(string("At least 2 CSVM Jets"));
            CutsselecTable.push_back(string("At Least 500 GeV HT"));

        }
        if(Muon && !Electron)
        {
            CutsselecTable.push_back(string("initial"));
            CutsselecTable.push_back(string("Event cleaning and Trigger"));
            CutsselecTable.push_back(string("Exactly 2 Loose Isolated Muon"));
            CutsselecTable.push_back(string("Z Mass Veto"));
            CutsselecTable.push_back(string("At least 4 Jets"));
            CutsselecTable.push_back(string("At least 1 CSVM Jet"));
            CutsselecTable.push_back(string("At least 2 CSVM Jets"));
            CutsselecTable.push_back(string("At Least 500 GeV HT"));
        }
        if(!Muon && Electron)
        {
            CutsselecTable.push_back(string("initial"));
            CutsselecTable.push_back(string("Event cleaning and Trigger"));
            CutsselecTable.push_back(string("Exactly 2 Loose Electron"));
            CutsselecTable.push_back(string("Z Mass Veto"));
            CutsselecTable.push_back(string("At least 4 Jets"));
            CutsselecTable.push_back(string("At least 1 CSVM Jet"));
            CutsselecTable.push_back(string("At least 2 CSVM Jets"));
            CutsselecTable.push_back(string("At Least 500 GeV HT"));
        }
    }


    SelectionTable selecTable(CutsselecTable, datasets);
    selecTable.SetLuminosity(Luminosity);
    selecTable.SetPrecision(1);

    /////////////////////////////////
    // Loop on datasets
    /////////////////////////////////

    cout << " - Loop over datasets ... " << datasets.size () << " datasets !" << endl;

    for (unsigned int d = 0; d < datasets.size(); d++)
    {
        cout<<"Load Dataset"<<endl;
        treeLoader.LoadDataset (datasets[d], anaEnv);  //open files and load dataset
        string previousFilename = "";
        int iFile = -1;
        bool nlo = false;
        dataSetName = datasets[d]->Name();
        if(dataSetName.find("bx50") != std::string::npos) bx25 = false;
        else bx25 = true;

        if(dataSetName.find("NLO") != std::string::npos || dataSetName.find("nlo") !=std::string::npos) nlo = true;
        else nlo = false;

        if(bx25) cout << "Dataset with 25ns Bunch Spacing!" <<endl;
        else cout << "Dataset with 50ns Bunch Spacing!" <<endl;
        if(nlo) cout << "NLO Dataset!" <<endl;
        else cout << "LO Dataset!" << endl;


        //////////////////////////////////////////////
        // Setup Date string and nTuple for output  //
        //////////////////////////////////////////////

        time_t t = time(0);   // get time now
        struct tm * now = localtime( & t );

        int year = now->tm_year + 1900;
        int month =  now->tm_mon + 1;
        int day = now->tm_mday;
        int hour = now->tm_hour;
        int min = now->tm_min;
        int sec = now->tm_sec;

        string year_str;
        string month_str;
        string day_str;
        string hour_str;
        string min_str;
        string sec_str;

        ostringstream convert;   // stream used for the conversion
        convert << year;      // insert the textual representation of 'Number' in the characters in the stream
        year_str = convert.str();
        convert.str("");
        convert.clear();
        convert << month;      // insert the textual representation of 'Number' in the characters in the stream
        month_str = convert.str();
        convert.str("");
        convert.clear();
        convert << day;      // insert the textual representation of 'Number' in the characters in the stream
        day_str = convert.str();
        convert.str("");
        convert.clear();
        convert << hour;      // insert the textual representation of 'Number' in the characters in the stream
        hour_str = convert.str();
        convert.str("");
        convert.clear();
        convert << min;      // insert the textual representation of 'Number' in the characters in the stream
        min_str = convert.str();
        convert.str("");
        convert.clear();
        convert << day;      // insert the textual representation of 'Number' in the characters in the stream
        sec_str = convert.str();
        convert.str("");
        convert.clear();


        string date_str = day_str + "_" + month_str + "_" + year_str;

        cout <<"DATE STRING   "<<date_str << endl;

        //string dataSetName = datasets[d]->Name();
        string channel_dir = "Craneens"+channelpostfix;
        string date_dir = channel_dir+"/Craneens" + date_str +"/";
        int mkdirstatus = mkdir(channel_dir.c_str(),0777);
        mkdirstatus = mkdir(date_dir.c_str(),0777);



        //     string Ntupname = "Craneens/Craneen_" + dataSetName +postfix + "_" + date_str+  ".root";

        string Ntupname = "Craneens"+channelpostfix+"/Craneens"+ date_str  +"/Craneen_" + dataSetName +postfix + ".root";
        string Ntuptitle = "Craneen_" + channelpostfix;
        string controlTuptitle = "ControlCraneen_" + channelpostfix;

        TFile * tupfile = new TFile(Ntupname.c_str(),"RECREATE");
//        TFile * tupMfile = new TFile(NMtupname.c_str(),"RECREATE");

        // TNtuple * tup = new TNtuple(Ntuptitle.c_str(),Ntuptitle.c_str(),"nJets:nLtags:nMtags:nTtags:HT:LeadingMuonPt:LeadingMuonEta:LeadingElectronPt:LeadingBJetPt:HT2M:HTb:HTH:HTRat:topness:ScaleFactor:PU:NormFactor:Luminosity:GenWeight");

        TNtuple * tup = new TNtuple(Ntuptitle.c_str(),Ntuptitle.c_str(),"BDT:nJets:nFatJets:nWTags:nTopTags:nLtags:nMtags:nTtags:3rdJetPt:4thJetPt:HT:LeadingMuonPt:LeadingMuonEta:LeadingElectronPt:LeadingBJetPt:HT2L:HTb:HTH:HTRat:topness:EventSph:EventCen:DiLepSph:DiLepCen:TopDiLepSph:TopDiLepCen:ScaleFactor:PU:NormFactor:Luminosity:GenWeight");
        TNtuple * controlTup = new TNtuple(controlTuptitle.c_str(),controlTuptitle.c_str(),"LeadingLeptonPt:LeadingLeptonIso:LeadingLeptonEta:SecondLeptonPt:SecondLeptonIso:SecondLeptonEta:LeadingJetPt:LeadingBJetPt:nJets:nMtags:ScaleFactor:PU:NormFactor:Luminosity");


        //////////////////////////////////////////////////
        /// Initialize JEC factors ///////////////////////
        //////////////////////////////////////////////////

        vector<JetCorrectorParameters> vCorrParam;
        JetCorrectionUncertainty *jecUnc;

        if((dataSetName.find("Data")<=0 || dataSetName.find("data")<=0 || dataSetName.find("DATA")<=0) && bx25) // 25ns Data!
        {
            JetCorrectorParameters *L1JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_DATA_L1FastJet_AK4PFchs.txt");
            vCorrParam.push_back(*L1JetCorPar);
            JetCorrectorParameters *L2JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_DATA_L2Relative_AK4PFchs.txt");
            vCorrParam.push_back(*L2JetCorPar);
            JetCorrectorParameters *L3JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_DATA_L3Absolute_AK4PFchs.txt");
            vCorrParam.push_back(*L3JetCorPar);
            JetCorrectorParameters *L2L3ResJetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_DATA_L2L3Residual_AK4PFchs.txt");
            vCorrParam.push_back(*L2L3ResJetCorPar);
            jecUnc = new JetCorrectionUncertainty("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_DATA_Uncertainty_AK4PFchs.txt");
        }
        else if((dataSetName.find("Data")<=0 || dataSetName.find("data")<=0 || dataSetName.find("DATA")<=0) && !bx25) // 50ns Data!
        {
            JetCorrectorParameters *L1JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_DATA_L1FastJet_AK4PFchs.txt");
            vCorrParam.push_back(*L1JetCorPar);
            JetCorrectorParameters *L2JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_DATA_L2Relative_AK4PFchs.txt");
            vCorrParam.push_back(*L2JetCorPar);
            JetCorrectorParameters *L3JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_DATA_L3Absolute_AK4PFchs.txt");
            vCorrParam.push_back(*L3JetCorPar);
            JetCorrectorParameters *L2L3ResJetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_DATA_L2L3Residual_AK4PFchs.txt");
            vCorrParam.push_back(*L2L3ResJetCorPar);
            jecUnc = new JetCorrectionUncertainty("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_DATA_Uncertainty_AK4PFchs.txt");
        }
        else if(bx25) // 25ns MC!
        {
            JetCorrectorParameters *L1JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_MC_L1FastJet_AK4PFchs.txt");
            vCorrParam.push_back(*L1JetCorPar);
            JetCorrectorParameters *L2JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_MC_L2Relative_AK4PFchs.txt");
            vCorrParam.push_back(*L2JetCorPar);
            JetCorrectorParameters *L3JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_MC_L3Absolute_AK4PFchs.txt");
            vCorrParam.push_back(*L3JetCorPar);
            JetCorrectorParameters *L2L3ResJetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_MC_L2L3Residual_AK4PFchs.txt");
            vCorrParam.push_back(*L2L3ResJetCorPar);
            jecUnc = new JetCorrectionUncertainty("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_MC_Uncertainty_AK4PFchs.txt");
        }
        else //50ns MC
        {
            JetCorrectorParameters *L1JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_MC_L1FastJet_AK4PFchs.txt");
            vCorrParam.push_back(*L1JetCorPar);
            JetCorrectorParameters *L2JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_MC_L2Relative_AK4PFchs.txt");
            vCorrParam.push_back(*L2JetCorPar);
            JetCorrectorParameters *L3JetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_MC_L3Absolute_AK4PFchs.txt");
            vCorrParam.push_back(*L3JetCorPar);
            JetCorrectorParameters *L2L3ResJetCorPar = new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_MC_L2L3Residual_AK4PFchs.txt");
            vCorrParam.push_back(*L2L3ResJetCorPar);
            jecUnc = new JetCorrectionUncertainty("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_50nsV5_MC_Uncertainty_AK4PFchs.txt");
        }
//        JetCorrectionUncertainty *jecUnc = new JetCorrectionUncertainty("../TopTreeAnalysisBase/Calibrations/JECFiles/Summer15_25nsV2_DATA_Uncertainty_AK4PFchs.txt");
//    JetCorrectionUncertainty *jecUnc = new JetCorrectionUncertainty(*(new JetCorrectorParameters("JECFiles/Fall12_V7_DATA_UncertaintySources_AK5PFchs.txt", "SubTotalMC")));
//    JetCorrectionUncertainty *jecUncTotal = new JetCorrectionUncertainty(*(new JetCorrectorParameters("JECFiles/Fall12_V7_DATA_UncertaintySources_AK5PFchs.txt", "Total")));

        JetTools *jetTools = new JetTools(vCorrParam, jecUnc, true);

        //////////////////////////////////////////////////
        // Loop on events
        /////////////////////////////////////////////////

        int itrigger = -1, previousRun = -1;

        int start = 0;
        cout << "teh bugz!" << endl;
        unsigned int ending = datasets[d]->NofEvtsToRunOver();

        cout <<"Number of events in total dataset = "<<  ending  <<endl;

        int event_start = startEvent;
        if (verbose > 1) cout << " - Loop over events " << endl;

        float BDTScore, MHT, MHTSig, STJet,muoneta, muonpt,electronpt,bjetpt, EventMass, EventMassX , SumJetMass, SumJetMassX,H,HX ,HTHi,HTRat, HT, HTX,HTH,HTXHX, sumpx_X, sumpy_X, sumpz_X, sume_X, sumpx, sumpy, sumpz, sume, jetpt,PTBalTopEventX,PTBalTopSumJetX , PTBalTopMuMet;

        double currentfrac =0.;
        double end_d;
        if(endEvent > ending)
            end_d = ending;
        else
            end_d = endEvent;

        //end_d = 10000; //artifical ending for debug
        int nEvents = end_d - event_start;
        cout <<"Will run over "<<  (end_d - event_start) << " events..."<<endl;
        cout <<"Starting event = = = = "<< event_start  << endl;
        if(end_d < startEvent)
        {
            cout << "Starting event larger than number of events.  Exiting." << endl;
            return 1;
        }

        //define object containers
        vector<TRootElectron*> selectedElectrons;
        vector<TRootPFJet*>    selectedJets;
        vector<TRootSubstructureJet*>    selectedFatJets;
        vector<TRootPFJet*>    MVASelJets1;
        vector<TRootMuon*>     selectedMuons;
        vector<TRootElectron*> selectedExtraElectrons;
        vector<TRootMuon*>     selectedExtraMuons;
        selectedElectrons.reserve(10);
        selectedMuons.reserve(10);

        TRootRun *runInfos = new TRootRun();
        datasets[d]->runTree()->SetBranchStatus("runInfos*",1);
        datasets[d]->runTree()->SetBranchAddress("runInfos",&runInfos);



        //////////////////////////////////////
        // Begin Event Loop
        //////////////////////////////////////

        for (unsigned int ievt = event_start; ievt < end_d; ievt++)
        {
            BDTScore= -99999.0, MHT = 0.,MHTSig = 0.,muoneta = 0., muonpt =0., electronpt=0., bjetpt =0., STJet = 0., EventMass =0., EventMassX =0., SumJetMass = 0., SumJetMassX=0., HTHi =0., HTRat = 0;
            H = 0., HX =0., HT = 0., HTX = 0.,HTH=0.,HTXHX=0., sumpx_X = 0., sumpy_X= 0., sumpz_X =0., sume_X= 0. , sumpx =0., sumpy=0., sumpz=0., sume=0., jetpt =0., PTBalTopEventX = 0., PTBalTopSumJetX =0.;

            double ievt_d = ievt;
            float centralWeight = 1, scaleUp = 1, scaleDown = 1;
            currentfrac = ievt_d/end_d;
            if (debug)cout <<"event loop 1"<<endl;

            if(ievt%1000 == 0)
            {
                std::cout<<"Processing the "<<ievt<<"th event, time = "<< ((double)clock() - start) / CLOCKS_PER_SEC << " ("<<100*(ievt-start)/(ending-start)<<"%)"<<flush<<"\r"<<endl;
            }

            float scaleFactor = 1.;  // scale factor for the event
            event = treeLoader.LoadEvent (ievt, vertex, init_muons, init_electrons, init_jets, init_fatjets,  mets, debug);  //load event

            float nvertices = vertex.size();
            float normfactor = datasets[d]->NormFactor();

            string currentFilename = datasets[d]->eventTree()->GetFile()->GetName();
            if(previousFilename != currentFilename)
            {
                previousFilename = currentFilename;
                iFile++;
                cout<<"File changed!!! => "<<currentFilename<<endl;
            }


            //cout<<"SetBranchAddress(runInfos,&runInfos) : "<<datasets[d]->runTree()->SetBranchAddress("runInfos",&runInfos)<<endl;
            int rBytes = datasets[d]->runTree()->GetEntry(iFile);

            int currentRun = event->runId();

            if(dataSetName.find("TTJets")!=std::string::npos)
            {
                centralWeight = (event->getWeight(1))/(abs(event->originalXWGTUP()));
                scaleUp = event->getWeight(5)/(abs(event->originalXWGTUP()));
                scaleDown = event->getWeight(9)/(abs(event->originalXWGTUP()));

                //cout <<"Central Weight Index: " << runInfos->getWeightInfo(currentRun).weightIndex("Central scale variation 1") << " Weight : " << centralWeight <<endl;
                //cout <<"Scale Up Weight Index: " << runInfos->getWeightInfo(currentRun).weightIndex("Central scale variation 5") << " Weight : " << scaleUp <<endl;
                //cout <<"Scale Down Weight Index: " << runInfos->getWeightInfo(currentRun).weightIndex("Central scale variation 9") << " Weight : " << scaleDown <<endl;
            }
            else if(dataSetName.find("tttt")!=std::string::npos || dataSetName.find("TTTT")!=std::string::npos)
            {
                centralWeight = (event->getWeight(1001))/(abs(event->originalXWGTUP()));
                scaleUp = event->getWeight(1005)/(abs(event->originalXWGTUP()));
                scaleDown = event->getWeight(1009)/(abs(event->originalXWGTUP()));

                //cout << "Unscaled Central Weight: " << event->getWeight(1001) << " originalXWGTUP: " << event->originalXWGTUP() << endl;

                //cout <<"Central Weight Index: " << runInfos->getWeightInfo(currentRun).weightIndex("scale_variation 1") << " Weight : " << centralWeight <<endl;
                //cout <<"Scale Up Weight Index: " << runInfos->getWeightInfo(currentRun).weightIndex("scale_variation 5") << " Weight : " << scaleUp <<endl;
                //cout <<"Scale Down Weight Index: " << runInfos->getWeightInfo(currentRun).weightIndex("scale_variation 9") << " Weight : " << scaleDown <<endl;
            }


            if(dataSetName.find("scaleup") != std::string::npos)
            {
                scaleFactor *= scaleUp;
            }
            else if(dataSetName.find("scaledown") != std::string::npos)
            {
                scaleFactor *= scaleDown;
            }
            else
            {
                scaleFactor *= centralWeight;
            }


            float rho = event->fixedGridRhoFastjetAll();
            if (debug)cout <<"Rho: " << rho <<endl;

            if (debug)cout <<"Number of Muons Loaded: " << init_muons.size() <<endl;

            for (Int_t initmu =0; initmu < init_muons.size(); initmu++ )
            {
                float initreliso = init_muons[initmu]->relPfIso(4, 0.5);
                MSPlot["InitMuonCutFlow"]->Fill(0, datasets[d], true, Luminosity*scaleFactor);
                if(init_muons[initmu]->isGlobalMuon() && init_muons[initmu]->isPFMuon())
                {
                    MSPlot["InitMuonCutFlow"]->Fill(1, datasets[d], true, Luminosity*scaleFactor);
                    if(fabs(init_muons[initmu]->Pt()) < 20)
                    {
                        MSPlot["InitMuonCutFlow"]->Fill(2, datasets[d], true, Luminosity*scaleFactor);
                        if(fabs(init_muons[initmu]->Eta()) < 2.4)
                        {
                            MSPlot["InitMuonCutFlow"]->Fill(3, datasets[d], true, Luminosity*scaleFactor);
                            if(fabs(init_muons[initmu]->chi2()) < 10)
                            {
                                MSPlot["InitMuonCutFlow"]->Fill(4, datasets[d], true, Luminosity*scaleFactor);
                                if(fabs(init_muons[initmu]->nofTrackerLayersWithMeasurement()) > 5)
                                {
                                    MSPlot["InitMuonCutFlow"]->Fill(5, datasets[d], true, Luminosity*scaleFactor);
                                    if(fabs(init_muons[initmu]->nofValidMuHits()) > 0)
                                    {
                                        MSPlot["InitMuonCutFlow"]->Fill(6, datasets[d], true, Luminosity*scaleFactor);
                                        if(fabs(init_muons[initmu]->d0()) < 0.2)
                                        {
                                            MSPlot["InitMuonCutFlow"]->Fill(7, datasets[d], true, Luminosity*scaleFactor);
                                            if(fabs(init_muons[initmu]->dz()) < 0.5)
                                            {
                                                MSPlot["InitMuonCutFlow"]->Fill(8, datasets[d], true, Luminosity*scaleFactor);
                                                if(init_muons[initmu]->nofValidPixelHits() > 0)
                                                {
                                                    MSPlot["InitMuonCutFlow"]->Fill(9, datasets[d], true, Luminosity*scaleFactor);
                                                    if(init_muons[initmu]->nofMatchedStations() > 1)
                                                    {
                                                        MSPlot["InitMuonCutFlow"]->Fill(10, datasets[d], true, Luminosity*scaleFactor);
                                                        if(initreliso < 0.12)
                                                        {
                                                            MSPlot["InitMuonCutFlow"]->Fill(11, datasets[d], true, Luminosity*scaleFactor);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }


            }


            if (debug)cout <<"Number of Electrons Loaded: " << init_electrons.size() <<endl;
            MSPlot["NbOfElectronsInit"]->Fill(init_electrons.size(), datasets[d], true, Luminosity*scaleFactor );

            for (Int_t initel =0; initel < init_electrons.size(); initel++ )
            {
                float initreliso = ElectronRelIso(init_electrons[initel], rho);
                MSPlot["InitElectronPt"]->Fill(init_electrons[initel]->Pt(), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronEta"]->Fill(init_electrons[initel]->Eta(), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronRelIsolation"]->Fill(initreliso, datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronSuperClusterEta"]->Fill(fabs(init_electrons[initel]->superClusterEta()), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectrondEtaI"]->Fill(fabs(init_electrons[initel]->deltaEtaIn()), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectrondPhiI"]->Fill(fabs(init_electrons[initel]->deltaPhiIn()), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronHoverE"]->Fill(init_electrons[initel]->hadronicOverEm(), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectrond0"]->Fill(fabs(init_electrons[initel]->d0()), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectrondZ"]->Fill(fabs(init_electrons[initel]->dz()), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronEminusP"]->Fill(fabs(1/init_electrons[initel]->E() - 1/init_electrons[initel]->P()), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronConversion"]->Fill(init_electrons[initel]->passConversion(), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronMissingHits"]->Fill(init_electrons[initel]->missingHits(), datasets[d], true, Luminosity*scaleFactor);
                MSPlot["InitElectronCutFlow"]->Fill(0, datasets[d], true, Luminosity*scaleFactor);
                if(init_electrons[initel]->Pt() > 30)
                {
                    MSPlot["InitElectronCutFlow"]->Fill(1, datasets[d], true, Luminosity*scaleFactor);
                    if(fabs(init_electrons[initel]->Eta()) < 2.5)
                    {
                        MSPlot["InitElectronCutFlow"]->Fill(2, datasets[d], true, Luminosity*scaleFactor);
                        if(fabs(init_electrons[initel]->deltaEtaIn()) < 0.009277)
                        {
                            MSPlot["InitElectronCutFlow"]->Fill(3, datasets[d], true, Luminosity*scaleFactor);
                            if(fabs(init_electrons[initel]->deltaPhiIn()) < 0.094739)
                            {
                                MSPlot["InitElectronCutFlow"]->Fill(4, datasets[d], true, Luminosity*scaleFactor);
                                if(fabs(init_electrons[initel]->hadronicOverEm()) < 0.093068)
                                {
                                    MSPlot["InitElectronCutFlow"]->Fill(5, datasets[d], true, Luminosity*scaleFactor);
                                    if(fabs(init_electrons[initel]->d0()) < 0.035904)
                                    {
                                        MSPlot["InitElectronCutFlow"]->Fill(6, datasets[d], true, Luminosity*scaleFactor);
                                        if(fabs(init_electrons[initel]->dz()) < 0.075496)
                                        {
                                            MSPlot["InitElectronCutFlow"]->Fill(7, datasets[d], true, Luminosity*scaleFactor);
                                            if(fabs((1/init_electrons[initel]->E()) - (1/init_electrons[initel]->P())) < 0.189968)
                                            {
                                                MSPlot["InitElectronCutFlow"]->Fill(8, datasets[d], true, Luminosity*scaleFactor);
                                                MSPlot["InitElectronDiagRelIso"]->Fill(init_electrons[initel]->relPfIso(3, 0.5), datasets[d], true, Luminosity*scaleFactor);
                                                MSPlot["InitElectronDiagChIso"]->Fill(init_electrons[initel]->chargedHadronIso(3), datasets[d], true, Luminosity*scaleFactor);
                                                MSPlot["InitElectronDiagNIso"]->Fill(init_electrons[initel]->neutralHadronIso(3), datasets[d], true, Luminosity*scaleFactor);
                                                MSPlot["InitElectronDiagPhIso"]->Fill(init_electrons[initel]->photonIso(3), datasets[d], true, Luminosity*scaleFactor);
                                                MSPlot["InitElectronDiagPUChIso"]->Fill(init_electrons[initel]->puChargedHadronIso(3), datasets[d], true, Luminosity*scaleFactor);
                                                if(ElectronRelIso(init_electrons[initel], rho) < 0.130136)
                                                {
                                                    MSPlot["InitElectronCutFlow"]->Fill(9, datasets[d], true, Luminosity*scaleFactor);
                                                    if(init_electrons[initel]->passConversion())
                                                    {
                                                        MSPlot["InitElectronCutFlow"]->Fill(10, datasets[d], true, Luminosity*scaleFactor);
                                                        if(fabs(init_electrons[initel]->missingHits()) <= 1)
                                                        {
                                                            MSPlot["InitElectronCutFlow"]->Fill(11, datasets[d], true, Luminosity*scaleFactor);

                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }


            }



            string graphName;

            //////////////////
            //Loading Gen jets
            //////////////////

            vector<TRootGenJet*> genjets;
            if( ! (dataSetName == "Data" || dataSetName == "data" || dataSetName == "DATA" ) )
            {
                // loading GenJets as I need them for JER
                genjets = treeLoader.LoadGenJet(ievt);
            }
//            string currentFilename = datasets[d]->eventTree()->GetFile()->GetName();
//            if(previousFilename != currentFilename)
//            {
//                previousFilename = currentFilename;
//                iFile++;
//                cout<<"File changed!!! => "<<currentFilename<<endl;
//            }

            ///////////////////////////////////////////
            //  Trigger
            ///////////////////////////////////////////
            bool trigged = false;
            std::string filterName = "";

            if(previousRun != currentRun)
            {
                cout <<"What run? "<< currentRun<<endl;
                previousRun = currentRun;
                cout << "HLT Debug output" << endl;

                //runInfos->getWeightInfo(currentRun).getweightNameList();

                //runInfos->getHLTinfo(currentRun).gethltNameList();

//                treeLoader.ListTriggers(currentRun, iFile);

                //int weightIdx = runInfos->getWeightInfo(currentRun).weightIndex("Central scale variation 1");



                if(nlo)
                {
                    if(centralWeight < 0.0)
                    {
                        //scaleFactor = -1.0;  //Taking into account negative weights in NLO Monte Carlo
                        negWeights++;
                    }
                }



                if(dataSetName.find("Data")!=string::npos || dataSetName.find("data")!=string::npos || dataSetName.find("DATA")!=string::npos)
                {
                    if (debug)cout <<"event loop 6a"<<endl;

                    // cout << " RUN " << event->runId() << endl;

                    if( Muon && Electron )
                        itrigger = treeLoader.iTrigger (string ("HLT_Mu17_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v2"), currentRun, iFile);
                    else if( Muon && !Electron )
                        itrigger = treeLoader.iTrigger (string ("HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v2"), currentRun, iFile);
                    else if( !Muon && Electron )
                        itrigger = treeLoader.iTrigger (string ("HLT_Ele17_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v2"), currentRun, iFile);

                    if(itrigger == 9999)
                    {
                        cout << "NO VALID TRIGGER FOUND FOR THIS EVENT (DATA) IN RUN " << event->runId() << endl;
                        //   exit(1);
                    }
                }
                else
                {
                    if( Muon && Electron )
                    {
                        itrigger = treeLoader.iTrigger ("HLT_Mu17_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v1", currentRun, iFile);
//                        cout << "iTrigger : " << itrigger << " iFile: " << iFile << endl;
//                        cout << "runInfos Trigger : " << runInfos->getHLTinfo(currentRun).hltPath("HLT_Mu17_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v1") << endl;
                    }
                    else if( Muon && !Electron )
                        itrigger = treeLoader.iTrigger (string ("HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v1"), currentRun, iFile);
                    else if( !Muon && Electron )
                        itrigger = treeLoader.iTrigger (string ("HLT_Ele17_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v1"), currentRun, iFile);

                    if(itrigger == 9999)
                    {
                        cerr << "NO VALID TRIGGER FOUND FOR THIS EVENT (" << dataSetName << ") IN RUN " << event->runId() << endl;
                        //exit(1);
                    }
                }

            } //end previousRun != currentRun

            ///////////////////////
            // JER smearing
            //////////////////////

//            if( ! (dataSetName == "Data" || dataSetName == "data" || dataSetName == "DATA" ) )
//            {
//                //JER
//                doJERShift == 0;
//                if(doJERShift == 1)
//                    jetTools->correctJetJER(init_jets, genjets, mets[0], "minus");
//                else if(doJERShift == 2)
//                    jetTools->correctJetJER(init_jets, genjets, mets[0], "plus");
//                else
//                    jetTools->correctJetJER(init_jets, genjets, mets[0], "nominal");
//
//                //     coutObjectsFourVector(init_muons,init_electrons,init_jets,mets,"After JER correction:");
//
//                // JES sysematic!
//                if (doJESShift == 1)
//                    jetTools->correctJetJESUnc(init_jets, mets[0], "minus");
//                else if (doJESShift == 2)
//                    jetTools->correctJetJESUnc(init_jets, mets[0], "plus");
//
//                //            coutObjectsFourVector(init_muons,init_electrons,init_jets,mets,"Before JES correction:");
//
//            }

            ///////////////////////////////////////////////////////////
            // Event selection
            ///////////////////////////////////////////////////////////

            // Apply trigger selection
//            trigged = treeLoader.EventTrigged (221);  //artifical HLT for Mirena
            trigged = treeLoader.EventTrigged (itrigger);
            //trigged = true;  // Disabling the HLT requirement
            if (debug)cout<<"triggered? Y/N?  "<< trigged  <<endl;
//            if(itrigger == 9999 ) cout << "Lumi Block: " << event->lumiBlockId() << " Event: " << event->eventId() << endl;
            if(!trigged)		   continue;  //If an HLT condition is not present, skip this event in the loop. this is present to cut down on compute time.
            // Declare selection instance
            Run2Selection selection(init_jets, init_fatjets, init_muons, init_electrons, mets);

            //Getting Event Weight




            // Define object selection cuts
            if (Muon && Electron && dilepton)
            {
                if (debug)cout<<"Getting Jets"<<endl;
                selectedJets                                        = selection.GetSelectedJets(); // Relying solely on cuts defined in setPFJetCuts()
                selectedFatJets                                        = selection.GetSelectedFatJets(); // Relying solely on cuts defined in setPFJetCuts()

                if (debug)cout<<"Getting Loose Muons"<<endl;
                selectedMuons                                       = selection.GetSelectedMuons(20, 2.4, 0.25, "Loose", "Spring15");
                selectedExtraMuons                                  = selection.GetSelectedMuons(0, 2.4, 1, "Loose", "Spring15");
                if (debug)cout<<"Getting Loose Electrons"<<endl;
                if(bx25) selectedElectrons                                   = selection.GetSelectedElectrons("Loose","Spring15_25ns",true); // VBTF ID
                else selectedElectrons                                   = selection.GetSelectedElectrons("Loose","Spring15_50ns",true); // VBTF ID

            }
            if (Muon && !Electron && dilepton)
            {
                if (debug)cout<<"Getting Jets"<<endl;
                selectedJets                                        = selection.GetSelectedJets(); // Relying solely on cuts defined in setPFJetCuts()
                if (debug)cout<<"Getting Medium Muons"<<endl;
                selectedMuons                                       = selection.GetSelectedMuons(20, 2.4, 0.25, "Loose", "Spring15");
                if (debug)cout<<"Getting Loose Electrons"<<endl;
                if(bx25) selectedElectrons                                   = selection.GetSelectedElectrons("Loose","Spring15_25ns",true); // VBTF ID
                else selectedElectrons                                   = selection.GetSelectedElectrons("Loose","Spring15_50ns",true); // VBTF ID
            }
            if (!Muon && Electron && dilepton)
            {
                if (debug)cout<<"Getting Jets"<<endl;
                selectedJets                                        = selection.GetSelectedJets(); // Relying solely on cuts defined in setPFJetCuts()
                if (debug)cout<<"Getting Medium Muons"<<endl;
                selectedMuons                                       = selection.GetSelectedMuons(20, 2.4, 0.25, "Loose", "Spring15");
                if (debug)cout<<"Getting Loose Electrons"<<endl;
                if(bx25) selectedElectrons                                   = selection.GetSelectedElectrons("Loose","Spring15_25ns",true); // VBTF ID
                else selectedElectrons                                   = selection.GetSelectedElectrons("Loose","Spring15_50ns",true); // VBTF ID
            }



            vector<TRootJet*>      selectedLBJets;
            vector<TRootJet*>      selectedMBJets;
            vector<TRootJet*>      selectedTBJets;
            vector<TRootJet*>      selectedLightJets;

            int JetCut =0;
            int nMu, nEl, nLooseIsoMu;

            nMu = selectedMuons.size(); //Number of Muons in Event
            nEl = selectedElectrons.size(); //Number of Electrons in Event

            MSPlot["NbOfElectronsPreSel"]->Fill(nEl, datasets[d], true, Luminosity*scaleFactor );


            bool isTagged =false;
            vector<TLorentzVector> selectedMuonsTLV_JC, selectedElectronsTLV_JC, selectedLooseIsoMuonsTLV;
            vector<TLorentzVector> mcParticlesTLV, selectedJetsTLV, mcMuonsTLV, mcPartonsTLV;
            vector<TRootMCParticle*> mcParticlesMatching_;
            vector<int> mcMuonIndex, mcPartonIndex;
            JetPartonMatching muonMatching, jetMatching;

            //////////////////////////////////
            // Preselection Lepton Operations //
            //////////////////////////////////

            float diLepMass = 0, diMuMass = 0;
            bool ZVeto = false;
            float ZMass = 91, ZMassWindow = 15;
            TLorentzVector lep1, lep2, diLep;

            for(int selmu = 0; selmu < selectedMuons.size(); selmu++)
            {
                selectedMuonsTLV_JC.push_back(*selectedMuons[selmu]);
            }

            for(int selel = 0; selel < selectedElectrons.size(); selel++)
            {
                selectedElectronsTLV_JC.push_back(*selectedElectrons[selel]);
            }

            if(nMu ==2 && nEl == 0 && Muon && !Electron)
            {
                lep1 = selectedMuonsTLV_JC[0];
                lep2 = selectedMuonsTLV_JC[1];
                diLep = lep1 + lep2;
                diLepMass = diLep.M();
                MSPlot["DiLepMass"]->Fill(diLepMass, datasets[d], true, Luminosity*scaleFactor );
                for(int mass = 0; mass < 20; mass++)
                {
                    float windowRes = 2.5;
                    if(((diLepMass < (ZMass-(mass*windowRes))) || (diLepMass > (ZMass+(mass*windowRes)))) && diLepMass > 20) MSPlot["ZMassWindowWidthAcc"]->Fill((2*mass*windowRes), datasets[d], true, Luminosity*scaleFactor );
                    else MSPlot["ZMassWindowWidthRej"]->Fill((2*mass*windowRes), datasets[d], true, Luminosity*scaleFactor );
                }
            }

            else if(nEl == 2 && nMu == 0 && Electron && !Muon)
            {
                lep1 = selectedElectronsTLV_JC[0];
                lep2 = selectedElectronsTLV_JC[1];
                diLep = lep1 + lep2;
                diLepMass = diLep.M();
                MSPlot["DiLepMass"]->Fill(diLepMass, datasets[d], true, Luminosity*scaleFactor );
                for(int mass = 0; mass < 20; mass++)
                {
                    float windowRes = 2.5;
                    if(((diLepMass < (ZMass-(mass*windowRes))) || (diLepMass > (ZMass+(mass*windowRes)))) && diLepMass > 20) MSPlot["ZMassWindowWidthAcc"]->Fill((2*mass*windowRes), datasets[d], true, Luminosity*scaleFactor );
                    else MSPlot["ZMassWindowWidthRej"]->Fill((2*mass*windowRes), datasets[d], true, Luminosity*scaleFactor );
                }
            }
            else if(nEl == 1 && nMu == 1 && Electron && Muon)
            {
                lep1 = selectedMuonsTLV_JC[0];
                lep2 = selectedElectronsTLV_JC[0];
                diLep = lep1 + lep2;
                diLepMass = diLep.M();
                MSPlot["DiLepMass"]->Fill(diLepMass, datasets[d], true, Luminosity*scaleFactor );
                for(int mass = 0; mass < 20; mass++)
                {
                    float windowRes = 2.5;
                    if(((diLepMass < (ZMass-(mass*windowRes))) || (diLepMass > (ZMass+(mass*windowRes)))) && diLepMass > 20) MSPlot["ZMassWindowWidthAcc"]->Fill((2*mass*windowRes), datasets[d], true, Luminosity*scaleFactor );
                    else MSPlot["ZMassWindowWidthRej"]->Fill((2*mass*windowRes), datasets[d], true, Luminosity*scaleFactor );
                }
            }


            ///////////////////////////////////////////////////////////////////////////////////
            // Preselection looping over Jet Collection                                      //
            // Summing HT and calculating leading, lagging, and ratio for Selected and BJets //
            ///////////////////////////////////////////////////////////////////////////////////
            float temp_HT = 0., HTb = 0.;

            double p_tags_tagged_mc = 1.;
            double p_tags_untagged_mc = 1.;
            double p_tags_tagged_data = 1.;
            double p_tags_untagged_data = 1.;
            double p_mc = 1., p_data = 1.;
            int jet_flavor;
            float eff=1   ;
            float scaled_eff=1 ;
            float a_eff = 1;
            float sf_a_eff = 1;
            double LightJeteff;
            double JetPt, JetEta;
            double SF_tag =1.;
            double event_weight = 1.;
            for (Int_t seljet =0; seljet < selectedJets.size(); seljet++ )
            {
                selectedJetsTLV.push_back(*selectedJets[seljet]);
                jet_flavor = selectedJets[seljet]->partonFlavour();
                JetPt = selectedJets[seljet]->Pt() ;
                JetEta = selectedJets[seljet]->Eta() ;
                temp_HT += JetPt;
                if (JetPt > 800.) JetPt = 800;
                if (JetEta > 2.4)
                {
                    JetEta = 2.4;
                }
                else if (JetEta < -2.4)
                {
                    JetEta = -2.4;
                }


                if(fabs(jet_flavor) == 5 || fabs(jet_flavor) == 4  )
                {
                    //SF_tag =  bTool->getSF(selectedJets[seljet]->Pt(),selectedJets[seljet]->Eta(),jet_flavor,dobTagEffShift );
                    //  cout <<" "<<endl;
                    ////cout <<"jet SF nom "<< bTool->getWeight(selectedJets[seljet]->Pt(),selectedJets[seljet]->Eta(),jet_flavor,0 )    <<endl;
                    //cout <<"jet SF minus "<< bTool->getWeight(selectedJets[seljet]->Pt(),selectedJets[seljet]->Eta(),jet_flavor,-1 )    <<endl;
                    //cout <<"jet SF plus "<< bTool->getWeight(selectedJets[seljet]->Pt(),selectedJets[seljet]->Eta(),jet_flavor,1 )    <<endl;
                }
                else
                {
                    //  cout <<" light jet... "<<endl;
                    //SF_tag =  bTool->getSF(selectedJets[seljet]->Pt(),selectedJets[seljet]->Eta(),jet_flavor,domisTagEffShift);
                }
                if (selectedJets[seljet]->btag_combinedInclusiveSecondaryVertexV2BJetTags() > 0.605   )
                {

                    selectedLBJets.push_back(selectedJets[seljet]);
                    if (selectedJets[seljet]->btag_combinedInclusiveSecondaryVertexV2BJetTags() > 0.890)
                    {
                        selectedMBJets.push_back(selectedJets[seljet]);

                        if (selectedJets[seljet]->btag_combinedInclusiveSecondaryVertexV2BJetTags() > 0.970)
                        {
                            selectedTBJets.push_back(selectedJets[seljet]);
                        }

                    }
                }
                else
                {
                    selectedLightJets.push_back(selectedJets[seljet]);
                }
            }

            float nJets = selectedJets.size(); //Number of Jets in Event
            float nMtags = selectedMBJets.size(); //Number of CSVM tags in Event
            float nLtags = selectedLBJets.size(); //Number of CSVL tags in Event (includes jets that pass CSVM)
            float nTtags = selectedTBJets.size(); //Number of CSVL tags in Event (includes jets that pass CSVM)
            float nFatJets = selectedFatJets.size();

            //            cout <<" med tags ...   "<< nMtags   <<endl;

            float nTopTags = 0;
            float nWTags = 0;
            ////
            //// W/Top tagging
            ////

            for (int fatjet = 0; fatjet < nFatJets; fatjet++)
            {

                float tau1 = selectedFatJets[fatjet]->Tau1();
                float tau2 = selectedFatJets[fatjet]->Tau2();
                float prunedmass = selectedFatJets[fatjet]->PrunedMass();
                float nsubjets =  selectedFatJets[fatjet]->CmsTopTagNsubjets();
                float minmass =  selectedFatJets[fatjet]->CmsTopTagMinMass();
                float topmass =  selectedFatJets[fatjet]->CmsTopTagMass();

                // cout <<"llop in fat jet "<< " tau1 "   <<  tau1  << " tau2 "<< tau2  << " prunedmass " << prunedmass  << " nsubjets " << nsubjets  << " minmass "  <<minmass<< " topmass "  << topmass <<endl;



                //W-tagging
                if(  (tau2/tau1)  > 0.6 && prunedmass > 50.0 )
                {

                    nWTags++;
                    //cout <<"W-TAG!"<<endl;
                }

                //Top-tagging
                if(  nsubjets  > 2 && minmass > 50.0 &&  topmass > 150.0 )
                {
                    //cout <<"TOP-TAG!"<<endl;
                    nTopTags++;

                }


            }


            //////////////////////
            // Sync'ing cutflow //
            //////////////////////

            if (debug)	cout <<" applying baseline event selection for cut table..."<<endl;
            // Apply primary vertex selection
            bool isGoodPV = selection.isPVSelected(vertex, 4, 24., 2);
            if (debug)	cout <<"PrimaryVertexBit: " << isGoodPV << " TriggerBit: " << trigged <<endl;
            selecTable.Fill(d,0,scaleFactor);
            weightCount += scaleFactor;
            eventCount++;
            if(Muon && Electron && dilepton)   //Muon-Electron Selection Table
            {
                if(isGoodPV && trigged)
                {
                    selecTable.Fill(d,1,scaleFactor);
                    if (nMu==1)
                    {
                        selecTable.Fill(d,2,scaleFactor);
                        if(nEl==1)
                        {
                            selecTable.Fill(d,3,scaleFactor);
                            if(nJets>=4)
                            {
                                selecTable.Fill(d,4,scaleFactor);
                                if(nMtags>=1)
                                {
                                    selecTable.Fill(d,5,scaleFactor);
                                    if(nMtags>=2)
                                    {
                                        selecTable.Fill(d,6,scaleFactor);
                                        if(temp_HT>=500)
                                        {
                                            selecTable.Fill(d,7,scaleFactor);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if(Muon && !Electron && dilepton)   //Muon-Electron Selection Table
            {
                if(diLepMass < 20 || (diLepMass > (ZMass-ZMassWindow) && diLepMass < (ZMass+ZMassWindow))) ZVeto = true;
                if(isGoodPV && trigged)
                {
                    selecTable.Fill(d,1,scaleFactor);
                    if (nMu == 2 && nEl == 0)
                    {
                        selecTable.Fill(d,2,scaleFactor);
                        if(!ZVeto)
                        {
                            selecTable.Fill(d,3,scaleFactor);
                            if(nJets>=4)
                            {
                                selecTable.Fill(d,4,scaleFactor);
                                if(nMtags>=1)
                                {
                                    selecTable.Fill(d,5,scaleFactor);
                                    if(nMtags>=2)
                                    {
                                        selecTable.Fill(d,6,scaleFactor);
                                        if(temp_HT>=500)
                                        {
                                            selecTable.Fill(d,7,scaleFactor);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if(!Muon && Electron && dilepton)   //Muon-Electron Selection Table
            {
                if(diLepMass < 20 || (diLepMass > (ZMass-ZMassWindow) && diLepMass < (ZMass-ZMassWindow))) ZVeto = true;
                if(isGoodPV && trigged)
                {
                    selecTable.Fill(d,1,scaleFactor);
                    if (nEl==2 && nMu ==0)
                    {
                        selecTable.Fill(d,2,scaleFactor);
                        if(!ZVeto)
                        {
                            selecTable.Fill(d,3,scaleFactor);
                            if(nJets>=4)
                            {
                                selecTable.Fill(d,4,scaleFactor);
                                if(nMtags>=1)
                                {
                                    selecTable.Fill(d,5,scaleFactor);
                                    if(nMtags>=2)
                                    {
                                        selecTable.Fill(d,6,scaleFactor);
                                        if(temp_HT>=500)
                                        {
                                            selecTable.Fill(d,7,scaleFactor);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //////////////
            // N-1 Plot //
            //////////////

//            MSPlot["NMinusOne"]->Fill(0, datasets[d], true, Luminosity*scaleFactor );
            if(Muon && Electron && dilepton)
            {
                if(             nMu==1 &&   nEl==1 &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(1, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&              nEl==1 &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(2, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==1 &&               nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(3, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==1 &&   nEl==1 &&               nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(4, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==1 &&   nEl==1 &&   nJets>=4 &&                 temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(5, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==1 &&   nEl==1 &&   nJets>=4 && nMtags>=2 )                     MSPlot["NMinusOne"]->Fill(6, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==1 &&   nEl==1 &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(7, datasets[d], true, Luminosity*scaleFactor );
            }
            if(Muon && !Electron && dilepton)
            {
                if(             nMu==2 &&   !ZVeto &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(1, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&              !ZVeto &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(2, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==2 &&               nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(3, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==2 &&   !ZVeto &&               nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(4, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==2 &&   !ZVeto &&   nJets>=4 &&                 temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(5, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==2 &&   !ZVeto &&   nJets>=4 && nMtags>=2                    )  MSPlot["NMinusOne"]->Fill(6, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nMu==2 &&   !ZVeto &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(7, datasets[d], true, Luminosity*scaleFactor );
            }
            if(!Muon && Electron && dilepton)
            {
                if(             nEl==2 &&   !ZVeto &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(1, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&              !ZVeto &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(2, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nEl==2 &&               nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(3, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nEl==2 &&   !ZVeto &&               nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(4, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nEl==2 &&   !ZVeto &&   nJets>=4 &&                 temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(5, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nEl==2 &&   !ZVeto &&   nJets>=4 && nMtags>=2                    )  MSPlot["NMinusOne"]->Fill(6, datasets[d], true, Luminosity*scaleFactor );
                if(isGoodPV &&  nEl==2 &&   !ZVeto &&   nJets>=4 && nMtags>=2 &&    temp_HT>=500 )  MSPlot["NMinusOne"]->Fill(7, datasets[d], true, Luminosity*scaleFactor );
            }

            /////////////////////////////////
            // Applying baseline selection //
            /////////////////////////////////

            //Filling Histogram of the number of vertices before Event Selection

            if (!trigged) continue;  // check that an HLT was triggered
            if (!isGoodPV) continue; // Check that there is a good Primary Vertex
////            if (!(selectedJets.size() >= 6)) continue; //Selection of a minimum of 6 Jets in Event
//
//          cout <<"Number of Muons, Electrons, Jets, BJets, JetCut, MuonChannel, ElectronChannel ===>  "<< nMu <<"  "  <<nEl<<" "<< selectedJets.size()   <<"  " <<  nLtags   <<"  "<<JetCut  <<"  "<<Muon<<" "<<Electron<<endl;


            if (debug)	cout <<" applying baseline event selection..."<<endl;
            //Apply the lepton, btag and HT selections
            if (Muon && Electron && dilepton)
            {
                if  (  !( nMu == 1 && nEl == 1 )) continue; // Muon-Electron Channel Selection
            }
            else if (Muon && !Electron && dilepton)
            {
                if  (  !( nMu == 2 && nEl == 0 && !ZVeto)) continue; // Muon-Electron Channel Selection
            }
            else if (!Muon && Electron && dilepton)
            {
                if  (  !( nMu == 0 && nEl == 2 && !ZVeto)) continue; // Muon-Electron Channel Selection
            }
            else
            {
                cerr<<"Correct Channel not selected."<<endl;
                exit(1);
            }


            sort(selectedJets.begin(),selectedJets.end(),HighestCVSBtag());

            //Scan for best MET Cut before Jet requirements
            for(int metCut = 0; metCut < 100; metCut = metCut + 5)
            {
                if(mets[0]->Et() > metCut) MSPlot["METCutAcc"]->Fill(metCut, datasets[d], true, Luminosity*scaleFactor );
                else MSPlot["METCutRej"]->Fill(metCut, datasets[d], true, Luminosity*scaleFactor );
            }

            if(debug) cout << "HT: " << temp_HT << " nMTags: " << nMtags << endl;

            if (dilepton && Muon && Electron)
            {
                if (!(nJets>=4 && nMtags >=2 )) continue; //Jet Tag Event Selection Requirements for Mu-El dilepton channel
                if (!(temp_HT >= 500)) continue; //Jet Tag Event Selection Requirements for Mu-El dilepton channel
            }
            else if (dilepton && Muon && !Electron)
            {
                if (!(nJets>=4 && nMtags >=2 )) continue; //Jet Tag Event Selection Requirements for Mu-El dilepton channel
                if (!(temp_HT >= 500)) continue; //Jet Tag Event Selection Requirements for Mu-El dilepton channel
            }
            else if (dilepton && !Muon && Electron)
            {
                if (!(nJets>=4 && nMtags >=2 )) continue; //Jet Tag Event Selection Requirements for Mu-El dilepton channel
                if (!(temp_HT >= 500)) continue; //Jet Tag Event Selection Requirements for Mu-El dilepton channel
            }
            if(debug)
            {
                cout<<"Selection Passed."<<endl;
            }

            passed++;

            vector<TLorentzVector*> selectedMuonTLV_JC;
            selectedMuonTLV_JC.push_back(selectedMuons[0]);

            ///////////////////////
            // Getting Gen Event //
            ///////////////////////

            TRootGenEvent* genEvt = 0;

            if(dataSetName != "data" && dataSetName != "Data" && dataSetName != "Data")
            {
                vector<TRootMCParticle*> mcParticles;
                vector<TRootMCParticle*> mcTops;
                mcParticlesMatching_.clear();
                mcParticlesTLV.clear();
                //selectedJetsTLV.clear();
                mcParticles.clear();
                mcTops.clear();

                int leptonPDG, muonPDG = 13, electronPDG = 11;
                leptonPDG = muonPDG;

                genEvt = treeLoader.LoadGenEvent(ievt,false);
                treeLoader.LoadMCEvent(ievt, genEvt, 0, mcParticlesMatching_,false);
                if (debug) cout <<"size   "<< mcParticlesMatching_.size()<<endl;
            }


            ///////////////////
            // Control Tuple //
            ///////////////////
            if(selectedMuons.size() == 2 && Muon && !Electron)
            {
                float reliso1 = selectedMuons[0]->relPfIso(4, 0.5);
                float reliso2 = selectedMuons[1]->relPfIso(4, 0.5);
                controlTup->Fill(selectedMuons[0]->Pt(), reliso1, selectedMuons[0]->Eta(), selectedMuons[1]->Pt(), reliso2, selectedMuons[1]->Eta(), selectedJets[0]->Pt(), selectedMBJets[0]->Pt(), nJets, nMtags, scaleFactor, nvertices, normfactor, Luminosity);
            }
            else if(selectedMuons.size() == 1 && selectedElectrons.size() == 1 && Muon && Electron)
            {
                float reliso1 = selectedMuons[0]->relPfIso(4, 0.5);
                float reliso2 = ElectronRelIso(selectedElectrons[0], rho);
                controlTup->Fill(selectedMuons[0]->Pt(), reliso1, selectedMuons[0]->Eta(), selectedElectrons[1]->Pt(), reliso2, selectedElectrons[1]->Eta(), selectedJets[0]->Pt(), selectedMBJets[0]->Pt(), nJets, nMtags, scaleFactor, nvertices, normfactor, Luminosity);
            }
            else if(selectedElectrons.size() == 2 && !Muon && Electron)
            {
                float reliso1 = ElectronRelIso(selectedElectrons[0], rho);
                float reliso2 = ElectronRelIso(selectedElectrons[1], rho);
                controlTup->Fill(selectedElectrons[0]->Pt(), reliso1, selectedElectrons[0]->Eta(), selectedElectrons[1]->Pt(), reliso2, selectedElectrons[1]->Eta(), selectedJets[0]->Pt(), selectedMBJets[0]->Pt(), nJets, nMtags, scaleFactor, nvertices, normfactor, Luminosity);
            }


            //////////////////////////////////////
            // MVA Hadronic Top Reconstructions //
            //////////////////////////////////////

            jetCombiner->ProcessEvent_SingleHadTop(datasets[d], mcParticlesMatching_, selectedJets, selectedMuonTLV_JC[0], genEvt, scaleFactor);
            double TriJetMass, DiJetMass;
            vector<TRootPFJet*> MVASelJets1;
            TLorentzVector Wh, Bh, Th;
            int wj1;
            int wj2;
            int bj1;

            if(!TrainMVA)
            {
                MVAvals1 = jetCombiner->getMVAValue(MVAmethod, 1); // 1 means the highest MVA value
                MSPlot["MVA1TriJet"]->Fill(MVAvals1.first, datasets[d], true, Luminosity*scaleFactor );
                topness = MVAvals1.first;
                for (Int_t seljet1 =0; seljet1 < selectedJets.size(); seljet1++ )
                {
                    if (seljet1 == MVAvals1.second[0] || seljet1 == MVAvals1.second[1] || seljet1 == MVAvals1.second[2])
                    {
                        MVASelJets1.push_back(selectedJets[seljet1]);
                    }

                }

                //check data-mc agreement of kin. reco. variables.
                float mindeltaR =100.;
                float mindeltaR_temp =100.;


                //define the jets from W as the jet pair with smallest deltaR
                for (int m=0; m<MVASelJets1.size(); m++)
                {
                    for (int n=0; n<MVASelJets1.size(); n++)
                    {
                        if(n==m) continue;
                        TLorentzVector lj1  = *MVASelJets1[m];
                        TLorentzVector lj2  = *MVASelJets1[n];
                        mindeltaR_temp  = lj1.DeltaR(lj2);
                        if (mindeltaR_temp < mindeltaR)
                        {
                            mindeltaR = mindeltaR_temp;
                            wj1 = m;
                            wj2 = n;
                        }
                    }
                }
                // find the index of the jet not chosen as a W-jet
                for (unsigned int p=0; p<MVASelJets1.size(); p++)
                {
                    if(p!=wj1 && p!=wj2) bj1 = p;
                }

                if (debug) cout <<"Processing event with jetcombiner : 3 "<< endl;

                //now that putative b and W jets are chosen, calculate the six kin. variables.
                Wh = *MVASelJets1[wj1]+*MVASelJets1[wj2];
                Bh = *MVASelJets1[bj1];
                Th = Wh+Bh;

                TriJetMass = Th.M();

                DiJetMass = Wh.M();
                //DeltaR
                float AngleThWh = fabs(Th.DeltaPhi(Wh));
                float AngleThBh = fabs(Th.DeltaPhi(Bh));

                float btag = MVASelJets1[bj1]->btag_combinedInclusiveSecondaryVertexV2BJetTags();

                double PtRat = (( *MVASelJets1[0] + *MVASelJets1[1] + *MVASelJets1[2] ).Pt())/( MVASelJets1[0]->Pt() + MVASelJets1[1]->Pt() + MVASelJets1[2]->Pt() );
                double diLepThdR = fabs(Th.DeltaR(diLep));
                double diLepThdPhi = fabs(Th.DeltaPhi(diLep));
                if (debug) cout <<"Processing event with jetcombiner : 4 "<< endl;

                MSPlot["MVA1TriJetMass"]->Fill(TriJetMass,  datasets[d], true, Luminosity*scaleFactor );
                MSPlot["MVA1DiJetMass"]->Fill(DiJetMass,  datasets[d], true, Luminosity*scaleFactor );
                MSPlot["MVA1BTag"]->Fill(btag,  datasets[d], true, Luminosity*scaleFactor );
                MSPlot["MVA1PtRat"]->Fill(PtRat,  datasets[d], true, Luminosity*scaleFactor );
                MSPlot["MVA1AnThWh"]->Fill(AngleThWh,  datasets[d], true, Luminosity*scaleFactor );
                MSPlot["MVA1AnThBh"]->Fill(AngleThBh,  datasets[d], true, Luminosity*scaleFactor );
                MSPlot["MVA1dRThDiLep"]->Fill(diLepThdR,  datasets[d], true, Luminosity*scaleFactor );
                MSPlot["MVA1dPhiThDiLep"]->Fill(diLepThdPhi,  datasets[d], true, Luminosity*scaleFactor );


                if (debug) cout <<"Processing event with jetcombiner : 8 "<< endl;


            }



            ///////////////////////////////////
            // Filling histograms / plotting //
            ///////////////////////////////////

            MSPlot["NbOfVertices"]->Fill(vertex.size(), datasets[d], true, Luminosity*scaleFactor);




            //////////////////////
            // Muon Based Plots //
            //////////////////////
            for (Int_t selmu =0; selmu < selectedMuons.size(); selmu++ )
            {
                MSPlot["MuonPt"]->Fill(selectedMuons[selmu]->Pt(), datasets[d], true, Luminosity*scaleFactor);
                float reliso = selectedMuons[selmu]->relPfIso(4, 0.5);
                MSPlot["MuonRelIsolation"]->Fill(reliso, datasets[d], true, Luminosity*scaleFactor);
            }

            //////////////////////////
            // Electron Based Plots //
            //////////////////////////

            for (Int_t selel =0; selel < selectedElectrons.size(); selel++ )
            {
                float reliso = ElectronRelIso(selectedElectrons[selel], rho);
                MSPlot["ElectronRelIsolation"]->Fill(reliso, datasets[d], true, Luminosity*scaleFactor);
                MSPlot["ElectronPt"]->Fill(selectedElectrons[selel]->Pt(), datasets[d], true, Luminosity*scaleFactor);
            }

            //////////////////////
            // Jets Based Plots //
            //////////////////////

            HT = 0;
            float HT1M2L=0, H1M2L=0, HTbjets=0, HT2M=0, H2M=0;


            for (Int_t seljet1 =0; seljet1 < selectedJets.size(); seljet1++ )
            {
                if(nMtags>=2 && seljet1>=2)
                {
                    jetpt = selectedJets[seljet1]->Pt();
                    HT2M = HT2M + jetpt;
                    H2M = H2M + selectedJets[seljet1]->P();
                }
                if(selectedJets[seljet1]->btag_combinedInclusiveSecondaryVertexV2BJetTags() > 0.244)
                {
                    HTb += selectedJets[seljet1]->Pt();
                }
                MSPlot["BdiscBJetCand_CSV"]->Fill(selectedJets[seljet1]->btag_combinedInclusiveSecondaryVertexV2BJetTags(),datasets[d], true, Luminosity*scaleFactor);
                MSPlot["JetEta"]->Fill(selectedJets[seljet1]->Eta() , datasets[d], true, Luminosity*scaleFactor);
                //Event-level variables
                jetpt = selectedJets[seljet1]->Pt();
                HT = HT + jetpt;
                H = H +  selectedJets[seljet1]->P();
                if (seljet1 > 2  )  HTHi +=  selectedJets[seljet1]->Pt();
            }

            HTRat = HTHi/HT;
            HTH = HT/H;

            MSPlot["HTExcess2M"]->Fill(HT2M, datasets[d], true, Luminosity*scaleFactor);
            MSPlot["HTH"]->Fill(HTH, datasets[d], true, Luminosity*scaleFactor);
            MSPlot["HT_SelectedJets"]->Fill(HT, datasets[d], true, Luminosity*scaleFactor);
            histo2D["HTLepSep"]->Fill(HT, lep1.DeltaR(lep2));
            sort(selectedJets.begin(),selectedJets.end(),HighestPt()); //order Jets wrt Pt for tuple output

            MSPlot["3rdJetPt"]->Fill(selectedJets[2]->Pt(), datasets[d], true, Luminosity*scaleFactor);
            MSPlot["4thJetPt"]->Fill(selectedJets[3]->Pt(), datasets[d], true, Luminosity*scaleFactor);
            if(selectedJets.size() >= 5) MSPlot["5thJetPt"]->Fill(selectedJets[4]->Pt(), datasets[d], true, Luminosity*scaleFactor);
            if(selectedJets.size() >= 6) MSPlot["6thJetPt"]->Fill(selectedJets[5]->Pt(), datasets[d], true, Luminosity*scaleFactor);

            if(dilepton && Muon && Electron)
            {
                muonpt = selectedMuons[0]->Pt();
                muoneta = selectedMuons[0]->Eta();
                electronpt  = selectedElectrons[0]->Pt();
            }
            if(dilepton && Muon && !Electron)
            {
                muonpt = selectedMuons[0]->Pt();
                muoneta = selectedMuons[0]->Eta();
            }
            if(dilepton && !Muon && Electron)
            {
                muonpt = selectedElectrons[0]->Pt();
                muoneta = selectedElectrons[0]->Eta();
            }


            bjetpt= selectedLBJets[0]->Pt();






            ///////////////////
            //MET Based Plots//
            ///////////////////

            MSPlot["MET"]->Fill(mets[0]->Et(), datasets[d], true, Luminosity*scaleFactor);

            //////////////////
            //Topology Plots//
            /////////////////

            vector<TLorentzVector> selectedParticlesTLV, diLepSystemTLV, topDiLepSystemTLV;
            //collection Total Event TLVs
            selectedParticlesTLV.insert(selectedParticlesTLV.end(), selectedElectronsTLV_JC.begin(), selectedElectronsTLV_JC.end());
            selectedParticlesTLV.insert(selectedParticlesTLV.end(), selectedMuonsTLV_JC.begin(), selectedMuonsTLV_JC.end());
            selectedParticlesTLV.insert(selectedParticlesTLV.end(), selectedJetsTLV.begin(), selectedJetsTLV.end());
            selectedParticlesTLV.push_back(*mets[0]);
            //collecting diLep TLVs
            diLepSystemTLV.push_back(lep1);
            diLepSystemTLV.push_back(lep2);
            diLepSystemTLV.push_back(*mets[0]);
            //collecting topDiLep TLVs
            topDiLepSystemTLV.insert(topDiLepSystemTLV.end(), diLepSystemTLV.begin(), diLepSystemTLV.end());
            topDiLepSystemTLV.push_back(*MVASelJets1[wj1]);
            topDiLepSystemTLV.push_back(*MVASelJets1[wj2]);
            topDiLepSystemTLV.push_back(*MVASelJets1[bj1]);

            float tSph = Sphericity(selectedParticlesTLV), tCen = Centrality(selectedParticlesTLV);
            float dSph = Sphericity(diLepSystemTLV), dCen = Centrality(diLepSystemTLV);
            float tdSph = Sphericity(topDiLepSystemTLV), tdCen = Centrality(topDiLepSystemTLV);

            MSPlot["TotalSphericity"]->Fill(tSph, datasets[d], true, Luminosity*scaleFactor);
            MSPlot["TotalCentrality"]->Fill(tCen, datasets[d], true, Luminosity*scaleFactor);
            MSPlot["DiLepSphericity"]->Fill(dSph, datasets[d], true, Luminosity*scaleFactor);
            MSPlot["DiLepCentrality"]->Fill(dCen, datasets[d], true, Luminosity*scaleFactor);
            MSPlot["TopDiLepSphericity"]->Fill(tdSph, datasets[d], true, Luminosity*scaleFactor);
            MSPlot["TopDiLepCentrality"]->Fill(tdCen, datasets[d], true, Luminosity*scaleFactor);

            /////////////////////
            // Calculating BDT //
            /////////////////////

            Eventcomputer_->FillVar("topness",topness);
            Eventcomputer_->FillVar("muonpt",muonpt);
            Eventcomputer_->FillVar("muoneta",muoneta);
            Eventcomputer_->FillVar("HTH", HTH);
            Eventcomputer_->FillVar("HTRat",HTRat);
            Eventcomputer_->FillVar("HTb", HTb);
            Eventcomputer_->FillVar("nLtags",nLtags );
            Eventcomputer_->FillVar("nMtags",nMtags );
            Eventcomputer_->FillVar("nTtags",nTtags );
            Eventcomputer_->FillVar("nJets", selectedJets.size() );
            Eventcomputer_->FillVar("Jet3Pt", selectedJets[2]->Pt() );
            Eventcomputer_->FillVar("Jet4Pt", selectedJets[3]->Pt() );
            Eventcomputer_->FillVar("HT2M",HT2M);
            Eventcomputer_->FillVar("EventSph",tSph);
            Eventcomputer_->FillVar("EventCen",tCen);
            Eventcomputer_->FillVar("DiLepSph",dSph);
            Eventcomputer_->FillVar("DiLepCen",dCen);
            Eventcomputer_->FillVar("TopDiLepSph",tdSph);
            Eventcomputer_->FillVar("TopDiLepCen",tdCen);

            std::map<std::string,Float_t> MVAVals = Eventcomputer_->GetMVAValues();

            for (std::map<std::string,Float_t>::const_iterator it = MVAVals.begin(); it != MVAVals.end(); ++it)
            {

                //  cout <<"MVA Method : "<< it->first    <<" Score : "<< it->second <<endl;
                BDTScore = it->second;

            }

            if((dataSetName.find("Data")<=0 || dataSetName.find("data")<=0 || dataSetName.find("DATA")<=0) && Muon && Electron)
            {
                cout <<"Data Event Passed Selection.  Run: "<< event->runId() << " LumiSection: " << event->lumiBlockId() << " Event: "<< event->eventId() << " HT: " << HT << " nMTags: " << nMtags <<endl;
                cout <<"Muon Eta: " << selectedMuons[0]->Eta() << " Muon Pt: " << selectedMuons[0]->Pt() << " Electron Eta: " << selectedElectrons[0]->Eta() << " Electron Pt: " << selectedElectrons[0]->Pt() << endl;
            }

            if((dataSetName.find("Data")<=0 || dataSetName.find("data")<=0 || dataSetName.find("DATA")<=0) && Muon && !Electron)
            {
                cout <<"Data Event Passed Selection.  Run: "<< event->runId() << " LumiSection: " << event->lumiBlockId() << " Event: "<< event->eventId() << " HT: " << HT << " nMTags: " << nMtags <<endl;
                cout <<"Muon1 Eta: " << selectedMuons[0]->Eta() << " Muon1 Pt: " << selectedMuons[0]->Pt() << " Muon2 Eta: " << selectedMuons[1]->Eta() << " Muon2 Pt: " << selectedMuons[1]->Pt() << endl;
            }
            if((dataSetName.find("Data")<=0 || dataSetName.find("data")<=0 || dataSetName.find("DATA")<=0) && !Muon && Electron)
            {
                cout <<"Data Event Passed Selection.  Run: "<< event->runId() << " LumiSection: " << event->lumiBlockId() << " Event: "<< event->eventId() << " HT: " << HT << " nMTags: " << nMtags <<endl;
                cout <<"Electron1 Eta: " << selectedElectrons[0]->Eta() << " Electron1 Pt: " << selectedElectrons[0]->Pt() << " Electron2 Eta: " << selectedElectrons[1]->Eta() << " Electron2 Pt: " << selectedElectrons[1]->Pt() << endl;
            }

            //////////////////
            //Filling nTuple//
            //////////////////

            //	  tup->Fill(nJets,nLtags,nMtags,nTtags,HT,muonpt,muoneta,electronpt,bjetpt,HT2M,HTb,HTH,HTRat,topness,scaleFactor,nvertices,normfactor,Luminosity,weight_0);


            float vals[31] = {BDTScore,nJets,nFatJets,nWTags,nTopTags,nLtags,nMtags,nTtags,selectedJets[2]->Pt(),selectedJets[3]->Pt(),HT,0,0,0,bjetpt,HT2M,HTb,HTH,HTRat,topness,tSph,tCen,dSph,dCen,tdSph,tdCen,scaleFactor,nvertices,normfactor,Luminosity,centralWeight};
            //                "BDT:nJets:nFatJets:nWTags:nTopTags:nLtags:nMtags:nTtags:HT:LeadingMuonPt:LeadingMuonEta:LeadingElectronPt:LeadingBJetPt:HT2L:HTb:HTH:HTRat:topness:ScaleFactor:PU:NormFactor:Luminosity:GenWeight");
            if(Muon && Electron)
            {
                vals[11] = muonpt;
                vals[12] = muoneta;
                vals[13] = selectedElectrons[0]->Pt();
            }
            if(Muon && !Electron)
            {
                vals[11] = muonpt;
                vals[12] = muoneta;
                vals[13] = selectedMuons[1]->Pt();
            }
            if(!Muon && Electron)
            {
                vals[11] = muonpt;
                vals[12] = muoneta;
                vals[13] = selectedElectrons[1]->Pt();
            }

            tup->Fill(vals);


        } //End Loop on Events

        tupfile->cd();
        tup->Write();
        tupfile->Close();


//        tupMfile->cd();
//        Mtup->Write();
//        tupMfile->Close();
        cout <<"n events passed  =  "<<passed <<endl;
        cout <<"n events with negative weights = "<<negWeights << endl;
        cout << "Event Count: " << eventCount << endl;
        cout << "Weight Count: " << weightCount << endl;
        //important: free memory
        treeLoader.UnLoadDataset();
    } //End Loop on Datasets

    eventlist.close();

    /////////////
    // Writing //
    /////////////

    cout << " - Writing outputs to the files ..." << endl;

    //////////////////////
    // Selection tables //
    //////////////////////

    //(bool mergeTT, bool mergeQCD, bool mergeW, bool mergeZ, bool mergeST)
    selecTable.TableCalculator(  true, true, true, true, true);

    //Options : WithError (false), writeMerged (true), useBookTabs (false), addRawsyNumbers (false), addEfficiencies (false), addTotalEfficiencies (false), writeLandscape (false)
    selecTable.Write(  outputDirectory+"/FourTop"+postfix+"_Table"+channelpostfix+".tex",    false,true,true,true,false,false,true);

    fout->cd();
    //TFile *foutmva = new TFile ("foutMVA.root","RECREATE");
    cout <<" after cd .."<<endl;

    string pathPNGJetCombi = pathPNG + "JetCombination/";
    mkdir(pathPNGJetCombi.c_str(),0777);
//    if(TrainMVA)jetCombiner->Write(foutmva, true, pathPNGJetCombi.c_str());

//Output ROOT file
    for(map<string,MultiSamplePlot*>::const_iterator it = MSPlot.begin();
            it != MSPlot.end();
            it++)
    {
        string name = it->first;
        MultiSamplePlot *temp = it->second;
        temp->Draw(name.c_str(), 0, false, false, false, 1);
        temp->Write(fout, name, false, pathPNG, "pdf");
    }


    TDirectory* th2dir = fout->mkdir("Histos2D");
    th2dir->cd();


    for(map<std::string,TH2F*>::const_iterator it = histo2D.begin(); it != histo2D.end(); it++)
    {

        TH2F *temp = it->second;
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

float Sphericity(vector<TLorentzVector> parts )
{
    if(parts.size()>0)
    {
        double spTensor[3*3] = {0.,0.,0.,0.,0.,0.,0.,0.,0.};
        int counter = 0;
        float tensorNorm = 0, y1 = 0, y2 = 0, y3 = 0;

        for(int tenx = 0; tenx < 3; tenx++)
        {
            for(int teny = 0; teny < 3; teny++)
            {
                for(int selpart = 0; selpart < parts.size(); selpart++)
                {

                    spTensor[counter] += ((parts[selpart][tenx])*(parts[selpart][teny]));
//                    if((tenx == 0 && teny == 2) || (tenx == 2 && teny == 1))
//                    {
//                    cout << "nan debug term " << counter+1 << ": " << (parts[selpart][tenx])*(parts[selpart][teny]) << endl;
//                    cout << "Tensor Building Term " << counter+1 << ": " << spTensor[counter] << endl;
//                    }
                    if(tenx ==0 && teny == 0)
                    {
                        tensorNorm += parts[selpart].Vect().Mag2();
                    }
                }
                if((tenx == 0 && teny == 2) || (tenx == 2 && teny == 1))
                {
//                    cout << "Tensor term pre-norm " << counter+1 << ": " << spTensor[counter] << endl;
                }
                spTensor[counter] /= tensorNorm;
//                cout << "Tensor Term " << counter+1 << ": " << spTensor[counter] << endl;
                counter++;
            }
        }
        TMatrixDSym m(3, spTensor);
        //m.Print();
        TMatrixDSymEigen me(m);
        TVectorD eigenval = me.GetEigenValues();
        vector<float> eigenVals;
        eigenVals.push_back(eigenval[0]);
        eigenVals.push_back(eigenval[1]);
        eigenVals.push_back(eigenval[2]);
        sort(eigenVals.begin(), eigenVals.end());
        //cout << "EigenVals: "<< eigenVals[0] << ", " << eigenVals[1] << ", " << eigenVals[2] << ", " << endl;
        float sp = 3.0*(eigenVals[0] + eigenVals[1])/2.0;
        //cout << "Sphericity: " << sp << endl;
        return sp;
    }
    else
    {
        return 0;
    }
}
float Centrality(vector<TLorentzVector> parts)
{
    float E = 0, ET = 0;
    for(int selpart = 0; selpart < parts.size(); selpart++)
    {
        E += parts[selpart].E();
        ET += parts[selpart].Et();
    }
    return ET/E;
}

float ElectronRelIso(TRootElectron* el, float rho)
{
    double EffectiveArea = 0.;
    // Updated to 2015 EA from https://indico.cern.ch/event/370494/contribution/2/attachments/736984/1011061/Rami_update_on_CB_ELE_ID_PHYS14PU20bx25.pdf
    if (fabs(el->superClusterEta()) >= 0.0   && fabs(el->superClusterEta()) < 0.8   ) EffectiveArea = 0.1013;
    if (fabs(el->superClusterEta()) >= 0.8   && fabs(el->superClusterEta()) < 1.3 ) EffectiveArea = 0.0988;
    if (fabs(el->superClusterEta()) >= 1.3 && fabs(el->superClusterEta()) < 2.0   ) EffectiveArea = 0.0572;
    if (fabs(el->superClusterEta()) >= 2.0   && fabs(el->superClusterEta()) < 2.2   ) EffectiveArea = 0.0842;
    if (fabs(el->superClusterEta()) >= 2.2   && fabs(el->superClusterEta()) < 2.5   ) EffectiveArea = 0.1530;
    if (fabs(el->superClusterEta()) >= 2.5) EffectiveArea = -9999;

    double isoCorr = 0;
    isoCorr = el->neutralHadronIso(3) + el->photonIso(3) - rho*EffectiveArea;
    float isolation = (el->chargedHadronIso(3) + (isoCorr > 0.0 ? isoCorr : 0.0))/(el->Pt());

    return isolation;
}
