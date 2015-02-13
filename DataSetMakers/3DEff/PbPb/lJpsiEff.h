#include <iostream>
#include <vector>

#include <TROOT.h>
#include <TStyle.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TH3D.h>
#include <TF1.h>
#include <TFitResult.h>
#include <TGraphAsymmErrors.h>
#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TClonesArray.h>
#include <TLorentzVector.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TChain.h>

using namespace std;

bool isForwardLowpT(double ymin, double ymax, double ptmin, double ptmax) {
//  if (ptmax<=6.5) return true;
//  if (ptmax<=6.5 && fabs(ymin)>=1.6 && fabs(ymax)<=2.4) return true;
//  else return false;
  return false;

}

bool isMuonInAccept(const TLorentzVector *aMuon) {
  return (fabs(aMuon->Eta()) < 2.4 &&
         ((fabs(aMuon->Eta()) < 1.0 && aMuon->Pt() >= 3.4) ||
         (1.0 <= fabs(aMuon->Eta()) && fabs(aMuon->Eta()) < 1.5 && aMuon->Pt() >= 5.8-2.4*fabs(aMuon->Eta())) ||
         (1.5 <= fabs(aMuon->Eta()) && aMuon->Pt() >= 3.3667-7.0/9.0*fabs(aMuon->Eta()))));
}

double fitERF(double *x, double *par) {
    return par[0]*TMath::Erf((x[0]-par[1])/par[2]);
}

double getAvgEffInRapPt(TH1D *h, double xmin, double xmax) {
  double avgEff = 0;
  int nbins = 0;
  int xaxismin = h->FindBin(xmin);
  int xaxismax = h->FindBin(xmax);
  for (int i=xaxismin; i<=xaxismax; i++) {
    double bincont = h->GetBinContent(i+1);
    if (bincont) {
      avgEff += bincont;
      nbins++;
    }
  }
  avgEff /= nbins;
  return avgEff;
}

double findCenWeight(const int Bin) {
  double NCollArray[40]={
    1747.8600, 1567.5300, 1388.3900, 1231.7700, 1098.2000, 980.4390, 861.6090, 766.0420, 676.5150, 593.4730,
    521.9120, 456.5420, 398.5460, 346.6470, 299.3050, 258.3440, 221.2160, 188.6770, 158.9860, 134.7000,
    112.5470, 93.4537, 77.9314, 63.5031, 52.0469, 42.3542, 33.9204, 27.3163, 21.8028, 17.2037,
    13.5881, 10.6538, 8.3555, 6.4089, 5.1334, 3.7322, 3.0663, 2.4193, 2.1190, 1.7695
  };
  return(NCollArray[Bin]);
}

void getCorrectedEffErr(const int nbins, TH1 *hrec, TH1 *hgen, TH1 *heff) {
  for (int a=0; a<nbins; a++) {
    double genInt = hgen->GetBinContent(a+1);
    double genErr = hgen->GetBinError(a+1);
    double recInt = hrec->GetBinContent(a+1);
    double recErr = hrec->GetBinError(a+1);
    double eff = recInt / genInt;

    double tmpErrGen1 = TMath::Power(eff,2) / TMath::Power(genInt,2);
    double tmpErrRec1 = TMath::Power(recErr,2);
    double tmpErr1 = tmpErrGen1 * tmpErrRec1;

    double tmpErrGen2 = TMath::Power(1-eff,2) / TMath::Power(genInt,2);
    double tmpErrRec2 = TMath::Abs(TMath::Power(genErr,2) - TMath::Power(recErr,2));
    double tmpErr2 = tmpErrGen2 * tmpErrRec2;
    double effErr = TMath::Sqrt(tmpErr1 + tmpErr2);

    if (genInt == 0) {
      heff->SetBinContent(a+1, 0);
      heff->SetBinError(a+1, 0);
    } else {
      heff->SetBinContent(a+1, eff);
      heff->SetBinError(a+1, effErr);
    }
  }
}


class Eff3DMC {
  private:
    bool absRapidity, npmc, isPbPb;

    string inFileNames[100], className, outFileName;
    TFile *file, *inFile[100], *outfile, *fileSinMuW, *fileSinMuW_LowPt;
    TTree *tree;
    TChain *chain;
    int nFiles;
    
    int centrality, HLTriggers,  Reco_QQ_trig[100], Reco_QQ_sign[100];
    int Reco_QQ_size, Gen_QQ_size;
    float Reco_QQ_ctau[100], Reco_QQ_ctauTrue[100], Gen_QQ_ctau[100], Reco_QQ_VtxProb[100];
    TClonesArray *Reco_QQ_4mom, *Gen_QQ_4mom;
    TClonesArray *Reco_QQ_mupl_4mom, *Gen_QQ_mupl_4mom;
    TClonesArray *Reco_QQ_mumi_4mom, *Gen_QQ_mumi_4mom;
    
    int nbinsy, nbinsy2, nbinspt, nbinspt2, nbinsctau, nbinsmidctau, nbinsforwctau, nbinscent, nbinscent2, nbinsresol;
    double resolmin, resolmax;

    // Rap histo (integrated over all other variables)
    TH1D *h1DGenRap, *h1DRecRap, *h1DEffRap;
    // Pt histo (integrated over all other variables)
    TH1D *h1DGenPt, *h1DRecPt, *h1DEffPt;
    TH1D *h1DGenPtFit[100], *h1DRecPtFit[100], *h1DEffPtFit[100], *h1DMeanPtFit[100][20];
    TF1 *f1DEffPtFit[100];
    TGraphAsymmErrors *g1DEffPtFit[100];
    // Cent histo (integrated over all other variables)
    TH1D *h1DGenCent, *h1DRecCent, *h1DEffCent;


  public:
    Eff3DMC(int _nFiles, string str1[], string str2, bool abs, bool npmc, bool isPbPb);
    Eff3DMC(string, string, bool, bool, bool);
    ~Eff3DMC();
    int SetTree();
    void CreateHistos(const int _nbinsy, const double *yarray, const int _nbinsy2, const double *yarray2, const int _nbinspt, const double *ptarray, const int _nbinspt2, const double *ptarray2, const int _nbinscent, const int *centarray, const int _nbinscent2, const int *centarray2, const int _nbinsctau, const double *_ctauarray, const int _nbinsforwctau, const double *_ctauforwarray);
    void LoopTree(const double *yarray, const double *yarray2, const double *ptarray, const double *ptarray2, const int *centarray, const int *centarray2);
    void GetEfficiency(const double *raparray2, const double *ptarray2, const int *centarray2);
    void SaveHistos(string str);
};

Eff3DMC::Eff3DMC(int _nFiles, string str1[], string str2, bool abs, bool _npmc, bool _isPbPb) {
  npmc = _npmc;
  nFiles = _nFiles;
  for (int i=0; i<nFiles; i++) {
    inFileNames[i] = str1[i];
    cout << "inFileNames[" << i << "]: " << inFileNames[i] << endl;
  }
  className = str2;
  absRapidity = abs;
  isPbPb = _isPbPb;
  cout << "nFiles: " << nFiles << endl;

//  fileSinMuW = new TFile(str1[nFiles-2].c_str(),"read");
//  fileSinMuW_LowPt = new TFile(str1[nFiles-1].c_str(),"read");
}


Eff3DMC::Eff3DMC(string str1, string str2, bool abs, bool _npmc, bool _isPbPb) {
  npmc = _npmc;
  nFiles = 1;
  inFileNames[0] = str1;
  className = str2;
  absRapidity = abs;
  isPbPb = _isPbPb;
  cout << "inFileNames: " << inFileNames[0] << endl;
  cout << "nFiles: " << nFiles << endl;
}

Eff3DMC::~Eff3DMC() {
  if (nFiles==1) {
    file->Close();
  } else {
//    fileSinMuW->Close();
//    fileSinMuW_LowPt->Close();
    delete chain;
  }

  delete h1DGenRap;
  delete h1DRecRap;
  delete h1DEffRap;
  delete h1DGenPt;
  delete h1DRecPt;
  delete h1DEffPt;
  delete h1DGenCent;
  delete h1DRecCent;
  delete h1DEffCent;

  for (int a=0; a<nbinsy2-1; a++) {
    for (int c=0; c<nbinscent2-1; c++) {
      int nidx = a*(nbinscent2-1) + c;
      delete h1DGenPtFit[nidx];
      delete h1DRecPtFit[nidx];
      delete h1DEffPtFit[nidx];
      delete f1DEffPtFit[nidx];
      delete g1DEffPtFit[nidx];
      for (int b=0; b<nbinspt2-1; b++) {
        delete h1DMeanPtFit[nidx][b];
      }
    }
  }


}

int Eff3DMC::SetTree() { 
  Reco_QQ_4mom=0, Gen_QQ_4mom=0;
  Reco_QQ_mupl_4mom=0, Gen_QQ_mupl_4mom=0;
  Reco_QQ_mumi_4mom=0, Gen_QQ_mumi_4mom=0;

  if (nFiles==1) {
    file = TFile::Open(inFileNames[0].c_str());
    tree = (TTree*)file->Get("myTree");
    tree->SetBranchAddress("Centrality",&centrality);
    tree->SetBranchAddress("HLTriggers",&HLTriggers);
    tree->SetBranchAddress("Reco_QQ_trig",Reco_QQ_trig);
    tree->SetBranchAddress("Reco_QQ_VtxProb",Reco_QQ_VtxProb);
    tree->SetBranchAddress("Reco_QQ_size",&Reco_QQ_size);
    tree->SetBranchAddress("Reco_QQ_sign",&Reco_QQ_sign);
    tree->SetBranchAddress("Reco_QQ_ctauTrue",Reco_QQ_ctauTrue);
    tree->SetBranchAddress("Reco_QQ_ctau",Reco_QQ_ctau);
    tree->SetBranchAddress("Reco_QQ_4mom",&Reco_QQ_4mom);
    tree->SetBranchAddress("Reco_QQ_mupl_4mom",&Reco_QQ_mupl_4mom);
    tree->SetBranchAddress("Reco_QQ_mumi_4mom",&Reco_QQ_mumi_4mom);
    tree->SetBranchAddress("Gen_QQ_size",&Gen_QQ_size);
    tree->SetBranchAddress("Gen_QQ_ctau",Gen_QQ_ctau);
    tree->SetBranchAddress("Gen_QQ_4mom",&Gen_QQ_4mom);
    tree->SetBranchAddress("Gen_QQ_mupl_4mom",&Gen_QQ_mupl_4mom);
    tree->SetBranchAddress("Gen_QQ_mumi_4mom",&Gen_QQ_mumi_4mom);

  } else if (nFiles > 1) {
    chain = new TChain("myTree");
    for (int i=0; i<nFiles; i++) {
      chain->Add(inFileNames[i].c_str());
    }
    chain->SetBranchAddress("Centrality",&centrality);
    chain->SetBranchAddress("HLTriggers",&HLTriggers);
    chain->SetBranchAddress("Reco_QQ_trig",Reco_QQ_trig);
    chain->SetBranchAddress("Reco_QQ_VtxProb",Reco_QQ_VtxProb);
    chain->SetBranchAddress("Reco_QQ_size",&Reco_QQ_size);
    chain->SetBranchAddress("Reco_QQ_sign",&Reco_QQ_sign);
    chain->SetBranchAddress("Reco_QQ_ctauTrue",Reco_QQ_ctauTrue);
    chain->SetBranchAddress("Reco_QQ_ctau",Reco_QQ_ctau);
    chain->SetBranchAddress("Reco_QQ_4mom",&Reco_QQ_4mom);
    chain->SetBranchAddress("Reco_QQ_mupl_4mom",&Reco_QQ_mupl_4mom);
    chain->SetBranchAddress("Reco_QQ_mumi_4mom",&Reco_QQ_mumi_4mom);
    chain->SetBranchAddress("Gen_QQ_size",&Gen_QQ_size);
    chain->SetBranchAddress("Gen_QQ_ctau",Gen_QQ_ctau);
    chain->SetBranchAddress("Gen_QQ_4mom",&Gen_QQ_4mom);
    chain->SetBranchAddress("Gen_QQ_mupl_4mom",&Gen_QQ_mupl_4mom);
    chain->SetBranchAddress("Gen_QQ_mumi_4mom",&Gen_QQ_mumi_4mom);
    cout << "nTrees: " << chain->GetNtrees() << endl;
    if (chain->GetNtrees() < 1) {
      cout << "SetTree: nFiles is less than 1. Insert \"nFiles\" larger than 0" <<endl;
      return -1;
    }
  } else {
    cout << "SetTree: nFiles is less than 1. Insert \"nFiles\" larger than 0" <<endl;
    return -1;
  }
  
  return 0;
  
}

void Eff3DMC::CreateHistos(const int _nbinsy, const double *yarray, const int _nbinsy2, const double *yarray2, const int _nbinspt, const double *ptarray, const int _nbinspt2, const double *ptarray2, const int _nbinscent, const int *centarray, const int _nbinscent2, const int *centarray2, const int _nbinsctau, const double *_ctauarray, const int _nbinsforwctau, const double *_ctauforwarray){
  nbinsy = _nbinsy;
  nbinsy2 = _nbinsy2;
  nbinspt = _nbinspt;
  nbinspt2 = _nbinspt2;
  nbinscent = _nbinscent;
  nbinscent2 = _nbinscent2;
  const double *ctauarray;

  double _ymin=yarray[0]; double _ymax=yarray[nbinsy-1];
  double _ptmin=ptarray[0]; double _ptmax=ptarray[nbinspt-1];
  int _centmin=centarray[0]; int _centmax=centarray[nbinscent-1];

  double _centarray[nbinscent];
  for (int i=0; i<nbinscent; i++) _centarray[i] = centarray[i]*2.5;

  ctauarray = _ctauarray;
  nbinsctau = _nbinsctau;
  nbinsmidctau = _nbinsctau;
  nbinsforwctau = _nbinsforwctau;

  TH1::SetDefaultSumw2();

  if (isForwardLowpT(_ymin, _ymax, _ptmin, _ptmax)) { // Less ctau bins for forward & low pT case
    ctauarray = _ctauforwarray;
    nbinsctau = nbinsforwctau;
  } else {
    ctauarray = _ctauarray;
    nbinsctau = nbinsmidctau;
  }

  h1DGenRap = new TH1D(
      Form("h1DGenRap_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";Rapidity",nbinsy-1,yarray);
  h1DRecRap = new TH1D(
      Form("h1DRecRap_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";Rapidity",nbinsy-1,yarray);
  h1DEffRap = new TH1D(
      Form("h1DEffRap_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";Rapidity",nbinsy-1,yarray);

  h1DGenPt = new TH1D(
      Form("h1DGenPt_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";p_{T} (GeV/c)",nbinspt-1,ptarray);
  h1DRecPt = new TH1D(
      Form("h1DRecPt_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";p_{T} (GeV/c)",nbinspt-1,ptarray);
  h1DEffPt = new TH1D(
      Form("h1DEffPt_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";p_{T} (GeV/c)",nbinspt-1,ptarray);

  h1DGenCent = new TH1D(
      Form("h1DGenCent_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";Centrality",nbinscent-1,_centarray);
  h1DRecCent = new TH1D(
      Form("h1DRecCent_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";Centrality",nbinscent-1,_centarray);
  h1DEffCent = new TH1D(
      Form("h1DEffCent_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";Centrality",nbinscent-1,_centarray);
  
  for (int a=0; a<nbinsy2-1; a++) {
    for (int c=0; c<nbinscent2-1; c++) {
      int nidx=a*(nbinscent2-1) + c;

      double ymin=yarray2[a]; double ymax=yarray2[a+1];
      int centmin=centarray2[c]; int centmax=centarray2[c+1];
      
      cout << "CreateHistos: nidx " <<  nidx << " " << a << " " << c << " " ;

      for (int b=0; b<nbinspt2-1; b++) {
        double ptmin=ptarray2[b]; double ptmax=ptarray2[b+1];
        cout << b << " " ;

        h1DMeanPtFit[nidx][b] = new TH1D(
            Form("h1DMeanPt_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
            ";p_{T} (GeV/c)",(ptmax-ptmin)*100,ptmin,ptmax);
        cout << h1DMeanPtFit[nidx][b] << endl;
      }

      h1DGenPtFit[nidx] = new TH1D(
          Form("h1DGenPt_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,_ptmin,_ptmax,centmin,centmax),
          ";p_{T} (GeV/c)",nbinspt2-1,ptarray2);
      h1DRecPtFit[nidx] = new TH1D(
          Form("h1DRecPt_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,_ptmin,_ptmax,centmin,centmax),
          ";p_{T} (GeV/c)",nbinspt2-1,ptarray2);
      h1DEffPtFit[nidx] = new TH1D(
          Form("h1DEffPt_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,_ptmin,_ptmax,centmin,centmax),
          ";p_{T} (GeV/c)",nbinspt2-1,ptarray2);
      f1DEffPtFit[nidx] = new TF1(Form("%s_TF",h1DEffPtFit[nidx]->GetName()),fitERF,_ptmin,_ptmax,3);

    }
  }

  cout << "CreateHistos: " << h1DEffRap << " " << h1DEffPt << " " << h1DEffCent << endl;

}

void Eff3DMC::LoopTree(const double *yarray, const double *yarray2, const double *ptarray, const double *ptarray2, const int *centarray, const int *centarray2){
  TLorentzVector *recoDiMu = new TLorentzVector;
  TLorentzVector *recoMu1 = new TLorentzVector;
  TLorentzVector *recoMu2 = new TLorentzVector;
  TLorentzVector *genMu1 = new TLorentzVector;
  TLorentzVector *genMu2 = new TLorentzVector;

  ////// Read data TTree
  int totalEvt = 0;
  if (nFiles == 1) {
    totalEvt = tree->GetEntries();
    cout << "totalEvt: " << totalEvt << endl;
  } else {
    totalEvt = chain->GetEntries();
    cout << "totalEvt: " << totalEvt << endl;
  }

  ////// Single muon efficiency weighting (RD/MC)
  string postfix[] = {"All"}; //, "0010", "1020", "2050", "50100"};
  const int ngraph = sizeof(postfix)/sizeof(string);
/*  TGraphAsymmErrors *gSingleMuW[ngraph], *gSingleMuW_LowPt[ngraph];
  for (int i=0; i<ngraph; i++) {
    // Mid-rapidity region
    gSingleMuW[i] = (TGraphAsymmErrors*)fileSinMuW->Get(Form("Trg_pt_%s_Ratio",postfix[i].c_str()));
    // Forward rapidity region
    gSingleMuW_LowPt[i] = (TGraphAsymmErrors*)fileSinMuW_LowPt->Get(Form("Trg_pt_%s_Ratio",postfix[i].c_str()));
  }*/

  TF1 *gSingleMuW[ngraph], *gSingleMuW_LowPt[ngraph];
  for (int i=0; i<ngraph; i++) {
/*    // Divide correction regions in |y|
    // Mid-rapidity region
    gSingleMuW[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
        "[0]*TMath::Erf((x-[1])/[2])/TMath::Erf((x-[3])/[4])",0,50);
    // Forward rapidity region
    gSingleMuW_LowPt[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
        "[0]*TMath::Erf((x-[1])/[2])/TMath::Erf((x-[3])/[4])",0,50);
    
    gSingleMuW[i]->SetParameters(0.9967,1.371,2.034,1.488,2.191);
    gSingleMuW_LowPt[i]->SetParameters(1.016,1.155,8.28,1.381,8.147);
*/
    // Divide correction regions in |eta|
    // Mid-rapidity region
    gSingleMuW[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
    "(0.9555*TMath::Erf((x-1.3240)/2.5683))/(0.9576*TMath::Erf((x-1.7883)/2.6583))");

    // Forward rapidity region
    gSingleMuW_LowPt[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
    "(0.8299*TMath::Erf((x-1.2785)/1.8833))/(0.7810*TMath::Erf((x-1.3609)/2.1231))");
//    "(0.8452*TMath::Erf((x-1.4262)/1.8845))/(0.7924*TMath::Erf((x-1.3808)/2.1061))");
//    "(0.8335*TMath::Erf((x-1.2470)/1.9782))/(0.7948*TMath::Erf((x-1.3091)/2.2783))");
    
  }
  ////// End of single muon efficiency weighting (RD/MC)

  double weight = 0;
//  for (int ev=0; ev<1000000; ev++) {
  for (int ev=0; ev<totalEvt; ev++) {
    if (ev%10000 == 0)
      cout << "LoopTree: " << "event # " << ev << " / " << totalEvt << endl;

    if (nFiles==1) {
      tree->GetEntry(ev);
      weight = 1;
    } else {
      if (weight != chain->GetWeight()) {
        weight = chain->GetWeight();
        cout << "New Weight: " << weight << endl;
      }   
      chain->GetEntry(ev);
    }

    for (int iRec=0; iRec<Reco_QQ_size; iRec++) {
      recoDiMu = (TLorentzVector*)Reco_QQ_4mom->At(iRec);
      recoMu1 = (TLorentzVector*)Reco_QQ_mupl_4mom->At(iRec);
      recoMu2 = (TLorentzVector*)Reco_QQ_mumi_4mom->At(iRec);
      double dm = recoDiMu->M();
      double drap = recoDiMu->Rapidity();
      double dpt = recoDiMu->Pt();
      double dctau = Reco_QQ_ctauTrue[iRec];
      double dctaureco = Reco_QQ_ctau[iRec];
      double dlxy = dctau*dpt/3.096916;
      double dlxyreco = dctaureco*dpt/3.096916;
      if ( isMuonInAccept(recoMu1) && isMuonInAccept(recoMu2) &&
           recoDiMu->M() >= 2.95 && recoDiMu->M()< 3.25 &&
           dpt >= ptarray[0] && dpt <= ptarray[nbinspt-1] &&
           Reco_QQ_sign[iRec] == 0 && Reco_QQ_VtxProb[iRec] > 0.01
         ) {

        if (absRapidity) {
          if (!(TMath::Abs(drap) >= yarray[0] && TMath::Abs(drap) <= yarray[nbinsy-1])) continue;
        } else {
          if (!(drap >= yarray[0] && drap <= yarray[nbinsy-1])) continue;
        }

        if (isPbPb) {
          if ( !((HLTriggers&1)==1 && (Reco_QQ_trig[iRec]&1)==1) ) continue;
        } else {
          if ( !((HLTriggers&2)==2 && (Reco_QQ_trig[iRec]&2)==2) ) continue;
        }

        // Get weighting factors
        double NcollWeight = findCenWeight(centrality);
        double singleMuWeight = 1;
        int ig=0;
//        if (centrality*2.5 >=0 && centrality*2.5 <10) ig=1;
//        else if (centrality*2.5 >=10 && centrality*2.5 <20) ig=2;
//        else if (centrality*2.5 >=20 && centrality*2.5 <50) ig=3;
//        else if (centrality*2.5 >=50 && centrality*2.5 <=100) ig=4;
        
/*        if (TMath::Abs(drap) < 1.6) {
          singleMuWeight = gSingleMuW[ig]->Eval(recoMu1->Pt()) * gSingleMuW[ig]->Eval(recoMu2->Pt());
//          cout << "mid-rap " << ig << " " << drap << " " << recoMu1->Pt() << " " << recoMu2->Pt() << " " << centrality << " " << singleMuWeight << endl;
        } else if (TMath::Abs(drap) >= 1.6 && TMath::Abs(drap) < 2.4) {
          singleMuWeight = gSingleMuW_LowPt[ig]->Eval(recoMu1->Pt()) * gSingleMuW_LowPt[ig]->Eval(recoMu2->Pt());
//          cout << "for-rap " << ig << " " << drap << " " << recoMu1->Pt() << " " << recoMu2->Pt() << " " << centrality << " " << singleMuWeight << endl;
        }
*/
        if (TMath::Abs(recoMu1->Eta()) < 1.6) {
          singleMuWeight = gSingleMuW[ig]->Eval(recoMu1->Pt());
        } else {
          singleMuWeight = gSingleMuW_LowPt[ig]->Eval(recoMu1->Pt());
        }

        if (TMath::Abs(recoMu2->Eta()) < 1.6) {
          singleMuWeight *= gSingleMuW[ig]->Eval(recoMu2->Pt());
        } else {
          singleMuWeight *= gSingleMuW_LowPt[ig]->Eval(recoMu2->Pt());
        }


        // Apply weights
        if (absRapidity) {
          if (isPbPb) {
            h1DRecRap->Fill(TMath::Abs(drap),weight*singleMuWeight*NcollWeight);
          } else {
            h1DRecRap->Fill(TMath::Abs(drap),singleMuWeight);
          }
        } else {
          if (isPbPb) {
            h1DRecRap->Fill(drap,weight*singleMuWeight*NcollWeight);
          } else {
            h1DRecRap->Fill(drap,singleMuWeight);
          }
        }
        if (isPbPb) {
          h1DRecPt->Fill(dpt,weight*singleMuWeight*NcollWeight);
          h1DRecCent->Fill(centrality*2.5,weight*singleMuWeight*NcollWeight);
        } else {
          h1DRecPt->Fill(dpt,singleMuWeight);
          h1DRecCent->Fill(centrality*2.5,singleMuWeight);
        }

        // 1D pT efficiency for fitting
        for (int a=0; a<nbinsy2-1; a++) {
          for (int c=0; c<nbinscent2-1; c++) {
            if (centarray2[c] <= centrality && centarray2[c+1] > centrality) {
              if (absRapidity) {
                if (!(TMath::Abs(drap) >= yarray[a] && TMath::Abs(drap) < yarray[a+1])) continue;
              } else {
                if (!(drap >= yarray[a] && drap < yarray[a+1])) continue;
              }

              int nidx = a*(nbinscent2-1) + c;
              if (isPbPb) {
                h1DRecPtFit[nidx]->Fill(dpt,weight*singleMuWeight*NcollWeight);
              } else {
                h1DRecPtFit[nidx]->Fill(dpt,singleMuWeight);
              }

              for (int b=0; b<nbinspt2-1; b++) {
                if (ptarray2[b] <= dpt && ptarray2[b+1] > dpt) {
                  if (isPbPb) {
                    h1DMeanPtFit[nidx][b]->Fill(dpt,weight*singleMuWeight*NcollWeight);
                  } else {
                    h1DMeanPtFit[nidx][b]->Fill(dpt,singleMuWeight);
                  }
                }
              }

            }
          }
        } // end of 1D pT efficiency for fitting
        
      } // end of Reco_QQ_4mom condition test
    } // end of iRec loop

    for (int iGen=0; iGen<Gen_QQ_size; iGen++) {
      genMu1 = (TLorentzVector*)Gen_QQ_mupl_4mom->At(iGen);
      genMu2 = (TLorentzVector*)Gen_QQ_mumi_4mom->At(iGen);
      TLorentzVector genDiMu = *genMu1 + *genMu2;
      double dm = genDiMu.M();
      double drap = genDiMu.Rapidity();
      double dpt = genDiMu.Pt();
      double dctau = Gen_QQ_ctau[iGen] * 10;
      double dlxy = dctau*dpt/3.096916;
      if ( isMuonInAccept(genMu1) && isMuonInAccept(genMu2) &&
           genDiMu.M() > 2 && genDiMu.M()< 4 &&
           dpt >= ptarray[0] && dpt <= ptarray[nbinspt-1] 
         ) {
        if (absRapidity) {
          if (!(TMath::Abs(drap) >= yarray[0] && TMath::Abs(drap) <= yarray[nbinsy-1])) continue;
        } else {
          if (!(drap >= yarray[0] && drap <= yarray[nbinsy-1])) continue;
        }

        double NcollWeight = findCenWeight(centrality);
        if (absRapidity) {
          if (isPbPb){
            h1DGenRap->Fill(TMath::Abs(drap),weight*NcollWeight);
          } else {
            h1DGenRap->Fill(TMath::Abs(drap));
          }
        } else {
          if (isPbPb) {
            h1DGenRap->Fill(drap,weight*NcollWeight);
          } else {
            h1DGenRap->Fill(drap);
          }
        }
        if (isPbPb) {
          h1DGenPt->Fill(dpt,weight*NcollWeight);
          h1DGenCent->Fill(centrality*2.5,weight*NcollWeight);
        } else {
          h1DGenPt->Fill(dpt);
          h1DGenCent->Fill(centrality*2.5);
        }

        for (int a=0; a<nbinsy2-1; a++) {
          for (int c=0; c<nbinscent2-1; c++) {
            if (centarray2[c] <= centrality && centarray2[c+1] > centrality) {
              if (absRapidity) {
                if (!(TMath::Abs(drap) >= yarray[a] && TMath::Abs(drap) < yarray[a+1])) continue;
              } else {
                if (!(drap >= yarray[a] && drap < yarray[a+1])) continue;
              }
              int nidx = a*(nbinscent2-1) + c;
              if (isPbPb) {
                h1DGenPtFit[nidx]->Fill(dpt,weight*NcollWeight);
              } else {
                h1DGenPtFit[nidx]->Fill(dpt);
              }
            }
          }
        }

      } // end of genDiMu condition test
    } // end of iGen loop
 
  } // end of event loop

}




void Eff3DMC::GetEfficiency(const double *raparray2, const double *ptarray2, const int *centarray2) {
  getCorrectedEffErr(nbinsy-1,h1DRecRap,h1DGenRap,h1DEffRap);
  getCorrectedEffErr(nbinspt-1,h1DRecPt,h1DGenPt,h1DEffPt);
  getCorrectedEffErr(nbinscent-1,h1DRecCent,h1DGenCent,h1DEffCent);

  for (int a=0; a<nbinsy2-1; a++) {
    for (int c=0; c<nbinscent2-1; c++) {
      int nidx = a*(nbinscent2-1) + c;
      double ymin = raparray2[a]; double ymax = raparray2[a+1];
      int centmin = centarray2[c]; int centmax = centarray2[c+1];

      cout << nidx<< " " << a << " " << c << " " << ymin << " " << ymax << " " << centmin << " " << centmax << endl;
      cout <<  h1DEffPtFit[nidx] << " " << h1DRecPtFit[nidx] << " " << h1DGenPtFit[nidx] << endl;

      getCorrectedEffErr(nbinspt2-1,h1DRecPtFit[nidx],h1DGenPtFit[nidx],h1DEffPtFit[nidx]);

      // Move pT values of histograms to <pT>
      cout << "\t\t" << h1DEffPtFit[nidx]->GetName() << endl;
      g1DEffPtFit[nidx] = new TGraphAsymmErrors(h1DEffPtFit[nidx]);
      g1DEffPtFit[nidx]->SetName(Form("%s_GASM",h1DEffPtFit[nidx]->GetName()));
      g1DEffPtFit[nidx]->GetXaxis()->SetTitle("p_{T} (GeV/c)");

      for (int b=0; b<nbinspt2-1; b++) {
        double gx, gy;
        g1DEffPtFit[nidx]->GetPoint(b,gx,gy);

        double meanpt, meanptxlerr, meanptxherr; 

        meanpt = h1DMeanPtFit[nidx][b]->GetMean();

        double ptmin = ptarray2[b]; double ptmax = ptarray2[b+1];
        cout << "(b, nidx, gx, gy) : ("<< b << ", " << nidx << ", "
             << gx << ", " << gy << ")" <<  endl;
        cout << "ptmin, ptmax, meanpt : "<< ptmin << ", " << ptmax << ", " << meanpt << "" <<  endl;
        
        meanptxlerr = meanpt - ptmin;
        meanptxherr = ptmax - meanpt;
      
        g1DEffPtFit[nidx]->SetPoint(b,meanpt,gy);
        g1DEffPtFit[nidx]->SetPointEXlow(b,meanptxlerr);
        g1DEffPtFit[nidx]->SetPointEXhigh(b,meanptxherr);
      }
      cout << endl;
      // end of move pT values of histograms to <pT>

      f1DEffPtFit[nidx]->SetParameters(0.5,4,8);
      if ( (ymin==1.6 && ymax==2.0) ||
           (ymin==2.0 && ymax==2.4) ||
           (ymin==-2.0 && ymax==-1.6) ||
           (ymin==-2.4 && ymax==-2.0)
         ){
        f1DEffPtFit[nidx]->SetParLimits(1,-10,3);
      } else {
        f1DEffPtFit[nidx]->SetParLimits(1,-10,6.5);
      }


      if (isPbPb) {
        if (npmc) {
          if (ymin==1.2 && ymax==1.6 && centmin==0 && centmax==4) {
            f1DEffPtFit[nidx]->SetParameters(0.4,2.3,8);
          } else if (ymin==1.2 && ymax==1.6 && centmin==8 && centmax==12) {
            f1DEffPtFit[nidx]->SetParameters(0.6,4,5.7);
          } else if (ymin==2.0 && ymax==2.4 && centmin==8 && centmax==16) {
            f1DEffPtFit[nidx]->SetParameters(0.4,-0.1,6.5);
          } else if (ymin==2.0 && ymax==2.4 && centmin==16 && centmax==40) {
            f1DEffPtFit[nidx]->SetParameters(0.4,0.2,5.0);
          } else if (ymin==-1.6 && ymax==-1.2 && centmin==4 && centmax==8) {
            f1DEffPtFit[nidx]->SetParameters(0.6,4,5.7);
          } else if (ymin==-2.0 && ymax==-1.6 && centmin==8 && centmax==12) {
            f1DEffPtFit[nidx]->SetParameters(0.5,2,5.7);
          } else if (ymin==-2.4 && ymax==-2.0 && centmin==0 && centmax==4) {
            f1DEffPtFit[nidx]->SetParameters(0.3,3,4.7);
          } else if (ymin==-2.4 && ymax==-2.0 && centmin==4 && centmax==8) {
            f1DEffPtFit[nidx]->SetParameters(0.3,3,4.7);
          } else if (ymin==-2.4 && ymax==-2.0 && centmin==12 && centmax==24) {
            f1DEffPtFit[nidx]->SetParameters(0.3,1.2,2.4);
          }
        } else { // PbPb PRMC
          if (ymin==0.8 && ymax==1.2 && centmin==4 && centmax==8) {
            f1DEffPtFit[nidx]->SetParameters(0.7,6.0,4.5);
          } else if (ymin==0.8 && ymax==1.2 && centmin==8 && centmax==16) {
            f1DEffPtFit[nidx]->SetParameters(0.7,6.0,4.5);
          } else if (ymin==-0.8 && ymax==0.0 && centmin==0 && centmax==4) {
            f1DEffPtFit[nidx]->SetParameters(0.7,6.0,4.5);
          } else if (ymin==2.0 && ymax==2.4 && centmin==0 && centmax==4) {
            f1DEffPtFit[nidx]->SetParameters(0.3,2.,4.5);
          } else if (ymin==2.0 && ymax==2.4 && centmin==8 && centmax==12) {
            f1DEffPtFit[nidx]->SetParameters(0.3,2.,4);
          } else if (ymin==-2.4 && ymax==-2.0 && centmin==8 && centmax==12) {
            f1DEffPtFit[nidx]->SetParameters(0.3,1.,6.0);
          } else if (ymin==-2.4 && ymax==-2.0 && centmin==16 && centmax==40) {
            f1DEffPtFit[nidx]->SetParameters(0.4,2.5,2.4);
          }
        }
      } else { // pp 
        if (ymin==1.6 && ymax==2.0) {
//          f1DEffPtFit[nidx]->SetParLimits(1,-10,3);
        }
        if (ymin==-2.4 && ymax==-2.0) {
          f1DEffPtFit[nidx]->SetParameters(0.33,0.39,7.0);
        } else if (ymin==2 && ymax==2.4) {
          f1DEffPtFit[nidx]->SetParameters(0.33,0.90,5.5);
        }
      }
      
      TFitResultPtr res = g1DEffPtFit[nidx]->Fit(Form("%s_TF",h1DEffPtFit[nidx]->GetName()),"R S");
      int counter=0;
      if (0 != res->Status()) {
        while (1) {
          counter++;
          res = g1DEffPtFit[nidx]->Fit(Form("%s_TF",h1DEffPtFit[nidx]->GetName()),"R S");
          if (0 == res->Status() || counter > 20) break;
        }
      }
     

    }
  }

}
 
void Eff3DMC::SaveHistos(string str) {
  outFileName = str;
  outfile = new TFile(outFileName.c_str(),"recreate");
  outfile->cd();

  h1DGenRap->Write();
  h1DRecRap->Write();
  h1DEffRap->Write();
  h1DGenPt->Write();
  h1DRecPt->Write();
  h1DEffPt->Write();
  h1DGenCent->Write();
  h1DRecCent->Write();
  h1DEffCent->Write();

  for (int a=0; a<nbinsy2-1; a++) {
    for (int c=0; c<nbinscent2-1; c++) {
      int nidx = a*(nbinscent2-1) + c;
      h1DGenPtFit[nidx]->Write();
      h1DRecPtFit[nidx]->Write();
      h1DEffPtFit[nidx]->Write();
      f1DEffPtFit[nidx]->Write();
      g1DEffPtFit[nidx]->Write();
      for (int b=0; b<nbinspt2-1; b++) {
        h1DMeanPtFit[nidx][b]->Write();
      }
    }
  }

  outfile->Close();
}






class EffMC {
  private:
    bool absRapidity, npmc, isPbPb;

    string inFileNames[100], className, outFileName;
    TFile *file, *inFile[100], *outfile, *fileSinMuW, *fileSinMuW_LowPt;
    TTree *tree;
    TChain *chain;
    int nFiles;
    
    int centrality, HLTriggers, Reco_QQ_trig[100], Reco_QQ_sign[100];
    int Reco_QQ_size, Gen_QQ_size;
    float Reco_QQ_ctau[100], Reco_QQ_ctauTrue[100], Gen_QQ_ctau[100], Reco_QQ_VtxProb[100];
    TClonesArray *Reco_QQ_4mom, *Gen_QQ_4mom;
    TClonesArray *Reco_QQ_mupl_4mom, *Gen_QQ_mupl_4mom;
    TClonesArray *Reco_QQ_mumi_4mom, *Gen_QQ_mumi_4mom;
    
    int nbinsy, nbinspt, nbinsctau, nbinsmidctau, nbinsforwctau, nbinscent, nbinsresol;
    double resolmin, resolmax;

    // Lxy histo
    TH3D *hGenLxy[20], *hRecLxy[20], *hEffLxy[20];
    TH1D *hGenLxyDiff[200], *hGenLxyRap[20], *hGenLxyPt[20], *hGenLxyCent[20], *hGenLxyA;
    TH1D *hRecLxyDiff[200], *hRecLxyRap[20], *hRecLxyPt[20], *hRecLxyCent[20], *hRecLxyA;
    TH1D *hEffLxyDiff[200], *hEffLxyRap[20], *hEffLxyPt[20], *hEffLxyCent[20], *hEffLxyA;
    TH1D *hresolLxyRap[20], *hresolLxyPt[20], *hresolLxyCent[20], *hresolLxyA;
    TH1D *hMeanLxy[200][30];

    // l_Jpsi histo
    TH3D *hGen[20], *hRec[20], *hEff[20];
    TH1D *hGenDiff[200], *hGenRap[20], *hGenPt[20], *hGenCent[20], *hGenA;
    TH1D *hRecDiff[200], *hRecRap[20], *hRecPt[20], *hRecCent[20], *hRecA;
    TH1D *hEffDiff[200], *hEffRap[20], *hEffPt[20], *hEffCent[20], *hEffA;
    TH1D *hresolRap[20], *hresolPt[20], *hresolCent[20], *hresolA;
    

  public:
    EffMC(int _nFiles, string str1[], string str2, bool abs, bool npmc, bool isPbPb);
    EffMC(string, string, bool, bool, bool);
    ~EffMC();
    int SetTree();
    void CreateHistos(const int _nbinsy, const double *yarray, const int _nbinspt, const double *ptarray, const int _nbinscent, const int *centarray, const int _nbinsctau, const double *_ctauarray, const int _nbinsforwctau, const double *_ctauforwarray, const int _nbinsresol, const double _resolmin, const double _resolmax);
    void LoopTree(const double *yarray, const double *ptarray, const int *centarray, const double *_ctauarray, const double *_ctauforwarray);
    void GetEfficiency();
    void SaveHistos(string str, const double *yarray, const double *ptarray, const int *centarray);
};

EffMC::EffMC(int _nFiles, string str1[], string str2, bool abs, bool _npmc, bool _isPbPb) {
  npmc = _npmc;
  nFiles = _nFiles;
  for (int i=0; i<nFiles; i++) {
    inFileNames[i] = str1[i];
    cout << "inFileNames[" << i << "]: " << inFileNames[i] << endl;
  }
  className = str2;
  absRapidity = abs;
  isPbPb = _isPbPb;
  cout << "nFiles: " << nFiles << endl;
  
//  fileSinMuW = new TFile(str1[nFiles-2].c_str(),"read");
//  fileSinMuW_LowPt = new TFile(str1[nFiles-1].c_str(),"read");
}


EffMC::EffMC(string str1, string str2, bool abs, bool _npmc, bool _isPbPb) {
  npmc = _npmc;
  nFiles = 1;
  inFileNames[0] = str1;
  className = str2;
  absRapidity = abs;
  isPbPb = _isPbPb;
  cout << "inFileNames: " << inFileNames[0] << endl;
  cout << "nFiles: " << nFiles << endl;
}

EffMC::~EffMC() {
  if (nFiles==1) {
    file->Close();
  } else {
//    fileSinMuW->Close();
//    fileSinMuW_LowPt->Close();
    delete chain;
  }
  
  delete hGenLxyA;
  delete hRecLxyA;
  delete hEffLxyA;

  delete hGenA;
  delete hRecA;
  delete hEffA;


  for (int a=0; a<nbinsy-1; a++) {
    for (int b=0; b<nbinspt-1; b++) {
      for (int c=0; c<nbinscent-1; c++) {
        int nidx = a*(nbinspt-1)*(nbinscent-1) + b*(nbinscent-1) + c;
        delete hGenLxyDiff[nidx];
        delete hRecLxyDiff[nidx];
        delete hEffLxyDiff[nidx];
        delete hGenDiff[nidx];
        delete hRecDiff[nidx];
        delete hEffDiff[nidx];

        for (int d=0; d<nbinsmidctau-1; d++) {
          if (!hMeanLxy[nidx][d]) delete hMeanLxy[nidx][d];
        }

      }
    }
  }

  for (int i=0; i<nbinscent-1; i++) {
    delete hGenLxy[i];
    delete hRecLxy[i];
    delete hEffLxy[i];
    delete hGen[i];
    delete hRec[i];
    delete hEff[i];
  }

  for (int i=0; i<nbinsy-1; i++) {
    delete hGenLxyRap[i];
    delete hRecLxyRap[i];
    delete hEffLxyRap[i];
    delete hGenRap[i];
    delete hRecRap[i];
    delete hEffRap[i];
  }
  for (int i=0; i<nbinspt-1; i++) {
    delete hGenLxyPt[i];
    delete hRecLxyPt[i];
    delete hEffLxyPt[i];
    delete hGenPt[i];
    delete hRecPt[i];
    delete hEffPt[i];
  }
  for (int i=0; i<nbinscent-1; i++) {
    delete hGenLxyCent[i];
    delete hRecLxyCent[i];
    delete hEffLxyCent[i];
    delete hGenCent[i];
    delete hRecCent[i];
    delete hEffCent[i];
  }

}

int EffMC::SetTree() { 
  Reco_QQ_4mom=0, Gen_QQ_4mom=0;
  Reco_QQ_mupl_4mom=0, Gen_QQ_mupl_4mom=0;
  Reco_QQ_mumi_4mom=0, Gen_QQ_mumi_4mom=0;

  if (nFiles==1) {
    file = TFile::Open(inFileNames[0].c_str());
    tree = (TTree*)file->Get("myTree");
    tree->SetBranchAddress("Centrality",&centrality);
    tree->SetBranchAddress("HLTriggers",&HLTriggers);
    tree->SetBranchAddress("Reco_QQ_trig",Reco_QQ_trig);
    tree->SetBranchAddress("Reco_QQ_VtxProb",Reco_QQ_VtxProb);
    tree->SetBranchAddress("Reco_QQ_size",&Reco_QQ_size);
    tree->SetBranchAddress("Reco_QQ_sign",&Reco_QQ_sign);
    tree->SetBranchAddress("Reco_QQ_ctauTrue",Reco_QQ_ctauTrue);
    tree->SetBranchAddress("Reco_QQ_ctau",Reco_QQ_ctau);
    tree->SetBranchAddress("Reco_QQ_4mom",&Reco_QQ_4mom);
    tree->SetBranchAddress("Reco_QQ_mupl_4mom",&Reco_QQ_mupl_4mom);
    tree->SetBranchAddress("Reco_QQ_mumi_4mom",&Reco_QQ_mumi_4mom);
    tree->SetBranchAddress("Gen_QQ_size",&Gen_QQ_size);
    tree->SetBranchAddress("Gen_QQ_ctau",Gen_QQ_ctau);
    tree->SetBranchAddress("Gen_QQ_4mom",&Gen_QQ_4mom);
    tree->SetBranchAddress("Gen_QQ_mupl_4mom",&Gen_QQ_mupl_4mom);
    tree->SetBranchAddress("Gen_QQ_mumi_4mom",&Gen_QQ_mumi_4mom);

  } else if (nFiles > 1) {
    chain = new TChain("myTree");
    for (int i=0; i<nFiles; i++) {
      chain->Add(inFileNames[i].c_str());
    }
    chain->SetBranchAddress("Centrality",&centrality);
    chain->SetBranchAddress("HLTriggers",&HLTriggers);
    chain->SetBranchAddress("Reco_QQ_trig",Reco_QQ_trig);
    chain->SetBranchAddress("Reco_QQ_VtxProb",Reco_QQ_VtxProb);
    chain->SetBranchAddress("Reco_QQ_size",&Reco_QQ_size);
    chain->SetBranchAddress("Reco_QQ_sign",&Reco_QQ_sign);
    chain->SetBranchAddress("Reco_QQ_ctauTrue",Reco_QQ_ctauTrue);
    chain->SetBranchAddress("Reco_QQ_ctau",Reco_QQ_ctau);
    chain->SetBranchAddress("Reco_QQ_4mom",&Reco_QQ_4mom);
    chain->SetBranchAddress("Reco_QQ_mupl_4mom",&Reco_QQ_mupl_4mom);
    chain->SetBranchAddress("Reco_QQ_mumi_4mom",&Reco_QQ_mumi_4mom);
    chain->SetBranchAddress("Gen_QQ_size",&Gen_QQ_size);
    chain->SetBranchAddress("Gen_QQ_ctau",Gen_QQ_ctau);
    chain->SetBranchAddress("Gen_QQ_4mom",&Gen_QQ_4mom);
    chain->SetBranchAddress("Gen_QQ_mupl_4mom",&Gen_QQ_mupl_4mom);
    chain->SetBranchAddress("Gen_QQ_mumi_4mom",&Gen_QQ_mumi_4mom);
    cout << "nTrees: " << chain->GetNtrees() << endl;
    if (chain->GetNtrees() < 1) {
      cout << "SetTree: nFiles is less than 1. Insert \"nFiles\" larger than 0" <<endl;
      return -1;
    }
  } else {
    cout << "SetTree: nFiles is less than 1. Insert \"nFiles\" larger than 0" <<endl;
    return -1;
  }
  
  return 0;
  
}

void EffMC::CreateHistos(const int _nbinsy, const double *yarray, const int _nbinspt, const double *ptarray, const int _nbinscent, const int *centarray, const int _nbinsctau, const double *_ctauarray, const int _nbinsforwctau, const double *_ctauforwarray, const int _nbinsresol, const double _resolmin, const double _resolmax){
  nbinsy = _nbinsy;
  nbinspt = _nbinspt;
  nbinscent = _nbinscent;
  nbinsresol = _nbinsresol;
  resolmin = _resolmin;
  resolmax = _resolmax;
  const double *ctauarray;

  double _ymin=yarray[0]; double _ymax=yarray[nbinsy-1];
  double _ptmin=ptarray[0]; double _ptmax=ptarray[nbinspt-1];
  int _centmin=centarray[0]; int _centmax=centarray[nbinscent-1];

  ctauarray = _ctauarray;
  nbinsctau = _nbinsctau;
  nbinsmidctau = _nbinsctau;
  nbinsforwctau = _nbinsforwctau;

  TH1::SetDefaultSumw2();

  hGenLxyA = new TH1D(
      Form("hGenLxy1D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
  hRecLxyA = new TH1D(
      Form("hRecLxy1D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
  hEffLxyA = new TH1D(
      Form("hEffLxy1D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);

  hGenA = new TH1D(
      Form("hGen1D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
  hRecA = new TH1D(
      Form("hRec1D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
  hEffA = new TH1D(
      Form("hEff1D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);

  hresolA = new TH1D(
      Form("hresol_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";(#font[12]{l}_{J/#psi} - #font[12]{l}_{J/#psi}(true)) / #font[12]{l}_{J/#psi}(true)",nbinsresol,resolmin,resolmax);
  hresolLxyA = new TH1D(
      Form("hresolLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax),
      ";(L_{xy} - L_{xy}(true)) / L_{xy}(true)",nbinsresol,resolmin,resolmax);

  for (int a=0; a<nbinsy-1; a++) {
    for (int b=0; b<nbinspt-1; b++) {
      for (int c=0; c<nbinscent-1; c++) {
        double ymin=yarray[a]; double ymax=yarray[a+1];
        double ptmin=ptarray[b]; double ptmax=ptarray[b+1];
        int centmin=centarray[c]; int centmax=centarray[c+1];
        int nidx = a*(nbinspt-1)*(nbinscent-1) + b*(nbinscent-1) + c;

        if (isForwardLowpT(ymin, ymax, ptmin, ptmax)) { // Less ctau bins for forward & low pT case
          ctauarray = _ctauforwarray;
          nbinsctau = nbinsforwctau;
        } else {
          ctauarray = _ctauarray;
          nbinsctau = nbinsmidctau;
        }

        hGenLxyDiff[nidx] = new TH1D(
            Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
            ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
        hRecLxyDiff[nidx] = new TH1D(
            Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
            ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
        hEffLxyDiff[nidx] = new TH1D(
            Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
            ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
        hGenDiff[nidx] = new TH1D(
            Form("hGen_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
            ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
        hRecDiff[nidx] = new TH1D(
            Form("hRec_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
            ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
        hEffDiff[nidx] = new TH1D(
            Form("hEff_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
            ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);

        for (int d=0; d<nbinsctau-1; d++) {
            cout << "CreateHistos: nidx " <<  nidx << " " << a << " " << b << " " << c << " " << d << " ";
          hMeanLxy[nidx][d] = new TH1D(
            Form("hMeanLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d_Lxy%.1f-%.1f",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax,ctauarray[d],ctauarray[d+1])
            ,";L_{xy} (Reco) (mm)",(ctauarray[d+1]-ctauarray[d])/100,ctauarray[d],ctauarray[d+1]);
          cout << hMeanLxy[nidx][d] << endl;
        }

      }
    }
  }


  for (int i=0; i<nbinscent-1; i++) {
    int centmin=centarray[i]; int centmax=centarray[i+1];

    hGenLxy[i] = new TH3D(
        Form("hGenLxy3D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,centmin,centmax),
        ";Rapidity;p_{T} (GeV/c);L_{xy} (true) (mm)",nbinsy-1,yarray,nbinspt-1,ptarray,nbinsctau-1,ctauarray);
    hRecLxy[i] = new TH3D(
        Form("hRecLxy3D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,centmin,centmax),
        ";Rapidity;p_{T} (GeV/c);L_{xy} (true) (mm)",nbinsy-1,yarray,nbinspt-1,ptarray,nbinsctau-1,ctauarray);
    hEffLxy[i] = new TH3D(
        Form("hEffLxy3D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,centmin,centmax),
        ";Rapidity;p_{T} (GeV/c);L_{xy} (true) (mm)",nbinsy-1,yarray,nbinspt-1,ptarray,nbinsctau-1,ctauarray);

    hGen[i] = new TH3D(
        Form("hGen3D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,centmin,centmax),
        ";Rapidity;p_{T} (GeV/c);#font[12]{l}_{J/#psi} (true) (mm)",nbinsy-1,yarray,nbinspt-1,ptarray,nbinsctau-1,ctauarray);
    hRec[i] = new TH3D(
        Form("hRec3D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,centmin,centmax),
        ";Rapidity;p_{T} (GeV/c);#font[12]{l}_{J/#psi} (true) (mm)",nbinsy-1,yarray,nbinspt-1,ptarray,nbinsctau-1,ctauarray);
    hEff[i] = new TH3D(
        Form("hEff3D_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),_ymin,_ymax,_ptmin,_ptmax,centmin,centmax),
        ";Rapidity;p_{T} (GeV/c);#font[12]{l}_{J/#psi} (true) (mm)",nbinsy-1,yarray,nbinspt-1,ptarray,nbinsctau-1,ctauarray);
  }

  for (int i=0; i<nbinsy-1; i++) {
    double ymin=yarray[i]; double ymax=yarray[i+1];
    double ptmin=ptarray[0]; double ptmax=ptarray[nbinspt-1];
    int centmin=centarray[0]; int centmax=centarray[nbinscent-1];

    if (isForwardLowpT(ymin, ymax, ptmin, ptmax)) { // Less ctau bins for forward & low pT case
      ctauarray = _ctauforwarray;
      nbinsctau = nbinsforwctau;
    } else {
      ctauarray = _ctauarray;
      nbinsctau = nbinsmidctau;
    }

    hGenLxyRap[i] = new TH1D(
        Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
    hRecLxyRap[i] = new TH1D(
        Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
    hEffLxyRap[i] = new TH1D(
        Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);

    hGenRap[i] = new TH1D(
        Form("hGen_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
    hRecRap[i] = new TH1D(
        Form("hRec_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
    hEffRap[i] = new TH1D(
        Form("hEff_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
    
    hresolRap[i] = new TH1D(
        Form("hresol_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";(#font[12]{l}_{J/#psi} - #font[12]{l}_{J/#psi}(true)) / #font[12]{l}_{J/#psi}(true)",nbinsresol,resolmin,resolmax);
    hresolLxyRap[i] = new TH1D(
        Form("hresolLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";(L_{xy} - L_{xy}(true)) / L_{xy}(true)",nbinsresol,resolmin,resolmax);
    
  }

  for (int i=0; i<nbinspt-1; i++) {
    double ymin=yarray[0]; double ymax=yarray[nbinsy-1];
    double ptmin=ptarray[i]; double ptmax=ptarray[i+1];
    int centmin=centarray[0]; int centmax=centarray[nbinscent-1];

    if (isForwardLowpT(ymin, ymax, ptmin, ptmax)) { // Less ctau bins for forward & low pT case
      ctauarray = _ctauforwarray;
      nbinsctau = nbinsforwctau;
    } else {
      ctauarray = _ctauarray;
      nbinsctau = nbinsmidctau;
    }

    hGenLxyPt[i] = new TH1D(
        Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
    hRecLxyPt[i] = new TH1D(
        Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
    hEffLxyPt[i] = new TH1D(
        Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);

    hGenPt[i] = new TH1D(
        Form("hGen_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
    hRecPt[i] = new TH1D(
        Form("hRec_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
    hEffPt[i] = new TH1D(
        Form("hEff_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
 
    hresolPt[i] = new TH1D(
        Form("hresol_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";(#font[12]{l}_{J/#psi} - #font[12]{l}_{J/#psi}(true)) / #font[12]{l}_{J/#psi}(true)",nbinsresol,resolmin,resolmax);
    hresolLxyPt[i] = new TH1D(
        Form("hresolLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";(L_{xy} - L_{xy}(true)) / L_{xy}(true)",nbinsresol,resolmin,resolmax);
  }

  for (int i=0; i<nbinscent-1; i++) {
    double ymin=yarray[0]; double ymax=yarray[nbinsy-1];
    double ptmin=ptarray[0]; double ptmax=ptarray[nbinspt-1];
    int centmin=centarray[i]; int centmax=centarray[i+1];

    if (isForwardLowpT(ymin, ymax, ptmin, ptmax)) { // Less ctau bins for forward & low pT case
      ctauarray = _ctauforwarray;
      nbinsctau = nbinsforwctau;
    } else {
      ctauarray = _ctauarray;
      nbinsctau = nbinsmidctau;
    }

    hGenLxyCent[i] = new TH1D(
        Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
    hRecLxyCent[i] = new TH1D(
        Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);
    hEffLxyCent[i] = new TH1D(
        Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";L_{xy} (true) (mm)",nbinsctau-1,ctauarray);

    hGenCent[i] = new TH1D(
        Form("hGen_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
    hRecCent[i] = new TH1D(
        Form("hRec_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
    hEffCent[i] = new TH1D(
        Form("hEff_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";#font[12]{l}_{J/#psi} (true) (mm)",nbinsctau-1,ctauarray);
 
    hresolCent[i] = new TH1D(
        Form("hresol_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";(#font[12]{l}_{J/#psi} - #font[12]{l}_{J/#psi}(true)) / #font[12]{l}_{J/#psi}(true)",nbinsresol,resolmin,resolmax);
    hresolLxyCent[i] = new TH1D(
        Form("hresolLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax),
        ";(L_{xy} - L_{xy}(true)) / L_{xy}(true)",nbinsresol,resolmin,resolmax);
  }

  cout << "CreateHistos: " << hGen << " " << hRec << " " << hEff << endl;
  cout << "CreateHistos: " << hEffRap[0] << " " << hEffPt[0] << endl;
}

void EffMC::LoopTree(const double *yarray, const double *ptarray, const int *centarray, const double *_ctauarray, const double *_ctauforwarray){
  TLorentzVector *recoDiMu = new TLorentzVector;
  TLorentzVector *recoMu1 = new TLorentzVector;
  TLorentzVector *recoMu2 = new TLorentzVector;
  TLorentzVector *genMu1 = new TLorentzVector;
  TLorentzVector *genMu2 = new TLorentzVector;

  ////// Read data TTree
  int totalEvt = 0;
  if (nFiles == 1) {
    totalEvt = tree->GetEntries();
    cout << "totalEvt: " << totalEvt << endl;
  } else {
    totalEvt = chain->GetEntries();
    cout << "totalEvt: " << totalEvt << endl;
  }

  ////// Single muon efficiency weighting (RD/MC)
  string postfix[] = {"All", "0010", "1020", "2050", "50100"};
  const int ngraph = sizeof(postfix)/sizeof(string);
/*  TGraphAsymmErrors *gSingleMuW[ngraph], *gSingleMuW_LowPt[ngraph];
  for (int i=0; i<ngraph; i++) {
    // Mid-rapidity region
    gSingleMuW[i] = (TGraphAsymmErrors*)fileSinMuW->Get(Form("Trg_pt_%s_Ratio",postfix[i].c_str()));
    // Forward rapidity region
    gSingleMuW_LowPt[i] = (TGraphAsymmErrors*)fileSinMuW_LowPt->Get(Form("Trg_pt_%s_Ratio",postfix[i].c_str()));
  }*/

  TF1 *gSingleMuW[ngraph], *gSingleMuW_LowPt[ngraph];
  for (int i=0; i<ngraph; i++) {
/*    // Divide correction regions in |y|
    // Mid-rapidity region
    gSingleMuW[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
        "[0]*TMath::Erf((x-[1])/[2])/TMath::Erf((x-[3])/[4])",0,50);
    // Forward rapidity region
    gSingleMuW_LowPt[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
        "[0]*TMath::Erf((x-[1])/[2])/TMath::Erf((x-[3])/[4])",0,50);
    
    gSingleMuW[i]->SetParameters(0.9967,1.371,2.034,1.488,2.191);
    gSingleMuW_LowPt[i]->SetParameters(1.016,1.155,8.28,1.381,8.147);
*/

    // Divide correction regions in |eta|
    // Mid-rapidity region
    gSingleMuW[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
    "(0.9555*TMath::Erf((x-1.3240)/2.5683))/(0.9576*TMath::Erf((x-1.7883)/2.6583))");

    // Forward rapidity region
    gSingleMuW_LowPt[i] = new TF1(Form("Trg_pt_%s_Ratio",postfix[i].c_str()),
    "(0.8299*TMath::Erf((x-1.2785)/1.8833))/(0.7810*TMath::Erf((x-1.3609)/2.1231))");

  }
  ////// End of single muon efficiency weighting (RD/MC)

  double weight = 0;
//  for (int ev=0; ev<1000000; ev++) {
  for (int ev=0; ev<totalEvt; ev++) {
    if (ev%10000 == 0)
      cout << "LoopTree: " << "event # " << ev << " / " << totalEvt << endl;

    if (nFiles==1) {
      tree->GetEntry(ev);
      weight = 1;
    } else {
      if (weight != chain->GetWeight()) {
        weight = chain->GetWeight();
        cout << "New Weight: " << weight << endl;
      }   
      chain->GetEntry(ev);
    }

    const double *ctauarray;
    for (int iRec=0; iRec<Reco_QQ_size; iRec++) {
      recoDiMu = (TLorentzVector*)Reco_QQ_4mom->At(iRec);
      recoMu1 = (TLorentzVector*)Reco_QQ_mupl_4mom->At(iRec);
      recoMu2 = (TLorentzVector*)Reco_QQ_mumi_4mom->At(iRec);
      double drap = recoDiMu->Rapidity();
      double dpt = recoDiMu->Pt();
      double dctau = Reco_QQ_ctauTrue[iRec];
      double dctaureco = Reco_QQ_ctau[iRec];
      double dlxy = dctau*dpt/3.096916;
      double dlxyreco = dctaureco*dpt/3.096916;
      if ( isMuonInAccept(recoMu1) && isMuonInAccept(recoMu2) &&
           recoDiMu->M() > 2.95 && recoDiMu->M()< 3.25 &&
           dpt >= ptarray[0] && dpt <= ptarray[nbinspt-1] &&
           Reco_QQ_sign[iRec] == 0 && Reco_QQ_VtxProb[iRec] > 0.01
         ) {

        if (absRapidity) {
          if (!(TMath::Abs(drap) >= yarray[0] && TMath::Abs(drap) <= yarray[nbinsy-1])) continue;
        } else {
          if (!(drap >= yarray[0] && drap <= yarray[nbinsy-1])) continue;
        }
        if (isPbPb) {
          if ( !((HLTriggers&1)==1 && (Reco_QQ_trig[iRec]&1)==1)) continue;
        } else {
          if ( !((HLTriggers&2)==2 && (Reco_QQ_trig[iRec]&2)==2)) continue;
        }

        // Get weighting factors
        double NcollWeight = findCenWeight(centrality);
        double singleMuWeight = 1;
        int ig=0;
//        if (centrality*2.5 >=0 && centrality*2.5 <10) ig=1;
//        else if (centrality*2.5 >=10 && centrality*2.5 <20) ig=2;
//        else if (centrality*2.5 >=20 && centrality*2.5 <50) ig=3;
//        else if (centrality*2.5 >=50 && centrality*2.5 <=100) ig=4;
        
/*        if (TMath::Abs(drap) < 1.6) {
          singleMuWeight = gSingleMuW[ig]->Eval(recoMu1->Pt()) * gSingleMuW[ig]->Eval(recoMu2->Pt());
//          cout << "mid-rap " << ig << " " << drap  << " " << recoMu1->Pt() << " " << recoMu2->Pt() << " " << centrality << " " << singleMuWeight << endl;
        } else if (TMath::Abs(drap) >= 1.6 && TMath::Abs(drap) < 2.4) {
          singleMuWeight = gSingleMuW_LowPt[ig]->Eval(recoMu1->Pt()) * gSingleMuW_LowPt[ig]->Eval(recoMu2->Pt());
//          cout << "for-rap " << ig << " " << drap << " " << recoMu1->Pt() << " " << recoMu2->Pt() << " " << centrality << " " << singleMuWeight << endl;
        }
*/
        if (TMath::Abs(recoMu1->Eta()) < 1.6) {
          singleMuWeight = gSingleMuW[ig]->Eval(recoMu1->Pt());
        } else {
          singleMuWeight = gSingleMuW_LowPt[ig]->Eval(recoMu1->Pt());
        }

        if (TMath::Abs(recoMu2->Eta()) < 1.6) {
          singleMuWeight *= gSingleMuW[ig]->Eval(recoMu2->Pt());
        } else {
          singleMuWeight *= gSingleMuW_LowPt[ig]->Eval(recoMu2->Pt());
        }

        // Differential histograms & Apply weights
        for (int a=0; a<nbinsy-1; a++) {
          for (int b=0; b<nbinspt-1; b++) {
            for (int c=0; c<nbinscent-1; c++) {
              int nidx = a*(nbinspt-1)*(nbinscent-1) + b*(nbinscent-1) + c;
              
              if (isForwardLowpT(yarray[a], yarray[a+1], ptarray[b], ptarray[b+1])) { // Less ctau bins for forward & low pT case
                ctauarray = _ctauforwarray;
                nbinsctau = nbinsforwctau;
              } else {
                ctauarray = _ctauarray;
                nbinsctau = nbinsmidctau;
              }

              if ( (ptarray[b] <= dpt && ptarray[b+1] > dpt) &&
                   (centarray[c] <= centrality && centarray[c+1] > centrality)
                 ) {

                if (absRapidity) {
                  if (!(TMath::Abs(drap) >= yarray[a] && TMath::Abs(drap) < yarray[a+1])) continue;
                } else {
                  if (!(drap >= yarray[a] && drap < yarray[a+1])) continue;
                }

                if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
                
                if (isPbPb) {
                  hRecDiff[nidx]->Fill(dctau,weight*singleMuWeight*NcollWeight);
                  hRecLxyDiff[nidx]->Fill(dlxy,weight*singleMuWeight*NcollWeight);
                } else {
                  hRecDiff[nidx]->Fill(dctau,singleMuWeight);
                  hRecLxyDiff[nidx]->Fill(dlxy,singleMuWeight);
                }
                  
                for (int d=0; d<nbinsctau-1; d++) {
                  if (dlxy >= ctauarray[d] && dlxy < ctauarray[d+1]) {
                    if (isPbPb) hMeanLxy[nidx][d]->Fill(dlxy,weight*NcollWeight);
                    else hMeanLxy[nidx][d]->Fill(dlxy,singleMuWeight);
                  }
                }

              } // end of rap & pT & centrality binning
            } // end of c loop
          } // end of b loop
        } // end of a loop
        
        // Fill 1D rap, pt, cent histograms
        for (int i=0; i<nbinsy-1; i++) {
          if (absRapidity) {
            if (!(TMath::Abs(drap) >= yarray[i] && TMath::Abs(drap) < yarray[i+1])) continue;
          } else {
            if (!(drap >= yarray[i] && drap < yarray[i+1])) continue;
          }
          if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
          if (isPbPb) {
            hRecLxyRap[i]->Fill(dlxy,weight*singleMuWeight*NcollWeight);
            hRecRap[i]->Fill(dctau,weight*singleMuWeight*NcollWeight);
            hresolLxyRap[i]->Fill((dlxyreco-dlxy)/dlxy,weight*singleMuWeight*NcollWeight);
            hresolRap[i]->Fill((dctaureco-dctau)/dctau,weight*singleMuWeight*NcollWeight);
          } else {
            hRecLxyRap[i]->Fill(dlxy*singleMuWeight);
            hRecRap[i]->Fill(dctau*singleMuWeight);
            hresolLxyRap[i]->Fill((dlxyreco-dlxy)/dlxy*singleMuWeight);
            hresolRap[i]->Fill((dctaureco-dctau)/dctau*singleMuWeight);
          }
        }
        for (int i=0; i<nbinspt-1; i++) {
          if (ptarray[i] <= dpt && ptarray[i+1] > dpt) {
            if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
            if (isPbPb) {
              hRecLxyPt[i]->Fill(dlxy,weight*singleMuWeight*NcollWeight);
              hRecPt[i]->Fill(dctau,weight*singleMuWeight*NcollWeight);
              hresolLxyPt[i]->Fill((dlxyreco-dlxy)/dlxy,weight*singleMuWeight*NcollWeight);
              hresolPt[i]->Fill((dctaureco-dctau)/dctau,weight*singleMuWeight*NcollWeight);
            } else {
              hRecLxyPt[i]->Fill(dlxy*singleMuWeight);
              hRecPt[i]->Fill(dctau*singleMuWeight);
              hresolLxyPt[i]->Fill((dlxyreco-dlxy)/dlxy*singleMuWeight);
              hresolPt[i]->Fill((dctaureco-dctau)/dctau*singleMuWeight);
            }
          }
        }
        for (int i=0; i<nbinscent-1; i++) {
          if (centarray[i] <= centrality && centarray[i+1] > centrality) {
            if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
            if (isPbPb) {
              hRecLxyCent[i]->Fill(dlxy,weight*singleMuWeight*NcollWeight);
              hRecCent[i]->Fill(dctau,weight*singleMuWeight*NcollWeight);
              hresolLxyCent[i]->Fill((dlxyreco-dlxy)/dlxy,weight*singleMuWeight*NcollWeight);
              hresolCent[i]->Fill((dctaureco-dctau)/dctau,weight*singleMuWeight*NcollWeight);
            } else {
              hRecLxyCent[i]->Fill(dlxy,singleMuWeight);
              hRecCent[i]->Fill(dctau,singleMuWeight);
              hresolLxyCent[i]->Fill((dlxyreco-dlxy)/dlxy,singleMuWeight);
              hresolCent[i]->Fill((dctaureco-dctau)/dctau,singleMuWeight);
            }
            if (absRapidity) {
              if (isPbPb) {
                hRec[i]->Fill(TMath::Abs(drap),dpt,dctau,weight*singleMuWeight*NcollWeight);
                hRecLxy[i]->Fill(TMath::Abs(drap),dpt,dlxy,weight*singleMuWeight*NcollWeight);
              } else {
                hRec[i]->Fill(TMath::Abs(drap),dpt,dctau,singleMuWeight);
                hRecLxy[i]->Fill(TMath::Abs(drap),dpt,dlxy,singleMuWeight);
              }
            } else {
              if (isPbPb) {
                hRec[i]->Fill(drap,dpt,dctau,weight*singleMuWeight*NcollWeight);
                hRecLxy[i]->Fill(drap,dpt,dlxy,weight*singleMuWeight*NcollWeight);
              } else {
                hRec[i]->Fill(drap,dpt,dctau,singleMuWeight);
                hRecLxy[i]->Fill(drap,dpt,dlxy,singleMuWeight);
              }
            }
          }
        }
        if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
        if (isPbPb) {
          hresolLxyA->Fill((dlxyreco-dlxy)/dlxy,weight*singleMuWeight*NcollWeight);
          hresolA->Fill((dctaureco-dctau)/dctau,weight*singleMuWeight*NcollWeight);
          hRecLxyA->Fill(dlxy,weight*singleMuWeight*NcollWeight);
          hRecA->Fill(dctau,weight*singleMuWeight*NcollWeight);
        } else {
          hresolLxyA->Fill((dlxyreco-dlxy)/dlxy,singleMuWeight);
          hresolA->Fill((dctaureco-dctau)/dctau,singleMuWeight);
          hRecLxyA->Fill(dlxy,singleMuWeight);
          hRecA->Fill(dctau,singleMuWeight);
        }
      } // end of Reco_QQ_4mom condition test
    } // end of iRec loop

    for (int iGen=0; iGen<Gen_QQ_size; iGen++) {
      genMu1 = (TLorentzVector*)Gen_QQ_mupl_4mom->At(iGen);
      genMu2 = (TLorentzVector*)Gen_QQ_mumi_4mom->At(iGen);
      TLorentzVector genDiMu = *genMu1 + *genMu2;
      double drap = genDiMu.Rapidity();
      double dpt = genDiMu.Pt();
      double dctau = Gen_QQ_ctau[iGen] * 10;
      double dlxy = dctau*dpt/3.096916;
      if ( isMuonInAccept(genMu1) && isMuonInAccept(genMu2) &&
           genDiMu.M() > 2 && genDiMu.M()< 4 &&
           dpt >= ptarray[0] && dpt <= ptarray[nbinspt-1] 
         ) {
        if (absRapidity) {
          if (!(TMath::Abs(drap) >= yarray[0] && TMath::Abs(drap) <= yarray[nbinsy-1])) continue;
        } else {
          if (!(drap >= yarray[0] && drap <= yarray[nbinsy-1])) continue;
        }

        double NcollWeight = findCenWeight(centrality);
        // Differential histograms
        for (int a=0; a<nbinsy-1; a++) {
          for (int b=0; b<nbinspt-1; b++) {
            for (int c=0; c<nbinscent-1; c++) {
              int nidx = a*(nbinspt-1)*(nbinscent-1) + b*(nbinscent-1) + c;
              if ( (ptarray[b] <= dpt && ptarray[b+1] > dpt) &&
                   (centarray[c] <= centrality && centarray[c+1] > centrality)
                 ) {
                if (absRapidity) {
                  if (!(TMath::Abs(drap) >= yarray[a] && TMath::Abs(drap) < yarray[a+1])) continue;
                } else {
                  if (!(drap >= yarray[a] && drap < yarray[a+1])) continue;
                }
                if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
                if (isPbPb) {
                  hGenDiff[nidx]->Fill(dctau,weight*NcollWeight);
                  hGenLxyDiff[nidx]->Fill(dlxy,weight*NcollWeight);
                } else {
                  hGenDiff[nidx]->Fill(dctau);
                  hGenLxyDiff[nidx]->Fill(dlxy);
                }
              }
            }
          }
        }

        for (int i=0; i<nbinsy-1; i++) {
          if (absRapidity) {
            if (!(TMath::Abs(drap) >= yarray[i] && TMath::Abs(drap) < yarray[i+1])) continue;
          } else {
            if (!(drap >= yarray[i] && drap < yarray[i+1])) continue;
          }
          if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
          if (isPbPb) {
            hGenRap[i]->Fill(dctau,weight*NcollWeight);
            hGenLxyRap[i]->Fill(dlxy,weight*NcollWeight);
          } else {
            hGenRap[i]->Fill(dctau);
            hGenLxyRap[i]->Fill(dlxy);
          }
        }
        for (int i=0; i<nbinspt-1; i++) {
          if (ptarray[i] <= dpt && ptarray[i+1] > dpt) {
            if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
            if (isPbPb) {
              hGenPt[i]->Fill(dctau,weight*NcollWeight);
              hGenLxyPt[i]->Fill(dlxy,weight*NcollWeight);
            } else {
              hGenPt[i]->Fill(dctau);
              hGenLxyPt[i]->Fill(dlxy);
            }
          }
        }
        for (int i=0; i<nbinscent-1; i++) {
          if (centarray[i] <= centrality && centarray[i+1] > centrality) {
            if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
            if (isPbPb) {
              hGenCent[i]->Fill(dctau,weight*NcollWeight);
              hGenLxyCent[i]->Fill(dlxy,weight*NcollWeight);
            } else {
              hGenCent[i]->Fill(dctau);
              hGenLxyCent[i]->Fill(dlxy);
            }
            if (absRapidity) {
              if (isPbPb) {
                hGen[i]->Fill(TMath::Abs(drap),dpt,dctau,weight*NcollWeight);
                hGenLxy[i]->Fill(TMath::Abs(drap),dpt,dlxy,weight*NcollWeight);
              } else {
                hGen[i]->Fill(TMath::Abs(drap),dpt,dctau);
                hGenLxy[i]->Fill(TMath::Abs(drap),dpt,dlxy);
              }
            } else {
              if (isPbPb) {
                hGen[i]->Fill(drap,dpt,dctau,weight*NcollWeight);
                hGenLxy[i]->Fill(drap,dpt,dlxy,weight*NcollWeight);
              } else {
                hGen[i]->Fill(drap,dpt,dctau);
                hGenLxy[i]->Fill(drap,dpt,dlxy);
              }
            }
          }
        }
        if (!npmc && (dlxy > 0.1 || dctau > 0.1)) continue;
        if (isPbPb) {
          hGenA->Fill(dctau,weight*NcollWeight);
          hGenLxyA->Fill(dlxy,weight*NcollWeight);
        } else {
          hGenA->Fill(dctau);
          hGenLxyA->Fill(dlxy);
        }
      } // end of genDiMu condition test
    } // end of iGen loop
 
  } // end of event loop

}

void EffMC::GetEfficiency() {
  ////// Get Efficiency numbers
  getCorrectedEffErr(nbinsctau-1,hRecLxyA,hGenLxyA,hEffLxyA);
  getCorrectedEffErr(nbinsctau-1,hRecA,hGenA,hEffA);

  for (int a=0; a<nbinsy-1; a++) {
    for (int b=0; b<nbinspt-1; b++) {
      for (int c=0; c<nbinscent-1; c++) {
        int nidx = a*(nbinspt-1)*(nbinscent-1) + b*(nbinscent-1) + c;
        getCorrectedEffErr(nbinsctau-1,hRecDiff[nidx],hGenDiff[nidx],hEffDiff[nidx]);
        getCorrectedEffErr(nbinsctau-1,hRecLxyDiff[nidx],hGenLxyDiff[nidx],hEffLxyDiff[nidx]);
      }
    }
  }
  for (int i=0; i<nbinsy-1; i++) {
    getCorrectedEffErr(nbinsctau-1,hRecRap[i],hGenRap[i],hEffRap[i]);
    getCorrectedEffErr(nbinsctau-1,hRecLxyRap[i],hGenLxyRap[i],hEffLxyRap[i]);
  }
  for (int i=0; i<nbinspt-1; i++) {
    getCorrectedEffErr(nbinsctau-1,hRecPt[i],hGenPt[i],hEffPt[i]);
    getCorrectedEffErr(nbinsctau-1,hRecLxyPt[i],hGenLxyPt[i],hEffLxyPt[i]);
  }
  for (int i=0; i<nbinscent-1; i++) {
    getCorrectedEffErr(nbinsctau-1,hRecCent[i],hGenCent[i],hEffCent[i]);
    getCorrectedEffErr(nbinsctau-1,hRecLxyCent[i],hGenLxyCent[i],hEffLxyCent[i]);
    hEff[i]->Divide(hRec[i],hGen[i]);
    hEffLxy[i]->Divide(hRecLxy[i],hGenLxy[i]);
  }

}
 
void EffMC::SaveHistos(string str, const double *yarray, const double *ptarray, const int *centarray) {
  outFileName = str;
  outfile = new TFile(outFileName.c_str(),"recreate");
  outfile->cd();

  hGenLxyA->Write();
  hRecLxyA->Write();
  hEffLxyA->Write();

  hresolA->Write();
  hresolLxyA->Write();

  hGenA->Write();
  hRecA->Write();
  hEffA->Write();

  for (int a=0; a<nbinsy-1; a++) {
    for (int b=0; b<nbinspt-1; b++) {
      for (int c=0; c<nbinscent-1; c++) {
        int nidx = a*(nbinspt-1)*(nbinscent-1) + b*(nbinscent-1) + c;
        
        hEffDiff[nidx]->Write();
        hEffLxyDiff[nidx]->Write();

        if (isForwardLowpT(yarray[a], yarray[a+1], ptarray[b], ptarray[b+1])) { // Less ctau bins for forward & low pT case
          nbinsctau = nbinsforwctau;
        } else {
          nbinsctau = nbinsmidctau;
        }

        for (int d=0; d<nbinsctau-1; d++) {
          hMeanLxy[nidx][d]->Write();
        }

      }
    }
  }

  for (int i=0; i<nbinsy-1; i++) {
    hGenLxyRap[i]->Write();
    hRecLxyRap[i]->Write();
    hEffLxyRap[i]->Write();

    hresolRap[i]->Write();
    hresolLxyRap[i]->Write();

    hGenRap[i]->Write();
    hRecRap[i]->Write();
    hEffRap[i]->Write();
  }
  for (int i=0; i<nbinspt-1; i++) {
    hGenLxyPt[i]->Write();
    hRecLxyPt[i]->Write();
    hEffLxyPt[i]->Write();

    hresolPt[i]->Write();
    hresolLxyPt[i]->Write();

    hGenPt[i]->Write();
    hRecPt[i]->Write();
    hEffPt[i]->Write();
  }
  for (int i=0; i<nbinscent-1; i++) {
    hGenLxyCent[i]->Write();
    hRecLxyCent[i]->Write();
    hEffLxyCent[i]->Write();

    hGenCent[i]->Write();
    hRecCent[i]->Write();
    hEffCent[i]->Write();
    
    hresolCent[i]->Write();
    hresolLxyCent[i]->Write();

    hGenLxy[i]->Write();
    hRecLxy[i]->Write();
    hEffLxy[i]->Write();

    hGen[i]->Write();
    hRec[i]->Write();
    hEff[i]->Write();
  }
  outfile->Close();
}

std::pair< string, string > FillLatexInfo(double ymin, double ymax, double ptmin, double ptmax, bool absRapidity) {
  double tmpyh,tmpyl;
  if (ymin > ymax) {
    tmpyh = ymin;
    tmpyl = ymax;
    ymin = tmpyl;
    ymax = tmpyh;
  }

  double ptminD, ptminF, ptmaxD, ptmaxF;
  double yminD, yminF, ymaxD, ymaxF;
  ptminF = modf(ptmin,&ptminD);
  ptmaxF = modf(ptmax,&ptmaxD);
  yminF = modf(ymin,&yminD);
  ymaxF = modf(ymax,&ymaxD);
  string ptstr, rapstr;
  char testStr[1024];

  if (ptmin == 0) {
    if (ptmaxF != 0) sprintf(testStr,"p_{T} < %.1f GeV/c",ptmax);
    else sprintf(testStr,"p_{T} < %.0f GeV/c",ptmax);
  } else {
    if (ptminF == 0 && ptmaxF == 0) sprintf(testStr,"%.0f < p_{T} < %.0f GeV/c",ptmin,ptmax);
    else if (ptminF == 0 && ptmaxF != 0) sprintf(testStr,"%.0f < p_{T} < %.1f GeV/c",ptmin,ptmax);
    else if (ptminF != 0 && ptmaxF == 0) sprintf(testStr,"%.1f < p_{T} < %.0f GeV/c",ptmin,ptmax);
    else sprintf(testStr,"%.1f < p_{T} < %.1f GeV/c",ptmin,ptmax);
  }
  ptstr = testStr;

  if (absRapidity){
    if (ymin==0.0) {
      if (ymaxF != 0) sprintf(testStr,"|y| < %.1f",ymax);
      else sprintf(testStr,"|y| < %.0f",ymax);
    } else {
      if (yminF == 0 && ymaxF == 0) sprintf(testStr,"%.0f < |y| < %.0f",ymin,ymax);
      else if (yminF == 0 && ymaxF != 0) sprintf(testStr,"%.0f < |y| < %.1f",ymin,ymax);
      else if (yminF != 0 && ymaxF == 0) sprintf(testStr,"%.1f < |y| < %.0f",ymin,ymax);
      else sprintf(testStr,"%.1f < |y| < %.1f",ymin,ymax);
    }
  } else {
    if (ymin==0.0) {
      if (ymaxF != 0) sprintf(testStr,"%.0f < y < %.1f",ymin,ymax);
      else sprintf(testStr,"%.0f < y < %.0f",ymin,ymax);
    } else {
      if (yminF == 0 && ymaxF == 0) sprintf(testStr,"%.0f < y < %.0f",ymin,ymax);
      else if (yminF == 0 && ymaxF != 0) sprintf(testStr,"%.0f < y < %.1f",ymin,ymax);
      else if (yminF != 0 && ymaxF == 0) sprintf(testStr,"%.1f < y < %.0f",ymin,ymax);
      else sprintf(testStr,"%.1f < y < %.1f",ymin,ymax);
    }
  }
  rapstr = testStr;

  std::pair< string, string > result = std::make_pair(ptstr, rapstr);
  return result;
}

void SetGraphStyle(TGraph *h, int i, int j, double rmin, double rmax){
  int colorArr[] = {kRed+1, kOrange+7, kSpring+4, kGreen+3, kAzure+1, kBlue+2, kViolet+5, kViolet-4, kMagenta, kMagenta+2};
  int markerArr[] = {kOpenCircle, kOpenSquare, kOpenStar, kOpenTriangleUp, 32, 33};
  
  h->GetYaxis()->SetRangeUser(rmin,rmax);

  h->SetMarkerSize(1.200);
  if (j < 2) h->SetMarkerSize(1.000);
  if (j == 2) h->SetMarkerSize(1.700);
  if (j == 5) h->SetMarkerSize(1.500);
  
  h->SetMarkerColor(colorArr[i]);
  h->SetLineColor(colorArr[i]);
  h->SetMarkerStyle(markerArr[j]);

  h->GetXaxis()->SetTitleSize(0.048);
  h->GetYaxis()->SetTitleSize(0.048);
  h->GetXaxis()->SetLabelSize(0.048);
  h->GetYaxis()->SetLabelSize(0.048);
}

void SetHistStyle(TGraph *h, int i, int j, double rmin, double rmax){
  int colorArr[] = {kRed+1, kOrange+7, kSpring+4, kGreen+3, kAzure+1, kBlue+2, kViolet+5, kViolet-4, kMagenta, kMagenta+2};
  int markerArr[] = {kOpenCircle, kOpenSquare, kOpenStar, kOpenTriangleUp, 32, 33};
  
  h->GetYaxis()->SetRangeUser(rmin,rmax);

  h->SetMarkerSize(1.200);
  if (j < 2) h->SetMarkerSize(1.000);
  if (j == 2) h->SetMarkerSize(1.700);
  if (j == 5) h->SetMarkerSize(1.500);
  
  h->SetMarkerColor(colorArr[i]);
  h->SetLineColor(colorArr[i]);
  h->SetMarkerStyle(markerArr[j]);

  h->GetXaxis()->SetTitleSize(0.048);
  h->GetYaxis()->SetTitleSize(0.048);
  h->GetXaxis()->SetLabelSize(0.048);
  h->GetYaxis()->SetLabelSize(0.048);
}

void SetHistStyle(TH1 *h, int i, int j, double rmin, double rmax){
  int colorArr[] = {kRed+1, kOrange+7, kSpring+4, kGreen+3, kAzure+1, kBlue+2, kViolet+5, kViolet-4, kMagenta, kMagenta+2};
  int markerArr[] = {kOpenCircle, kOpenSquare, kOpenStar, kOpenTriangleUp, 32, 33};
  
  h->GetYaxis()->SetRangeUser(rmin,rmax);

  h->SetMarkerSize(1.200);
  if (j < 2) h->SetMarkerSize(1.000);
  if (j == 2) h->SetMarkerSize(1.700);
  if (j == 5) h->SetMarkerSize(1.500);
  
  h->SetMarkerColor(colorArr[i]);
  h->SetLineColor(colorArr[i]);
  h->SetMarkerStyle(markerArr[j]);

  h->SetTitleSize(0.048,"XYZ");
  h->SetLabelSize(0.048,"XYZ");
}

void SetLegendStyle(TLegend* l) {
  l->SetFillColor(0);
  l->SetFillStyle(4000);
  l->SetBorderSize(0);
  l->SetMargin(0.15);
}
