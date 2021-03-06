#include <iostream>
#include <math.h>
#include <fstream>
#include <utility>
#include <map>
#include <stdexcept>

#include <TROOT.h>
#include <TFile.h>
#include <TVector3.h>
#include "TH1F.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGraphAsymmErrors.h"
#include "TF1.h"
#include "TF2.h"
#include "TFitResult.h"
#include "TLorentzVector.h"
#include "TRandom3.h"
#include "TClonesArray.h"
#include "TChain.h"
#include "TLatex.h"

#include "RooFit.h"
#include "RooDataSet.h"
#include "RooRealVar.h"
#include "RooCategory.h"
#include "RooPlot.h"

#include <TCanvas.h>
#include "TStyle.h"
#include "TPaletteAxis.h"


//-1: etHFp+etHFm combined without auto-correlation
//-2: etHFp+etHFm combined with enhanced auto-correlation
//-3: *NOT* flattening + etHFp+etHFm combined without auto-correlation
//otherwise, indicates event plane method
static int RPNUM = -1;

//0: Nominal (one of the mu trig), 1: bit1 & Cowboy, 2: bit1 & Sailor, 3: bit1, 4: bit2
//6: one of the single mu trig, 7: (one of the single mu trig) & cowboy, 8: (one of the single mu trig) & sailor
//9: (one of the single mu trig) && (one of the double mu trig)
//10: bit1,2,4 (HLT_HIL1DoubleMu0_NHitQ || HLT_HIL2DoubleMu3 || HLT_HIL3DoubleMuOpen_Mgt2_OS_NoCowboy)
static int trigType = 3;

//runType can be combined with trigType
//0: Don't use this option (default!!) 
//1: nMuValHits > 12, 2: select J/psi with |single mu_eta| < 1.2, 3: |zVtx| < 10 cm
//4: numOfMatch > 1 with a J/psi mass closest dimuon per event
//5: numOfMatch > 2 with a J/psi mass closest dimuon per event
//6: various single muon cuts for both single muons
//7: both single mu pT > 4
//81: random number < 0.5 
//82: random number >=0.5
//9: Jpsi_dPhi with J/psi phi in [-pi, pi] range
static int runType = 0;

//0 : DO NOT weight, 1: Apply weight
static bool doWeighting = false;

/////// These are not input arguments
//0 : use Lxy/ctau for lifetime, 1: use Lxyz/ctau3D for lifetime
static bool use3DCtau = true;

//0: not apply tnp correction factor, 1: apply tnp correction factor(old)
//2: apply tnp correction factor(new: STA * (muID+trig) ) (default)
//3: apply tnp correction factor(new: muID+trig)
static int useTnPCorr = 2;

//0: not apply Lxyz efficiency, 1: apply Lxyz efficiency with feffLxy
//2: apply Lxyz efficiency with heffLxy (default)
static int useLxyzCorr = 2;

//0: not use y-pT efficiency map & 1D pT fit over 3-6.5, 6.5-30 GeV/c
//1: use y-pT efficiency map, 2: use y-pT eff map only in forward region
//3: not use y-pT efficiency map & 1D pT fit over 3-30 GeV/c (default)
//4: use y-pT eff map only in forward & low-pT region
//5: same as 3, but use TH1D instead of TF1
//6: same as 3, but use TGraphAsymmErrors instead of TF1
//7: same as 3, but use toyMC(min integrated eff) - v2 systematics
//8: same as 3, but use toyMC(max integrated eff) - v2 systematics
static int useRapPtEff = 3;

//0: apply ratio for 3D and 4D eff (default), 1: apply difference for 3D and 4D eff
static bool useEffDiff = false;
/////// End of non-input-arguments

//0: don't care about RPAng, 1: Pick events with RPAng != -10
static bool checkRPNUM = false;

static const double Jpsi_MassMin=2.6;
static const double Jpsi_MassMax=3.5;
static const double Jpsi_PtMin=3.0;
static const double Jpsi_PtMax=30;
static const double Jpsi_YMin=0;
static const double Jpsi_YMax=2.4;
static const double Jpsi_CtMin = -50.0;
static const double Jpsi_CtMax = 50.0;
static const double Jpsi_CtErrMin = 0.0;
static const double Jpsi_CtErrMax = 1.0;
static const double Jpsi_PhiMin=-3.14159265359;
static const double Jpsi_PhiMax=3.14159265359;
static const double Jpsi_dPhiMin=Jpsi_PhiMin*2;
static const double Jpsi_dPhiMax=Jpsi_PhiMax*2;

using namespace RooFit;
using namespace std;

static const double PDGJpsiM = 3.096916;
const bool isPbPb = true;

// Binning for pT efficiency curves
const int centarr[] = {0, 4, 8, 16, 40};
const int centforwarr[] = {0, 4, 8, 16, 40};
//const int centarr[] = {0, 40};
//const int centforwarr[] = {0, 40};
const double ptarr[] = {6.5, 7.5, 9.0, 11, 13, 16, 30.0};
const double ptforwarr[] = {3.0, 4.5, 6.0, 7.5, 9.0, 11, 13, 16, 30.0};
const double raparr[] = {-1.6, -1.2, -0.8, 0.0, 0.8, 1.2, 1.6};
const double rapforwarr[] = {-2.4, -2.0, -1.6, 1.6, 2.0, 2.4};
const unsigned int nCentArr = sizeof(centarr)/sizeof(int) -1;
const unsigned int nCentForwArr = sizeof(centforwarr)/sizeof(int) -1;
const unsigned int nPtArr = sizeof(ptarr)/sizeof(double) -1;
const unsigned int nPtForwArr = sizeof(ptforwarr)/sizeof(double) -1;
const unsigned int nRapArr = sizeof(raparr)/sizeof(double) -1;
const unsigned int nRapForwArr = sizeof(rapforwarr)/sizeof(double) -1;
const unsigned int nHistEff = nCentArr * nPtArr * nRapArr;
const unsigned int nHistForwEff = nCentForwArr * nPtForwArr * nRapForwArr;

// Binning for Lxy efficiency curves
const int _centarr[] = {0, 40};
const int _centforwarr[] = {0, 40};
const double _ptarr[] = {6.5, 7.5, 8.5, 9.5, 11, 13, 16, 30};
const double _ptforwarr[] = {3.0, 5.5, 6.5, 8.5, 11, 16, 30};
const double _raparr[] = {-1.6, -1.2, -0.8, 0.0, 0.8, 1.2, 1.6};
const double _rapforwarr[] = {-2.4, -2.0, -1.6, 1.6, 2.0, 2.4};
const unsigned int _nCentArr = sizeof(_centarr)/sizeof(int) -1;
const unsigned int _nCentForwArr = sizeof(_centforwarr)/sizeof(int) -1;
const unsigned int _nPtArr = sizeof(_ptarr)/sizeof(double) -1;
const unsigned int _nPtForwArr = sizeof(_ptforwarr)/sizeof(double) -1;
const unsigned int _nRapArr = sizeof(_raparr)/sizeof(double) -1;
const unsigned int _nRapForwArr = sizeof(_rapforwarr)/sizeof(double) -1;
const unsigned int _nHistEff = _nCentArr * _nPtArr * _nRapArr;
const unsigned int _nHistForwEff = _nCentForwArr * _nPtForwArr * _nRapForwArr;

TH1D *heffPt[nRapArr * nCentArr], *heffPt_LowPt[nRapForwArr * nCentForwArr], *heffPt_ForwHighPt[nRapForwArr * nCentForwArr];
TGraphAsymmErrors *geffPt[nRapArr * nCentArr], *geffPt_LowPt[nRapForwArr * nCentForwArr], *geffPt_ForwHighPt[nRapForwArr * nCentForwArr];
TF1 *feffPt[nRapArr * nCentArr], *feffPt_LowPt[nRapForwArr * nCentForwArr], *feffPt_ForwHighPt[nRapForwArr * nCentForwArr];
TF2 *feffRapPt[2*nCentArr], *feffRapPt_LowPt[2*nCentForwArr], *feffRapPt_ForwHighPt[2*nCentForwArr];
TF1 *feffLxy[_nHistEff], *feffLxy_LowPt[_nHistEff];
TH1D *heffLxy[_nHistEff], *heffLxy_LowPt[_nHistEff];
TH1D *heffEmpty[nRapArr * nCentArr], *heffEmpty_LowPt[nRapArr * nCentArr];
TH1D *heffCentCow[nHistEff], *heffCentCow_LowPt[nHistEff];
TH1D *heffCentSai[nHistEff], *heffCentSai_LowPt[nHistEff];

TH2D *hLxyCtau[nHistEff], *hLxyCtau_LowPt[nHistEff];
TH2D *hLxyCtau2[10];

string fitfunc_min[nRapArr * nCentArr] = {
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent0-4_GASM
"106.481*TMath::Erf((x--14.8372)/10.4122)+-105.858",
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent4-8_GASM
"31.5088*TMath::Erf((x--55.0034)/34.8251)+-30.7247",
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent8-16_GASM
"4.74156*TMath::Erf((x--9.84849)/12.6077)+-4.07249",
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent16-40_GASM
"142.653*TMath::Erf((x--34.8581)/19.4314)+-141.94",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent0-4_GASM
"0.908312*TMath::Erf((x-4.23279)/6.62284)+-0.146621",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent4-8_GASM
"46.0973*TMath::Erf((x--20.9959)/15.4311)+-45.2913",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent8-16_GASM
"325.037*TMath::Erf((x--19.8335)/11.8007)+-324.273",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent16-40_GASM
"1.49*TMath::Erf((x-1.45279)/7.30568)+-0.711975",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent0-4_GASM
"214.816*TMath::Erf((x--17.5747)/11.5187)+-214.05",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent4-8_GASM
"214.91*TMath::Erf((x--20.9471)/13.1092)+-214.11",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent8-16_GASM
"325.635*TMath::Erf((x--25.2187)/14.7121)+-324.815",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent16-40_GASM
"69.5525*TMath::Erf((x--14.3849)/11.5003)+-68.7436",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent0-4_GASM
"3.68862*TMath::Erf((x--0.608145)/7.68558)+-2.90104",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent4-8_GASM
"0.370499*TMath::Erf((x-8.14597)/3.92333)+0.407311",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent8-16_GASM
"6.44023*TMath::Erf((x--5.2901)/9.85714)+-5.64613",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent16-40_GASM
"265.769*TMath::Erf((x--19.7875)/12.4549)+-264.96",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent0-4_GASM
"2.1171*TMath::Erf((x-0.709647)/7.46813)+-1.37915",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent4-8_GASM
"251.133*TMath::Erf((x--19.4843)/11.7765)+-250.41",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent8-16_GASM
"1.33608*TMath::Erf((x-2.9251)/6.0507)+-0.59628",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent16-40_GASM
"112.28*TMath::Erf((x--23.0723)/14.8772)+-111.503",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent0-4_GASM
"0.586483*TMath::Erf((x-3.93084)/7.29001)+0.0393527",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent4-8_GASM
"145.837*TMath::Erf((x--43.1395)/23.0171)+-145.151",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent8-16_GASM
"348.199*TMath::Erf((x--32.9351)/16.8395)+-347.564",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent16-40_GASM
"332.898*TMath::Erf((x--34.7199)/17.7434)+-332.237"
};

string fitfunc_max[nRapArr * nCentArr] = {
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent0-4_GASM
"106.471*TMath::Erf((x--7.90353)/7.2643)+-105.872",
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent4-8_GASM
"31.4744*TMath::Erf((x--23.1478)/17.1367)+-30.7856",
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent8-16_GASM
"44.6761*TMath::Erf((x--21.1208)/15.1788)+-43.9846",
//h1DEffPt_PRJpsi_Rap-1.6--1.2_Pt6.5-30.0_Cent16-40_GASM
"142.643*TMath::Erf((x--28.3422)/16.6171)+-141.95",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent0-4_GASM
"51.7438*TMath::Erf((x--13.3069)/11.3823)+-51.0132",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent4-8_GASM
"46.0836*TMath::Erf((x--11.7728)/10.6469)+-45.317",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent8-16_GASM
"325.025*TMath::Erf((x--12.7688)/8.86425)+-324.286",
//h1DEffPt_PRJpsi_Rap-1.2--0.8_Pt6.5-30.0_Cent16-40_GASM
"1.50883*TMath::Erf((x-2.62771)/6.5082)+-0.723883",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent0-4_GASM
"214.817*TMath::Erf((x--16.0025)/11.0044)+-214.05",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent4-8_GASM
"214.908*TMath::Erf((x--17.4207)/11.6933)+-214.114",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent8-16_GASM
"325.624*TMath::Erf((x--19.0573)/12.0085)+-324.827",
//h1DEffPt_PRJpsi_Rap-0.8-0.0_Pt6.5-30.0_Cent16-40_GASM
"69.5456*TMath::Erf((x--10.7857)/9.67685)+-68.7543",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent0-4_GASM
"3.68158*TMath::Erf((x--0.410094)/7.70849)+-2.91272",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent4-8_GASM
"3.56861*TMath::Erf((x-0.382273)/6.99949)+-2.79101",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent8-16_GASM
"6.45003*TMath::Erf((x--2.32508)/7.90458)+-5.66054",
//h1DEffPt_PRJpsi_Rap0.0-0.8_Pt6.5-30.0_Cent16-40_GASM
"265.771*TMath::Erf((x--21.2022)/13.1459)+-264.958",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent0-4_GASM
"2.14525*TMath::Erf((x-1.99332)/6.62147)+-1.38278",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent4-8_GASM
"251.137*TMath::Erf((x--15.6179)/10.3774)+-250.407",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent8-16_GASM
"0.715567*TMath::Erf((x-6.10271)/3.72643)+-0.00279617",
//h1DEffPt_PRJpsi_Rap0.8-1.2_Pt6.5-30.0_Cent16-40_GASM
"112.293*TMath::Erf((x--17.3989)/12.3514)+-111.495",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent0-4_GASM
"0.205921*TMath::Erf((x-9.36133)/3.10009)+0.383222",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent4-8_GASM
"145.851*TMath::Erf((x--42.1658)/23.4133)+-145.137",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent8-16_GASM
"348.197*TMath::Erf((x--22.4351)/12.7214)+-347.567",
//h1DEffPt_PRJpsi_Rap1.2-1.6_Pt6.5-30.0_Cent16-40_GASM
"332.887*TMath::Erf((x--23.9643)/13.3694)+-332.249"
};

string fitfunc_min_LowPt[nRapForwArr * nCentForwArr] = {
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent0-4_GASM
"0.169765*TMath::Erf((x-4.68277)/4.55015)+0.162752",
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent4-8_GASM
"0.20068*TMath::Erf((x-5.19599)/5.68531)+0.177815",
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent8-16_GASM
"0.185565*TMath::Erf((x-4.65544)/2.70228)+0.193694",
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent16-40_GASM
"0.182279*TMath::Erf((x-5.02509)/1.67875)+0.222152",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent0-4_GASM
"0.249954*TMath::Erf((x-5.97662)/3.70536)+0.20895",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent4-8_GASM
"0.306223*TMath::Erf((x-4.69931)/4.82626)+0.174838",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent8-16_GASM
"0.316677*TMath::Erf((x-4.50426)/4.69977)+0.176749",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent16-40_GASM
"0.237422*TMath::Erf((x-4.9129)/1.79721)+0.236304",
//Rap-1.6-1.6_Pt3.0-30.0_Cent0-4 : needs to be skipped
"x",
//Rap-1.6-1.6_Pt3.0-30.0_Cent4-8 : needs to be skipped
"x",
//Rap-1.6-1.6_Pt3.0-30.0_Cent8-16 : needs to be skipped
"x",
//Rap-1.6-1.6_Pt3.0-30.0_Cent16-40 : needs to be skipped
"x",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent0-4_GASM
"0.700598*TMath::Erf((x--0.397142)/10.1278)+-0.203814",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent4-8_GASM
"0.261241*TMath::Erf((x-5.24936)/4.29254)+0.210498",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent8-16_GASM
"0.413606*TMath::Erf((x-3.01139)/6.60264)+0.0834922",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent16-40_GASM
"0.303841*TMath::Erf((x-4.36867)/3.42211)+0.195678",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent0-4_GASM
"0.757192*TMath::Erf((x--11.4894)/19.4769)+-0.406281",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent4-8_GASM
"0.16448*TMath::Erf((x-5.19729)/2.12894)+0.167299",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent8-16_GASM
"0.262167*TMath::Erf((x-3.07265)/6.38892)+0.134741",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent16-40_GASM
"0.228193*TMath::Erf((x-2.8978)/4.35902)+0.1494"
};

string fitfunc_max_LowPt[nRapForwArr * nCentForwArr] = {
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent0-4_GASM
"0.160934*TMath::Erf((x-5.51027)/3.56603)+0.165625",
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent4-8_GASM
"0.173965*TMath::Erf((x-6.76239)/3.12455)+0.216076",
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent8-16_GASM
"0.191761*TMath::Erf((x-5.02068)/3.23016)+0.192953",
//h1DEffPt_PRJpsi_Rap-2.4--2.0_Pt3.0-30.0_Cent16-40_GASM
"0.17836*TMath::Erf((x-4.72263)/2.11809)+0.22865",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent0-4_GASM
"0.264057*TMath::Erf((x-5.72519)/4.38311)+0.210879",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent4-8_GASM
"0.334264*TMath::Erf((x-4.69525)/6.36364)+0.171674",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent8-16_GASM
"0.261883*TMath::Erf((x-5.80821)/3.59948)+0.227803",
//h1DEffPt_PRJpsi_Rap-2.0--1.6_Pt3.0-30.0_Cent16-40_GASM
"0.241573*TMath::Erf((x-5.22982)/1.85577)+0.241418",
//Rap-1.6-1.6_Pt3.0-30.0_Cent0-4 : needs to be skipped
"x",
//Rap-1.6-1.6_Pt3.0-30.0_Cent4-8 : needs to be skipped
"x",
//Rap-1.6-1.6_Pt3.0-30.0_Cent8-16 : needs to be skipped
"x",
//Rap-1.6-1.6_Pt3.0-30.0_Cent16-40 : needs to be skipped
"x",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent0-4_GASM
"0.207192*TMath::Erf((x-6.43589)/1.59396)+0.227105",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent4-8_GASM
"0.210173*TMath::Erf((x-6.26577)/2.13962)+0.232887",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent8-16_GASM
"0.255617*TMath::Erf((x-5.73705)/3.24177)+0.22751",
//h1DEffPt_PRJpsi_Rap1.6-2.0_Pt3.0-30.0_Cent16-40_GASM
"0.260821*TMath::Erf((x-5.36486)/2.68753)+0.231312",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent0-4_GASM
"0.14597*TMath::Erf((x-6.8138)/3.35437)+0.178964",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent4-8_GASM
"0.152551*TMath::Erf((x-5.67714)/0.734688)+0.177915",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent8-16_GASM
"0.189454*TMath::Erf((x-5.56577)/3.13749)+0.200219",
//h1DEffPt_PRJpsi_Rap2.0-2.4_Pt3.0-30.0_Cent16-40_GASM
"0.193597*TMath::Erf((x-4.39757)/2.76911)+0.191164"
};

// Variables for a dimuon
struct Condition {
  double theMass, theRapidity, thePt, theP, theCentrality;
  double thePhi, thedPhi, thedPhi22, thedPhi23;
  double vprob, theCt, theCtErr, Lxyz, zVtx, theEff;
  int HLTriggers, Reco_QQ_trig, theCat,Jq;
  int mupl_nMuValHits, mumi_nMuValHits;
  int mupl_numOfMatch, mumi_numOfMatch;
  int mupl_nTrkHits, mumi_nTrkHits;
  int mupl_nTrkWMea, mumi_nTrkWMea;
  double mupl_norChi2_inner, mumi_norChi2_inner, mupl_norChi2_global, mumi_norChi2_global;
  double theCtTrue, genType;
} ;

bool checkTriggers(const struct Condition Jpsi, bool cowboy, bool sailor);
bool checkRunType(const struct Condition Jpsi, const TLorentzVector* m1P, const TLorentzVector* m2P);
bool isAccept(const TLorentzVector* aMuon);
bool isMuonInAccept(const TLorentzVector *aMuon);
double reducedPhi(double thedPhi);


double fitERF(double *x, double *par) {
  return par[0]*TMath::Erf((x[0]-par[1])/par[2]);
}


int main(int argc, char* argv[]) {
  bool Centrality40Bins=false, Centralitypp=false;
  string fileName, outputDir;
  string effWeight;
  int initev = 0;
  int nevt = -1;

  if (argc == 1) {
    cout << "====================================================================\n";
    cout << "Use the program with this commend:" << endl;
    cout << "./Tree2Datasets =c [centrality type] =ot [trigType] =or [runType] =oc [use or don't use RP] =op [RP number] =w [Weighting] =f [input TTree file] [output directory]" << endl;
    cout << "=c: (0) PbPb, (1) pp, (2) pA" << endl;
    cout << "=ot: Check trigger combinations in the macro" << endl;
    cout << "=or: Check cut combinations in the macro" << endl;
    cout << "=oc: (0) Use reaction plane, (1) Don't use reaction plane" << endl;
    cout << "=op: (-1) Normal, (-2) Auto-correction, (-3) Is not flatten" << endl;
    cout << "   : (0 <=) Specific reaction plane numbers in series of 3 eta regions" << endl;
    cout << "./Tree2Datasets =c 0 =ot 3 =or 0 =oc 1 =op -1 =w 0 =f /tmp/miheejo/mini_Jpsi_Histos_may202012_m25gev.root default_bit1" << endl;
    cout << "====================================================================\n";
    return 0;
  }
  
  for (int i=1; i<argc; i++) {
    char *tmpargv = argv[i];
    switch (tmpargv[0]) {
      case '=':{
        switch (tmpargv[1]) {
          case 'c':
            if (0 == atoi(argv[i+1])) Centrality40Bins = true;
            else if (1 == atoi(argv[i+1])) Centralitypp = true;
            else if (2 == atoi(argv[i+1])) {
              Centralitypp = false; Centrality40Bins = false;
            }
            break;
          case 'f':
            fileName = argv[i+1];
            outputDir = argv[i+2];
            break;
          case 'o':
            switch (tmpargv[2]) {
              case 'p':
                RPNUM = atoi(argv[i+1]);
                break;
              case 't':
                trigType = atoi(argv[i+1]);
                break;
              case 'r':
                runType = atoi(argv[i+1]);
                break;
              case 'c':
                if (0 == atoi(argv[i+1])) checkRPNUM = false;
                else checkRPNUM = true;
                break;
            }
            break;
          case 'w':
            if (atoi(argv[i+1]) == 0) doWeighting = false;
            else {
              doWeighting =true;
              effWeight = argv[i+2];
            }
            break;
          case 'e':
            initev = atoi(argv[i+1]);
            nevt = atoi(argv[i+2]);
            break;
        }
      }
    } // end of checking switch loop
  } // end of checking options

  cout << "fileName: " << fileName << endl;
  cout << "output directory: " << outputDir << endl;
  cout << "trigType: "<< trigType << endl;
  cout << "runType: " << runType << endl;
  cout << "checkRPNUM: " << checkRPNUM << endl;
  cout << "RPNUM: " << RPNUM << endl;
  cout << "weighting: " << doWeighting << " " << effWeight << endl;
  cout << "start event #: " << initev << endl;
  cout << "end event #: " << nevt << endl;


  TFile *file=TFile::Open(fileName.c_str());
  TTree *Tree=(TTree*)file->Get("myTree");
  if (!file->IsOpen() || Tree==NULL ) {
    cout << "Cannot open the input file. exit"<< endl;
    return -3;
  }


  // Settings for Lxyz information imports to normal onia tree
  TFile *fileLxyz;
  TTree *TreeLxyz;
  if (use3DCtau) {
    if (isPbPb) fileLxyz=TFile::Open("/home/mihee/cms/oniaTree/2011PbPb/Jpsi_Histos_3Mu_v2.root");
    else fileLxyz=TFile::Open("/home/mihee/cms/oniaTree/2013pp/Lxyz_2013PPMuon_GlbGlb_Jpsi_Histos_3Mu_v1.root");

    TreeLxyz = (TTree*)fileLxyz->Get("myTree");
  }

  unsigned int eventNbLxyz, runNbLxyz, LSLxyz;
  Int_t Reco_QQ_sizeLxyz;
  Float_t Reco_QQ_ctau3D[100], Reco_QQ_ctauErr3D[100], Reco_QQ_ctauLxy[100];
  TClonesArray *Reco_QQ_4momLxyz;
  
  TBranch *b_eventNbLxyz, *b_runNbLxyz, *b_LSLxyz, *b_Reco_QQ_sizeLxyz;
  TBranch *b_Reco_QQ_ctau3D, *b_Reco_QQ_ctauErr3D, *b_Reco_QQ_ctauLxy;
  TBranch *b_Reco_QQ_4momLxyz;

  Reco_QQ_4momLxyz = 0;

  if (use3DCtau) {
    TreeLxyz->SetBranchAddress("runNb", &runNbLxyz, &b_runNbLxyz);
    TreeLxyz->SetBranchAddress("eventNb", &eventNbLxyz, &b_eventNbLxyz);
    TreeLxyz->SetBranchAddress("LS", &LSLxyz, &b_LSLxyz);

    TreeLxyz->SetBranchAddress("Reco_QQ_size", &Reco_QQ_sizeLxyz, &b_Reco_QQ_sizeLxyz);
    TreeLxyz->SetBranchAddress("Reco_QQ_4mom", &Reco_QQ_4momLxyz, &b_Reco_QQ_4momLxyz);
    TreeLxyz->SetBranchAddress("Reco_QQ_ctau", Reco_QQ_ctauLxy, &b_Reco_QQ_ctauLxy);
    TreeLxyz->SetBranchAddress("Reco_QQ_ctau3D", Reco_QQ_ctau3D, &b_Reco_QQ_ctau3D);
    TreeLxyz->SetBranchAddress("Reco_QQ_ctauErr3D", Reco_QQ_ctauErr3D, &b_Reco_QQ_ctauErr3D);
  }

  TLorentzVector* JPLxyz = new TLorentzVector;


  // Settings for efficiency weighting
  TFile *effFileLxy;
  TFile *effFilepT, *effFilepT_LowPt, *effFilepT_ForwHighPt;
  TFile *effFilepT_Minus, *effFilepT_Minus_LowPt, *effFilepT_Minus_ForwHighPt;
  TFile *effFileCowboy, *effFileCowboy_LowPt;
  TFile *effFileSailor, *effFileSailor_LowPt;

  
  // Test for Lxy-Ctau 2D map
  for (int a=0; a<10; a++) {
    hLxyCtau2[a] = new TH2D(Form("hLxyCtau_Data_%d",a),";Lxy(Reco) (mm);ctau (mm)",12,0,3,12,0,3);
  }
  
  for (unsigned int a=0; a<_nRapArr; a++) {
    for (unsigned int b=0; b<_nPtArr; b++) {
      for (unsigned int c=0; c<_nCentArr; c++) {
        unsigned int nidx = a*_nPtArr*_nCentArr + b*_nCentArr + c;
        if (_raparr[a]==-1.6 && _raparr[a+1]==1.6) continue;
                
        hLxyCtau[nidx] = new TH2D(Form("hLxyCtau_Data_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_raparr[a],_raparr[a+1],_ptarr[b],_ptarr[b+1],_centarr[c],_centarr[c+1])
              ,";Lxy(Reco) (mm);ctau (mm)",12,0,3,12,0,3);

      }
    }
  }

  // Forward region + including low pT bins
  for (unsigned int a=0; a<_nRapForwArr; a++) {
    if (_rapforwarr[a]==-1.6 && _rapforwarr[a+1]==1.6) continue;
    for (unsigned int b=0; b<_nPtForwArr; b++) {
      if (_ptforwarr[b]<=6.5 && _ptforwarr[b+1]<=6.5) {
        for (unsigned int c=0; c<_nCentForwArr; c++) {
          unsigned int nidx = a*_nPtForwArr*_nCentForwArr + b*_nCentForwArr + c;
          double ymin=_rapforwarr[a]; double ymax=_rapforwarr[a+1];
          double ptmin=_ptforwarr[b]; double ptmax=_ptforwarr[b+1];
          double centmin=_centforwarr[c]; double centmax=_centforwarr[c+1];

          hLxyCtau_LowPt[nidx] = new TH2D(Form("hLxyCtau_Data_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centforwarr[c],_centforwarr[c+1])
                ,";Lxy(Reco) (mm);ctau (mm)",12,0,3,12,0,3);
        }
      } else {
        for (unsigned int c=0; c<_nCentArr; c++) {
          unsigned int nidx = a*_nPtForwArr*_nCentArr + b*_nCentArr + c;
          double ymin=_rapforwarr[a]; double ymax=_rapforwarr[a+1];
          double ptmin=_ptforwarr[b]; double ptmax=_ptforwarr[b+1];
          double centmin=_centarr[c]; double centmax=_centarr[c+1];

          hLxyCtau_LowPt[nidx] = new TH2D(Form("hLxyCtau_Data_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centarr[c],_centarr[c+1])
                ,";Lxy(Reco) (mm);ctau (mm)",12,0,3,12,0,3);
        }
      }
    }
  }
    
  // To be used with useTnPCorr option (old uses [0] only)
  TF1 *gSingleMuW[2];
  TF1 *gSingleMuW_LowPt[2];
  TF1 *gSingleMuWSTA;
  TF1 *gSingleMuWSTA_LowPt;
  if (useTnPCorr==1) {
    gSingleMuW[0] = new TF1(Form("TnP_ScaleFactor"),
    "(0.9555*TMath::Erf((x-1.3240)/2.5683))/(0.9576*TMath::Erf((x-1.7883)/2.6583))");
    gSingleMuW_LowPt[0] = new TF1(Form("TnP_ScaleFactor_LowPt"),
    "(0.8335*TMath::Erf((x-1.2470)/1.9782))/(0.7948*TMath::Erf((x-1.3091)/2.2783))");
  } else if (useTnPCorr==2 || useTnPCorr==3) {
    gSingleMuW[0] = new TF1(Form("TnP_ScaleFactor_Rap0.0-0.9"),
    "(0.9646*TMath::Erf((x-0.1260)/3.5155))/(0.9724*TMath::Erf((x-0.4114)/3.3775))");
    gSingleMuW[1] = new TF1(Form("TnP_ScaleFactor_Rap0.9-1.6"),
    "(0.9725*TMath::Erf((x-1.0054)/2.3187))/(0.9502*TMath::Erf((x-1.3857)/2.0757))");
    gSingleMuW_LowPt[0] = new TF1(Form("TnP_ScaleFactor_Rap1.6-2.1"),
    "(0.9194*TMath::Erf((x-0.9733)/2.1374))/(0.8971*TMath::Erf((x-1.0984)/2.3510))");
    gSingleMuW_LowPt[1] = new TF1(Form("TnP_ScaleFactor_Rap2.1-2.4"),
    "(0.8079*TMath::Erf((x-0.9421)/0.8577))/(0.7763*TMath::Erf((x-0.8419)/1.6742))");

    // pp SFs
    gSingleMuWSTA = new TF1(Form("TnP_ScaleFactor_STA"),
    "(0.9891*TMath::Erf((x-1.4814)/2.5014))/(0.9911*TMath::Erf((x-1.4336)/2.8548))");
    gSingleMuWSTA_LowPt = new TF1(Form("TnP_ScaleFactor_STA_LowPt"),
    "(0.8956*TMath::Erf((x-0.5162)/1.7646))/(0.9132*TMath::Erf((x-0.8045)/1.8366))");
    // PbPb SFs (not used)
//    gSingleMuWSTA = new TF1(Form("TnP_ScaleFactor_STA"),
//    "(1.0000*TMath::Erf((x-1.3923)/2.3653))/(1.0000*TMath::Erf((x-1.5330)/2.8467))");
//    gSingleMuWSTA_LowPt = new TF1(Form("TnP_ScaleFactor_STA_LowPt"),
//    "(1.0000*TMath::Erf((x-0.0000)/2.5236))/(0.9523*TMath::Erf((x-0.7714)/2.0628))");
  }

  if (doWeighting) {
    string dirPath;
    if (use3DCtau) {
      dirPath = "/home/mihee/cms/RegIt_JpsiRaa/Efficiency/PbPb/root604/RegionsDividedInEta_noTnPCorr/";
      if (!isPbPb) dirPath = "/home/mihee/cms/RegIt_JpsiRaa/Efficiency/pp/root604/RegionsDividedInEta_noTnPCorr/";
    } else {
      dirPath = "/home/mihee/cms/RegIt_JpsiRaa/Efficiency/PbPb/root528/RegionsDividedInEtaLxyBin2/";
      if (!isPbPb) dirPath = "/home/mihee/cms/RegIt_JpsiRaa/Efficiency/pp/root528/RegionsDividedInEtaLxy/";
    }
    char effHistname[1000];

//  vector<int>centEff;
//    size_t pos=0;
//    while ((pos = effWeight.find("+")) != std::string::npos) {
//      string token = effWeight.substr(0,pos);
//      centEff.push_back(atoi(token.c_str()));
//      effWeight.erase(0,pos + 1);
//    }
//    centEff.push_back(atoi(effWeight.c_str()));   //Take the last element
//    nCentEff = centEff.size();
//    cout << "Efficiency weighting turned on: nCentEff:: " << nCentEff << endl;
//    for (unsigned int ii=0; ii<nCentEff; ii++) cout << centEff[ii] << "  ";
//    cout << endl;

    if (trigType == 3 || trigType == 4) {
      if (isPbPb) {
        if (use3DCtau) sprintf(effHistname,"/home/mihee/cms/RegIt_JpsiRaa/datasets/PbPb/RecoLxyEff_RegionsDividedInEta_noTnPCorr_root604/ctau2mm/FinalEfficiency_pbpb_notAbs.root");
        else sprintf(effHistname,"/home/mihee/cms/RegIt_JpsiRaa/datasets/PbPb/RecoLxyEff_RegionsDividedInEtaLxyBin2/FinalEfficiency_pbpb_notAbs.root");
      } else {
        if (use3DCtau) sprintf(effHistname,"/home/mihee/cms/RegIt_JpsiRaa/datasets/pp/RecoLxyEff_RegionsDividedInEta_noTnPCorr_root604/ctau2mm/FinalEfficiency_pp_notAbs.root");
        else sprintf(effHistname,"/home/mihee/cms/RegIt_JpsiRaa/datasets/pp/RecoLxyEff_RegionsDividedInEtaLxy/FinalEfficiency_pp_notAbs.root");
      }
      cout << effHistname << endl;
      effFileLxy = new TFile(effHistname);

      sprintf(effHistname,"%s/notAbs_Rap0.0-1.6_Pt6.5-30.0/PRMC3DAnaBins_eff.root",dirPath.c_str());
      cout << effHistname << endl;
      effFilepT = new TFile(effHistname);
      
      if (useRapPtEff==3 || (useRapPtEff>=5 && useRapPtEff<=8))
        sprintf(effHistname,"%s/notAbs_Rap1.6-2.4_Pt3.0-30.0/PRMC3DAnaBins_eff.root",dirPath.c_str());
      else
        sprintf(effHistname,"%s/notAbs_Rap1.6-2.4_Pt3.0-6.5/PRMC3DAnaBins_eff.root",dirPath.c_str());
      cout << effHistname << endl;
      effFilepT_LowPt = new TFile(effHistname);
      
      sprintf(effHistname,"%s/notAbs_Rap1.6-2.4_Pt6.5-30.0/PRMC3DAnaBins_eff.root",dirPath.c_str());
      cout << effHistname << endl;
      effFilepT_ForwHighPt = new TFile(effHistname);

      sprintf(effHistname,"%s/notAbs_Rap-1.6-0.0_Pt6.5-30.0/PRMC3DAnaBins_eff.root",dirPath.c_str());
      cout << effHistname << endl;
      effFilepT_Minus = new TFile(effHistname);
      
      if (useRapPtEff==3 || (useRapPtEff>=5 && useRapPtEff<=8))
        sprintf(effHistname,"%s/notAbs_Rap-2.4--1.6_Pt3.0-30.0/PRMC3DAnaBins_eff.root",dirPath.c_str());
      else
        sprintf(effHistname,"%s/notAbs_Rap-2.4--1.6_Pt3.0-6.5/PRMC3DAnaBins_eff.root",dirPath.c_str());
      cout << effHistname << endl;
      effFilepT_Minus_LowPt = new TFile(effHistname);
      
      sprintf(effHistname,"%s/notAbs_Rap-2.4--1.6_Pt6.5-30.0/PRMC3DAnaBins_eff.root",dirPath.c_str());
      cout << effHistname << endl;
      effFilepT_Minus_ForwHighPt = new TFile(effHistname);

//      if (!effFileLxy->IsOpen() ||
//          !effFilepT->IsOpen() || !effFilepT_LowPt->IsOpen() || !effFilepT_ForwHighPt->IsOpen() ||
//          !effFilepT_Minus->IsOpen() || !effFilepT_Minus_LowPt->IsOpen() || !effFilepT_Minus_ForwHighPt->IsOpen()
//         ) {
//        cout << "CANNOT read efficiency root files. Exit." << endl;
//        return -4;
//      }

    }

    TLatex *lat = new TLatex(); lat->SetNDC(); lat->SetTextSize(0.035); lat->SetTextColor(kBlack);
    if (trigType == 3 || trigType == 4) {
      // Mid-rapidity
      for (unsigned int a=0; a<nRapArr; a++) {
        for (unsigned int c=0; c<nCentArr; c++) {
          unsigned int nidx = a*nCentArr + c;
          if (raparr[a]==-1.6 && raparr[a+1]==1.6) continue;

          string fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d",raparr[a],raparr[a+1],centarr[c],centarr[c+1]);
          if (raparr[a]<=0 && raparr[a+1]<=0)
            heffPt[nidx] = (TH1D*)effFilepT_Minus->Get(fitname.c_str());
          else
            heffPt[nidx] = (TH1D*)effFilepT->Get(fitname.c_str());
          fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d_TF",raparr[a],raparr[a+1],centarr[c],centarr[c+1]);
          if (useRapPtEff==7) {
            feffPt[nidx] = new TF1(fitname.c_str(),fitfunc_min[nidx].c_str(),6.5,30);
            cout << "\t\t feffPt["<<nidx<<"] " << fitfunc_min[nidx] << "\t" << feffPt[nidx]->Eval(6.5) << endl;
          } else if (useRapPtEff==8) {
            feffPt[nidx] = new TF1(fitname.c_str(),fitfunc_max[nidx].c_str(),6.5,30);
            cout << "\t\t feffPt["<<nidx<<"] " << fitfunc_max[nidx] << "\t" << feffPt[nidx]->Eval(6.5) << endl;
          } else { // nominal
            if (raparr[a]<=0 && raparr[a+1]<=0)
              feffPt[nidx] = (TF1*)effFilepT_Minus->Get(fitname.c_str());
            else
              feffPt[nidx] = (TF1*)effFilepT->Get(fitname.c_str());
          }
          fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d_GASM",raparr[a],raparr[a+1],centarr[c],centarr[c+1]);
          if (raparr[a]<=0 && raparr[a+1]<=0)
            geffPt[nidx] = (TGraphAsymmErrors*)effFilepT_Minus->Get(fitname.c_str());
          else
            geffPt[nidx] = (TGraphAsymmErrors*)effFilepT->Get(fitname.c_str());
          cout << "\t" << nidx << " " << feffPt[nidx] << " "<< heffPt[nidx] << " " << geffPt[nidx] << endl;
          cout << "\t" << nidx << " " << feffPt[nidx]->GetName() << " "<< heffPt[nidx]->GetName() << " " << geffPt[nidx]->GetName() << endl;

          fitname = Form("h1DEmptyPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d",raparr[a],raparr[a+1],centarr[c],centarr[c+1]);
          heffEmpty[nidx] = new TH1D(fitname.c_str(),"#varepsilon #leq 0;p_{T} (GeV/c);Counts",14,2.0,30.0);
        }
      }

      // 2D rap-pt efficiency fit function
      if (useRapPtEff==1 || useRapPtEff==2 || useRapPtEff==4) {
        for (unsigned int c=0; c<nCentArr; c++) {
          string fitname = Form("h2DEffRapPt_PRJpsi_Rap-1.6-0.0_Pt6.5-30.0_Cent%d-%d_TF",centarr[c],centarr[c+1]);
          feffRapPt[c] = (TF2*)effFilepT->Get(fitname.c_str());
          if (!feffRapPt[c]) feffRapPt[c] = (TF2*)effFilepT_Minus->Get(fitname.c_str());
          cout << feffRapPt[c]->GetName() << endl;
        }
        for (unsigned int c=0; c<nCentArr; c++) {
          string fitname = Form("h2DEffRapPt_PRJpsi_Rap0.0-1.6_Pt6.5-30.0_Cent%d-%d_TF",centarr[c],centarr[c+1]);
          feffRapPt[c+nCentArr] = (TF2*)effFilepT->Get(fitname.c_str());
          if (!feffRapPt[c+nCentArr]) feffRapPt[c+nCentArr] = (TF2*)effFilepT_Minus->Get(fitname.c_str());
          cout << feffRapPt[c+nCentArr]->GetName() << endl;
        }
      }


      if (isPbPb || !isPbPb) {
        for (unsigned int a=0; a<_nRapArr; a++) {
          for (unsigned int b=0; b<_nPtArr; b++) {
            for (unsigned int c=0; c<_nCentArr; c++) {
              unsigned int nidx = a*_nPtArr*_nCentArr + b*_nCentArr + c;
              if (_raparr[a]==-1.6 && _raparr[a+1]==1.6) continue;
              
              string fitname;
              if (!effWeight.compare("profile")) {
                fitname = Form("heffProf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_raparr[a],_raparr[a+1],_ptarr[b],_ptarr[b+1],_centarr[c],_centarr[c+1]);
                heffLxy[nidx] = (TH1D*)effFileLxy->Get(fitname.c_str());
                fitname = Form("heffProf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d_TF",_raparr[a],_raparr[a+1],_ptarr[b],_ptarr[b+1],_centarr[c],_centarr[c+1]);
                feffLxy[nidx] = (TF1*)effFileLxy->Get(fitname.c_str());
              } else if ( (!effWeight.compare("weightedEff")) || (effWeight.compare("profile")) ){
                fitname = Form("heffSimUnf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_raparr[a],_raparr[a+1],_ptarr[b],_ptarr[b+1],_centarr[c],_centarr[c+1]);
                heffLxy[nidx] = (TH1D*)effFileLxy->Get(fitname.c_str());
                
                fitname = Form("heffSimUnf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d_TF",_raparr[a],_raparr[a+1],_ptarr[b],_ptarr[b+1],_centarr[c],_centarr[c+1]);
                feffLxy[nidx] = (TF1*)effFileLxy->Get(fitname.c_str());
              }
              cout << "\t" << nidx << " " << fitname  << " " << feffLxy[nidx] << " " << heffLxy[nidx] << endl;
              
            }
          }
        }
      }

      // Forward region + including low pT bins
      for (unsigned int a=0; a<nRapForwArr; a++) {
        if (rapforwarr[a]==-1.6 && rapforwarr[a+1]==1.6) continue;
        for (unsigned int c=0; c<nCentForwArr; c++) {
          unsigned int nidx = a*nCentForwArr + c;

          string fitname;
          if (useRapPtEff==3 || (useRapPtEff>=5 && useRapPtEff<=8)) {
            fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt3.0-30.0_Cent%d-%d",rapforwarr[a],rapforwarr[a+1],centforwarr[c],centforwarr[c+1]);
            if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
              heffPt_LowPt[nidx] = (TH1D*)effFilepT_Minus_LowPt->Get(fitname.c_str());
            else
              heffPt_LowPt[nidx] = (TH1D*)effFilepT_LowPt->Get(fitname.c_str());
            fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt3.0-30.0_Cent%d-%d_TF",rapforwarr[a],rapforwarr[a+1],centforwarr[c],centforwarr[c+1]);
            if (useRapPtEff==7) {
              feffPt_LowPt[nidx] = new TF1(fitname.c_str(),fitfunc_min_LowPt[nidx].c_str(),3,30);
              cout << "\t\t feffPt_LowPt["<<nidx<<"] " << fitfunc_min_LowPt[nidx] << "\t" << feffPt_LowPt[nidx]->Eval(6.5) << endl;
            } else if (useRapPtEff==8) {
              feffPt_LowPt[nidx] = new TF1(fitname.c_str(),fitfunc_max_LowPt[nidx].c_str(),3,30);
              cout << "\t\t feffPt_LowPt["<<nidx<<"] " << fitfunc_max_LowPt[nidx] << "\t" << feffPt_LowPt[nidx]->Eval(6.5) << endl;
            } else { // nominal
              if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
                feffPt_LowPt[nidx] = (TF1*)effFilepT_Minus_LowPt->Get(fitname.c_str());
              else
                feffPt_LowPt[nidx] = (TF1*)effFilepT_LowPt->Get(fitname.c_str());
            }
            fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt3.0-30.0_Cent%d-%d_GASM",rapforwarr[a],rapforwarr[a+1],centforwarr[c],centforwarr[c+1]);
            if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
              geffPt_LowPt[nidx] = (TGraphAsymmErrors*)effFilepT_Minus_LowPt->Get(fitname.c_str());
            else
              geffPt_LowPt[nidx] = (TGraphAsymmErrors*)effFilepT_LowPt->Get(fitname.c_str());
          } else {
            fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt3.0-6.5_Cent%d-%d",rapforwarr[a],rapforwarr[a+1],centforwarr[c],centforwarr[c+1]);
            if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
              heffPt_LowPt[nidx] = (TH1D*)effFilepT_Minus_LowPt->Get(fitname.c_str());
            else
              heffPt_LowPt[nidx] = (TH1D*)effFilepT_LowPt->Get(fitname.c_str());
            fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt3.0-6.5_Cent%d-%d_TF",rapforwarr[a],rapforwarr[a+1],centforwarr[c],centforwarr[c+1]);
            if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
              feffPt_LowPt[nidx] = (TF1*)effFilepT_Minus_LowPt->Get(fitname.c_str());
            else
              feffPt_LowPt[nidx] = (TF1*)effFilepT_LowPt->Get(fitname.c_str());
          }
          cout << "\t" << nidx << " feffPt_LowPt: " << feffPt_LowPt[nidx] << " " << heffPt_LowPt[nidx] << " " << geffPt_LowPt[nidx] << endl;
          cout << "\t" << nidx << " " << feffPt_LowPt[nidx]->GetName() << " "<< heffPt_LowPt[nidx]->GetName() << " " << geffPt_LowPt[nidx]->GetName() << endl;
        }
        
        for (unsigned int c=0; c<nCentArr; c++) {
          unsigned int nidx = a*nCentArr + c;
          
          string fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d",rapforwarr[a],rapforwarr[a+1],centarr[c],centarr[c+1]);
          if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
            heffPt_ForwHighPt[nidx] = (TH1D*)effFilepT_Minus_ForwHighPt->Get(fitname.c_str());
          else
            heffPt_ForwHighPt[nidx] = (TH1D*)effFilepT_ForwHighPt->Get(fitname.c_str());
          fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d_TF",rapforwarr[a],rapforwarr[a+1],centarr[c],centarr[c+1]);
//          if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
//            feffPt_ForwHighPt[nidx] = (TF1*)effFilepT_Minus_ForwHighPt->Get(fitname.c_str());
//          else
//            feffPt_ForwHighPt[nidx] = (TF1*)effFilepT_ForwHighPt->Get(fitname.c_str());
          fitname = Form("h1DEffPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d_GASM",rapforwarr[a],rapforwarr[a+1],centarr[c],centarr[c+1]);
          if (rapforwarr[a]<=0 && rapforwarr[a+1]<=0)
            geffPt_ForwHighPt[nidx] = (TGraphAsymmErrors*)effFilepT_Minus_ForwHighPt->Get(fitname.c_str());
          else
            geffPt_ForwHighPt[nidx] = (TGraphAsymmErrors*)effFilepT_ForwHighPt->Get(fitname.c_str());
//          cout << "\t" << nidx << " feffPt_ForwHighPt: " << feffPt_ForwHighPt[nidx] << " " << heffPt_ForwHighPt[nidx] << endl;
//          cout << "\t" << nidx <<  feffPt_ForwHighPt[nidx]->GetName() << " " << heffPt_ForwHighPt[nidx]->GetName() << endl;
          fitname = Form("h1DEmptyPt_PRJpsi_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d",rapforwarr[a],rapforwarr[a+1],centarr[c],centarr[c+1]);
          heffEmpty_LowPt[nidx] = new TH1D(fitname.c_str(),"#varepsilon #leq 0;p_{T} (GeV/c);Counts",14,2.0,30.0);
        }

      }

      // 2D rap-pt efficiency fit function
      if (useRapPtEff==1 || useRapPtEff==2 || useRapPtEff==4) {
        for (unsigned int c=0; c<nCentForwArr; c++) {
          string fitname = Form("h2DEffRapPt_PRJpsi_Rap-2.4--1.6_Pt3.0-6.5_Cent%d-%d_TF",centforwarr[c],centforwarr[c+1]);
          feffRapPt_LowPt[c] = (TF2*)effFilepT_LowPt->Get(fitname.c_str());
          if (!feffRapPt_LowPt[c]) feffRapPt_LowPt[c] = (TF2*)effFilepT_Minus_LowPt->Get(fitname.c_str());
          cout << feffRapPt_LowPt[c]->GetName() << endl;

          fitname = Form("h2DEffRapPt_PRJpsi_Rap1.6-2.4_Pt3.0-6.5_Cent%d-%d_TF",centforwarr[c],centforwarr[c+1]);
          feffRapPt_LowPt[c+nCentForwArr] = (TF2*)effFilepT_LowPt->Get(fitname.c_str());
          if (!feffRapPt_LowPt[c+nCentForwArr]) feffRapPt_LowPt[c+nCentForwArr] = (TF2*)effFilepT_Minus_LowPt->Get(fitname.c_str());
          cout << feffRapPt_LowPt[c+nCentForwArr]->GetName() << endl;
        }
        for (unsigned int c=0; c<nCentArr; c++) {
          string fitname = Form("h2DEffRapPt_PRJpsi_Rap1.6-2.4_Pt6.5-30.0_Cent%d-%d_TF",centarr[c],centarr[c+1]);
          feffRapPt_ForwHighPt[c+nCentArr] = (TF2*)effFilepT_ForwHighPt->Get(fitname.c_str());
          if (!feffRapPt_ForwHighPt[c+nCentArr]) feffRapPt_ForwHighPt[c+nCentArr] = (TF2*)effFilepT_Minus_ForwHighPt->Get(fitname.c_str());
          cout << feffRapPt_ForwHighPt[c+nCentArr]->GetName() << endl;

          fitname = Form("h2DEffRapPt_PRJpsi_Rap-2.4--1.6_Pt6.5-30.0_Cent%d-%d_TF",centarr[c],centarr[c+1]);
          feffRapPt_ForwHighPt[c] = (TF2*)effFilepT_ForwHighPt->Get(fitname.c_str());
          if (!feffRapPt_ForwHighPt[c]) feffRapPt_ForwHighPt[c] = (TF2*)effFilepT_Minus_ForwHighPt->Get(fitname.c_str());
          cout << feffRapPt_ForwHighPt[c]->GetName() << endl;
        }
      }

      if (isPbPb || !isPbPb) {
        for (unsigned int a=0; a<_nRapForwArr; a++) {
          if (_rapforwarr[a]==-1.6 && _rapforwarr[a+1]==1.6) continue;
          for (unsigned int b=0; b<_nPtForwArr; b++) {
            if (_ptforwarr[b]<=6.5 && _ptforwarr[b+1]<=6.5) {
              for (unsigned int c=0; c<_nCentForwArr; c++) {
                unsigned int nidx = a*_nPtForwArr*_nCentForwArr + b*_nCentForwArr + c;
                double ymin=_rapforwarr[a]; double ymax=_rapforwarr[a+1];
                double ptmin=_ptforwarr[b]; double ptmax=_ptforwarr[b+1];
                double centmin=_centforwarr[c]; double centmax=_centforwarr[c+1];
                
                string fitname;
                if (!effWeight.compare("profile")) {
                  fitname = Form("heffProf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centforwarr[c],_centforwarr[c+1]);
                  heffLxy_LowPt[nidx] = (TH1D*)effFileLxy->Get(fitname.c_str());
                  fitname = Form("heffProf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d_TF",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centforwarr[c],_centforwarr[c+1]);
                  feffLxy_LowPt[nidx] = (TF1*)effFileLxy->Get(fitname.c_str());
                } else if ( (!effWeight.compare("weightedEff")) || (effWeight.compare("profile")) ){
                  fitname = Form("heffSimUnf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centforwarr[c],_centforwarr[c+1]);
                  heffLxy_LowPt[nidx] = (TH1D*)effFileLxy->Get(fitname.c_str());
                  fitname = Form("heffSimUnf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d_TF",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centforwarr[c],_centforwarr[c+1]);
                  feffLxy_LowPt[nidx] = (TF1*)effFileLxy->Get(fitname.c_str());
                }
                cout << "\t" << nidx << " " << fitname << " " << feffLxy_LowPt[nidx] << " " << heffLxy_LowPt[nidx] << endl;
              }
            } else {
              for (unsigned int c=0; c<_nCentArr; c++) {
                unsigned int nidx = a*_nPtForwArr*_nCentArr + b*_nCentArr + c;
                double ymin=_rapforwarr[a]; double ymax=_rapforwarr[a+1];
                double ptmin=_ptforwarr[b]; double ptmax=_ptforwarr[b+1];
                double centmin=_centarr[c]; double centmax=_centarr[c+1];
                
                string fitname;
                if (!effWeight.compare("profile")) {
                  fitname = Form("heffProf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centarr[c],_centarr[c+1]);
                  heffLxy_LowPt[nidx] = (TH1D*)effFileLxy->Get(fitname.c_str());

                  fitname = Form("heffProf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d_TF",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centarr[c],_centarr[c+1]);
                  feffLxy_LowPt[nidx] = (TF1*)effFileLxy->Get(fitname.c_str());
                } else if ( (!effWeight.compare("weightedEff")) || (effWeight.compare("profile")) ){
                  fitname = Form("heffSimUnf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centarr[c],_centarr[c+1]);
                  heffLxy_LowPt[nidx] = (TH1D*)effFileLxy->Get(fitname.c_str());

                  fitname = Form("heffSimUnf_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d_TF",_rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centarr[c],_centarr[c+1]);
                  feffLxy_LowPt[nidx] = (TF1*)effFileLxy->Get(fitname.c_str());
                }
                cout << "\t" << nidx << " " << fitname << " " << feffLxy_LowPt[nidx] << " " << heffLxy_LowPt[nidx] << endl;
              }
            }
          }
        }
      }

    } else { // end of trig==3 || trig==4
      cout << "##########################################################\n";
      cout << "You chose trigType " << trigType << endl;
      cout << "This trigType do NOT work with efficiency weighting.\n";
      cout << "Efficiency weighting will NOT be applied!\n";
      cout << "##########################################################\n";
      doWeighting = false;
    }

  }

  UInt_t          runNb;
  UInt_t          eventNb;
  UInt_t          LS;
  Int_t           Centrality;
  Int_t           Reco_QQ_size;
  Int_t           Reco_QQ_type[100];   //[Reco_QQ_size]
  Int_t           Reco_QQ_sign[100];   //[Reco_QQ_size]
  Int_t           Reco_QQ_trig[100];   //[Reco_QQ_size]
  Int_t           Reco_QQ_mupl_nMuValHits[100];
  Int_t           Reco_QQ_mumi_nMuValHits[100];
  Int_t           Reco_QQ_mupl_nTrkHits[100];  // track hits plus global muons
  Int_t           Reco_QQ_mumi_nTrkHits[100];  // track hits minus global muons
  Int_t           Reco_QQ_mupl_nTrkWMea[100];  // tracker layers with measurement for plus inner track muons
  Int_t           Reco_QQ_mumi_nTrkWMea[100];  // tracker layers with measurement for minus inner track muons
  Int_t           Reco_QQ_mupl_numOfMatch[100];  // number of matched segments for plus inner track muons
  Int_t           Reco_QQ_mumi_numOfMatch[100];  // number of matched segments for minus inner track muons
  Float_t         Reco_QQ_mupl_norChi2_inner[100];  // chi2/ndof for plus inner track muons
  Float_t         Reco_QQ_mumi_norChi2_inner[100];  // chi2/ndof for minus inner track muons
  Float_t         Reco_QQ_mupl_norChi2_global[100];  // chi2/ndof for plus global muons
  Float_t         Reco_QQ_mumi_norChi2_global[100];  // chi2/ndof for minus global muons
  TClonesArray    *Reco_QQ_4mom;
  TClonesArray    *Reco_QQ_mupl_4mom;
  TClonesArray    *Reco_QQ_mumi_4mom;
  Float_t         Reco_QQ_ctau[100];   //[Reco_QQ_size]
  Float_t         Reco_QQ_ctauErr[100];   //[Reco_QQ_size]
  Float_t         Reco_QQ_VtxProb[100];   //[Reco_QQ_size]
  Float_t         rpAng[38];   //[Reco_QQ_size]
  Float_t         zVtx;         //Primary vertex position
  int             HLTriggers; 
//  Int_t           Gen_QQ_size;
//  Int_t           Gen_QQ_type[100];
//  Float_t         Reco_QQ_ctauTrue[100];   //[Reco_QQ_size]

  TBranch        *b_runNb;
  TBranch        *b_eventNb;
  TBranch        *b_LS;
  TBranch        *b_Centrality;   //!
  TBranch        *b_Reco_QQ_size;   //!
  TBranch        *b_Reco_QQ_type;   //!
  TBranch        *b_Reco_QQ_sign;   //!
  TBranch        *b_Reco_QQ_mupl_nMuValHits;   //!
  TBranch        *b_Reco_QQ_mumi_nMuValHits;   //!
  TBranch        *b_Reco_QQ_mupl_nTrkHits;
  TBranch        *b_Reco_QQ_mumi_nTrkHits;
  TBranch        *b_Reco_QQ_mupl_nTrkWMea;
  TBranch        *b_Reco_QQ_mumi_nTrkWMea;
  TBranch        *b_Reco_QQ_mupl_norChi2_inner;
  TBranch        *b_Reco_QQ_mumi_norChi2_inner;
  TBranch        *b_Reco_QQ_mupl_norChi2_global;
  TBranch        *b_Reco_QQ_mumi_norChi2_global;
  TBranch        *b_Reco_QQ_mupl_numOfMatch;   //!
  TBranch        *b_Reco_QQ_mumi_numOfMatch;   //!
  TBranch        *b_HLTriggers;   //!
  TBranch        *b_Reco_QQ_trig;   //!
  TBranch        *b_Reco_QQ_4mom;   //!
  TBranch        *b_Reco_QQ_mupl_4mom;   //!
  TBranch        *b_Reco_QQ_mumi_4mom;   //!
  TBranch        *b_Reco_QQ_ctau;   //!
  TBranch        *b_Reco_QQ_ctauErr;   //!
  TBranch        *b_Reco_QQ_VtxProb;   //!
  TBranch        *b_rpAng;   //!
  TBranch        *b_zVtx;
//  TBranch        *b_Gen_QQ_size;   //!
//  TBranch        *b_Gen_QQ_type;
//  TBranch        *b_Reco_QQ_ctauTrue;   //!

  TLorentzVector* JP= new TLorentzVector;
  TLorentzVector* m1P= new TLorentzVector;
  TLorentzVector* m2P= new TLorentzVector;


  TH1I *PassingEvent;
  // Normal datasets
  RooDataSet* dataJpsi;
  RooDataSet* dataJpsiSame;
  // Have efficiency for every events
  RooDataSet* dataJpsiW;
  RooDataSet* dataJpsiSameW;
  // Efficiencies are applied to datasets as a weight
  RooDataSet* dataJpsiWeight;
  RooDataSet* dataJpsiSameWeight;
  // Random datasets : 2nd weighted dataset
  RooDataSet* dataJpsiW2;
  RooDataSet* dataJpsiWeight2;
  
  RooRealVar* Jpsi_Mass;
  RooRealVar* Psip_Mass;      
  RooRealVar* Jpsi_Pt;
  RooRealVar* Jpsi_Ct;
  RooRealVar* Jpsi_Lxyz;
  RooRealVar* Jpsi_CtErr;
  RooRealVar* Jpsi_Y;
  RooRealVar* Jpsi_Phi;
  RooRealVar* Jpsi_dPhi;
  RooRealVar* Jpsi_Cent;
  RooCategory* Jpsi_Type;
  RooCategory* Jpsi_Sign;
  RooRealVar* Jpsi_3DEff; //3D efficiency
//  RooRealVar* Jpsi_CtTrue;
//  RooCategory* MCType;

  Jpsi_Mass = new RooRealVar("Jpsi_Mass","J/#psi mass",Jpsi_MassMin,Jpsi_MassMax,"GeV/c^{2}");
  Psip_Mass = new RooRealVar("Psip_Mass","#psi' mass",3.3,Jpsi_MassMax,"GeV/c^{2}");
  Jpsi_Pt = new RooRealVar("Jpsi_Pt","J/#psi pt",Jpsi_PtMin,Jpsi_PtMax,"GeV/c");
  Jpsi_Y = new RooRealVar("Jpsi_Y","J/#psi y",-Jpsi_YMax,Jpsi_YMax);
  Jpsi_Phi = new RooRealVar("Jpsi_Phi","J/#psi phi",Jpsi_PhiMin,Jpsi_PhiMax,"rad");
  Jpsi_dPhi = new RooRealVar("Jpsi_dPhi","J/#psi phi - rpAng",Jpsi_dPhiMin,Jpsi_dPhiMax,"rad");
  Jpsi_Type = new RooCategory("Jpsi_Type","Category of Jpsi_");
  Jpsi_Sign = new RooCategory("Jpsi_Sign","Charge combination of Jpsi_");
  Jpsi_Ct = new RooRealVar("Jpsi_Ct","J/#psi c#tau",Jpsi_CtMin,Jpsi_CtMax,"mm");
  Jpsi_Lxyz = new RooRealVar("Jpsi_Lxyz","J/#psi L_{xyz}",Jpsi_CtMin,Jpsi_CtMax,"mm");
  Jpsi_CtErr = new RooRealVar("Jpsi_CtErr","J/#psi c#tau error",Jpsi_CtErrMin,Jpsi_CtErrMax,"mm");
  Jpsi_3DEff = new RooRealVar("Jpsi_3DEff","J/#psi efficiency weight",1.,100.);
  Jpsi_Cent = new RooRealVar("Centrality","Centrality of the event",0,100);
//  MCType = new RooCategory("MCType","Type of generated Jpsi_");
//  Jpsi_CtTrue = new RooRealVar("Jpsi_CtTrue","J/#psi c#tau true",Jpsi_CtMin,Jpsi_CtMax,"mm");

  Jpsi_Type->defineType("GG",0);
  Jpsi_Type->defineType("GT",1);
  Jpsi_Type->defineType("TT",2);

  Jpsi_Sign->defineType("OS",0);
  Jpsi_Sign->defineType("PP",1);
  Jpsi_Sign->defineType("MM",2);

//  MCType->defineType("PR",0);
//  MCType->defineType("NP",1);

  Reco_QQ_4mom = 0;
  Reco_QQ_mupl_4mom = 0;
  Reco_QQ_mumi_4mom = 0;

  Tree->SetBranchAddress("runNb", &runNb, &b_runNb);
  Tree->SetBranchAddress("eventNb", &eventNb, &b_eventNb);
  Tree->SetBranchAddress("LS", &LS, &b_LS);
  Tree->SetBranchAddress("Centrality", &Centrality, &b_Centrality);
  if (RPNUM == -3) {  //Not-flatten
    Tree->SetBranchAddress("NfRpAng", rpAng, &b_rpAng);
  } else {  //Flatten reaction plane
    Tree->SetBranchAddress("rpAng", rpAng, &b_rpAng);
  }
  Tree->SetBranchAddress("Reco_QQ_size", &Reco_QQ_size, &b_Reco_QQ_size);
  Tree->SetBranchAddress("HLTriggers", &HLTriggers, &b_HLTriggers);
  Tree->SetBranchAddress("Reco_QQ_trig", Reco_QQ_trig, &b_Reco_QQ_trig);
  Tree->SetBranchAddress("Reco_QQ_type", Reco_QQ_type, &b_Reco_QQ_type);
  Tree->SetBranchAddress("Reco_QQ_sign", Reco_QQ_sign, &b_Reco_QQ_sign);
  Tree->SetBranchAddress("Reco_QQ_mupl_nMuValHits", Reco_QQ_mupl_nMuValHits, &b_Reco_QQ_mupl_nMuValHits);
  Tree->SetBranchAddress("Reco_QQ_mumi_nMuValHits", Reco_QQ_mumi_nMuValHits, &b_Reco_QQ_mumi_nMuValHits);
  Tree->SetBranchAddress("Reco_QQ_mupl_numOfMatch", Reco_QQ_mupl_numOfMatch, &b_Reco_QQ_mupl_numOfMatch);
  Tree->SetBranchAddress("Reco_QQ_mumi_numOfMatch", Reco_QQ_mumi_numOfMatch, &b_Reco_QQ_mumi_numOfMatch);
  Tree->SetBranchAddress("Reco_QQ_mupl_nTrkHits", Reco_QQ_mupl_nTrkHits, &b_Reco_QQ_mupl_nTrkHits);
  Tree->SetBranchAddress("Reco_QQ_mumi_nTrkHits", Reco_QQ_mumi_nTrkHits, &b_Reco_QQ_mumi_nTrkHits);
  Tree->SetBranchAddress("Reco_QQ_mupl_nTrkWMea", Reco_QQ_mupl_nTrkWMea, &b_Reco_QQ_mupl_nTrkWMea);
  Tree->SetBranchAddress("Reco_QQ_mumi_nTrkWMea", Reco_QQ_mumi_nTrkWMea, &b_Reco_QQ_mumi_nTrkWMea);
  Tree->SetBranchAddress("Reco_QQ_mupl_norChi2_inner", Reco_QQ_mupl_norChi2_inner, &b_Reco_QQ_mupl_norChi2_inner);
  Tree->SetBranchAddress("Reco_QQ_mumi_norChi2_inner", Reco_QQ_mumi_norChi2_inner, &b_Reco_QQ_mumi_norChi2_inner);
  Tree->SetBranchAddress("Reco_QQ_mupl_norChi2_global", Reco_QQ_mupl_norChi2_global, &b_Reco_QQ_mupl_norChi2_global);
  Tree->SetBranchAddress("Reco_QQ_mumi_norChi2_global", Reco_QQ_mumi_norChi2_global, &b_Reco_QQ_mumi_norChi2_global);
  Tree->SetBranchAddress("Reco_QQ_4mom", &Reco_QQ_4mom, &b_Reco_QQ_4mom);
  Tree->SetBranchAddress("Reco_QQ_mupl_4mom", &Reco_QQ_mupl_4mom, &b_Reco_QQ_mupl_4mom);
  Tree->SetBranchAddress("Reco_QQ_mumi_4mom", &Reco_QQ_mumi_4mom, &b_Reco_QQ_mumi_4mom);
  Tree->SetBranchAddress("Reco_QQ_ctau", Reco_QQ_ctau, &b_Reco_QQ_ctau);
  Tree->SetBranchAddress("Reco_QQ_ctauErr", Reco_QQ_ctauErr, &b_Reco_QQ_ctauErr);
  Tree->SetBranchAddress("Reco_QQ_VtxProb", Reco_QQ_VtxProb, &b_Reco_QQ_VtxProb);
  Tree->SetBranchAddress("zVtx",&zVtx,&b_zVtx);
//  Tree->SetBranchAddress("Gen_QQ_size", &Gen_QQ_size, &b_Gen_QQ_size);
//  Tree->SetBranchAddress("Gen_QQ_type", Gen_QQ_type, &b_Gen_QQ_type);
//  Tree->SetBranchAddress("Reco_QQ_ctauTrue", Reco_QQ_ctauTrue, &b_Reco_QQ_ctauTrue);

  // Without weighting
  RooArgList varlist(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
  RooArgList varlistSame(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
  RooArgList varlist2(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);

  // With weighting
  RooArgList varlistW(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
  RooArgList varlistSameW(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
  RooArgList varlist2W(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);

  // MC Templates
//  RooArgList varlist(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_Type,*Jpsi_dPhi,*MCType,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_CtTrue);
//  RooArgList varlistSame(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_Type,*Jpsi_dPhi,*MCType,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_CtTrue);
//  RooArgList varlist2(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_Type,*Jpsi_dPhi,*MCType,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_CtTrue);

  PassingEvent = new TH1I("NumPassingEvent",";;total number of events",1,1,2);
  dataJpsi = new RooDataSet("dataJpsi","A sample",varlist);
  dataJpsiSame = new RooDataSet("dataJpsiSame","A sample",varlistSame);
  if (doWeighting) {
    dataJpsiW = new RooDataSet("dataJpsiW","A sample",varlistW);
    if (runType==8) {
      dataJpsiW2 = new RooDataSet("dataJpsiW2","A sample",varlistW);
    }
    dataJpsiSameW = new RooDataSet("dataJpsiSameW","A sample",varlistSameW);
  }

  TH1D *JpsiPt = new TH1D("JpsiPt","JpsiPt",25,0,100);

  double arrXaxis[] = {-1.5, -0.7, -0.6, -0.5, -0.467, -0.433, -0.4, -0.367, -0.333, -0.3, -0.267, -0.233, -0.2,
    -0.19, -0.18, -0.17, -0.16, -0.15, -0.14, -0.13, -0.12, -0.11, -0.10, -0.09, -0.08, -0.07, -0.06,
    -0.05, -0.04, -0.03, -0.02, -0.01, 0, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09, 0.1,
    0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.19, 0.2, 0.225, 0.25, 0.275, 0.3, 0.325, 0.35,
    0.375, 0.4, 0.425, 0.45, 0.475, 0.5, 0.547, 0.593, 0.640, 0.687, 0.733, 0.780, 0.827, 0.873,
    0.920, 0.967, 1.013, 1.060, 1.107, 1.153, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0};
  unsigned int sizeXaxis = sizeof(arrXaxis) / sizeof(double);
  TH1D *hJpsiCtau[10];
  for (int hidx=0; hidx<10; hidx++) {
    hJpsiCtau[hidx] = new TH1D(Form("hJpsiCtau_%d",hidx),";#font[12]{l}_{J/#psi} (mm);0.03 mm",sizeXaxis-1,arrXaxis);
  }
  
  const int nEvents = Tree->GetEntries();
  double *randomVar = new double[nEvents];
  if (runType == 8) {
    TRandom *r3 = new TRandom3();
    r3->SetSeed(0);
    r3->RndmArray(nEvents,randomVar);
  }

  // Make a map from event list
  map<int, int> mapEvtList;
  map<int, int>::iterator it_map;
  if (use3DCtau) {
    fstream LifetimeEntryList;
    if (isPbPb) LifetimeEntryList.open("EntryList_20150529.txt",fstream::in);
    else LifetimeEntryList.open("EntryList_20150709.txt",fstream::in);
    cout << "LifetimeEntryList: " << LifetimeEntryList.good() << endl;

    while (LifetimeEntryList.good()) {
      int evFull, evLxyz;
      unsigned int runnum, evtnum;

      LifetimeEntryList >> runnum >> evtnum >> evFull >> evLxyz;
      mapEvtList[evFull] = evLxyz;
    }
  } // End of making map for event list

  // Start to process! Read tree..
  if (nevt == -1) nevt = Tree->GetEntries();
  for (int ev=initev; ev<nevt; ++ev) {
    if (ev%100000==0) cout << ">>>>> EVENT " << ev << " / " << Tree->GetEntries() <<  endl;

    Tree->GetEntry(ev);
 
    float theRPAng=0, theRPAng22=0, theRPAng23=0;

    // Normal HI event plane setting
    if (RPNUM >= 0) {
      theRPAng = rpAng[RPNUM];
      theRPAng22 = rpAng[RPNUM]; //etHFp
      theRPAng23 = rpAng[RPNUM+1]; //etHFm
    } else {
      theRPAng = rpAng[22];  //will not be used, put arbitary number
      theRPAng22 = rpAng[22]; //etHFp
      theRPAng23 = rpAng[23]; //etHFm
    }


    //Loop over all dimuons in this event and find the most J/psi mass closest dimuon (runType == 4)
    double diffMass = 1.0;
    bool passMostJpsi = false;
    struct Condition mostJpsi; //Contains all condition variables for the most possible J/psi, used for runType == 4

    for (int i=0; i<Reco_QQ_size; ++i) {
      struct Condition Jpsi; //Contains all condition variables

      JP = (TLorentzVector*) Reco_QQ_4mom->At(i);
      m1P = (TLorentzVector*) Reco_QQ_mupl_4mom->At(i);
      m2P = (TLorentzVector*) Reco_QQ_mumi_4mom->At(i);
      if (Centrality40Bins) Jpsi.theCentrality = Centrality * 2.5;
      else if (Centralitypp) Jpsi.theCentrality = 97.5;
      else Jpsi.theCentrality = Centrality;
      Jpsi.vprob = Reco_QQ_VtxProb[i];
      Jpsi.theCat = Reco_QQ_type[i];
      Jpsi.Jq = Reco_QQ_sign[i];
      Jpsi.theCt = Reco_QQ_ctau[i];
      Jpsi.theCtErr = Reco_QQ_ctauErr[i];
      Jpsi.Lxyz = Jpsi.theCt*JP->Pt()/PDGJpsiM ;
      
      // If 3D ctau is going to be used
      if (use3DCtau) {
        int eventLxyz = 0;
        try {
          eventLxyz = mapEvtList.at(ev);
        } catch (const std::out_of_range& oor) {
//          cout << "Event in Lxyz root file doesn't exist" << endl;
          continue; // Skip this event, which will not be used in the end!
        } 
//        cout << "eventLxyz: " << eventLxyz << endl;
        TreeLxyz->GetEntry(eventLxyz);

        TLorentzVector* JPLxyz = new TLorentzVector;
        for (int j=0; j<Reco_QQ_sizeLxyz; ++j) {
          TLorentzVector *JPLxyz = (TLorentzVector*)Reco_QQ_4momLxyz->At(j);
          if ((JPLxyz->M() == JP->M()) && (JPLxyz->Pt() == JP->Pt()) && (JPLxyz->Rapidity() == JP->Rapidity())) {
//            if (TMath::Abs(Reco_QQ_ctau[i]-Reco_QQ_ctauLxy[j]) < 1E-4*TMath::Abs(Reco_QQ_ctau[j])) {
              Jpsi.theCt = Reco_QQ_ctau3D[j];
              Jpsi.theCtErr = Reco_QQ_ctauErr3D[j];
              Jpsi.Lxyz = Jpsi.theCt*JP->P()/PDGJpsiM ;
//              cout << "ctau: " << Reco_QQ_ctauLxy[j] << " ctau3D: " << Jpsi.theCt << endl;
//              break;
//            } else {
//              cout << "ctau in 3D file: " << Reco_QQ_ctauLxy[j] << " ctau in 2D file: " << Jpsi.theCt << endl;
//            }
          }
        }
        delete JPLxyz;
      }

      Jpsi.mupl_nMuValHits = Reco_QQ_mupl_nMuValHits[i];      
      Jpsi.mumi_nMuValHits = Reco_QQ_mumi_nMuValHits[i];      
      Jpsi.mupl_numOfMatch = Reco_QQ_mupl_numOfMatch[i];
      Jpsi.mumi_numOfMatch = Reco_QQ_mumi_numOfMatch[i];
      Jpsi.mupl_nTrkHits = Reco_QQ_mupl_nTrkHits[i];
      Jpsi.mumi_nTrkHits = Reco_QQ_mumi_nTrkHits[i];
      Jpsi.mupl_nTrkWMea = Reco_QQ_mupl_nTrkWMea[i];
      Jpsi.mumi_nTrkWMea = Reco_QQ_mumi_nTrkWMea[i];
      Jpsi.mupl_norChi2_inner = Reco_QQ_mupl_norChi2_inner[i];
      Jpsi.mumi_norChi2_inner = Reco_QQ_mumi_norChi2_inner[i];
      Jpsi.mupl_norChi2_global = Reco_QQ_mupl_norChi2_global[i];
      Jpsi.mumi_norChi2_global = Reco_QQ_mumi_norChi2_global[i];
//      Jpsi.genType = Gen_QQ_type[i];
//      Jpsi.theCtTrue = Reco_QQ_ctauTrue[i];

      Jpsi.theMass =JP->M();
      Jpsi.theRapidity=JP->Rapidity();
      Jpsi.theP=JP->P();
      Jpsi.thePt=JP->Pt();
      Jpsi.thePhi = JP->Phi();

      Jpsi.HLTriggers = HLTriggers;
      Jpsi.Reco_QQ_trig = Reco_QQ_trig[i];
      Jpsi.zVtx = zVtx;

      if (checkRPNUM) {
        if (theRPAng > -9) Jpsi.thedPhi=JP->Phi()-theRPAng;
        Jpsi.thedPhi = TMath::Abs(reducedPhi(Jpsi.thedPhi));
        if (theRPAng22 > -9) Jpsi.thedPhi22=JP->Phi()-theRPAng22;
        Jpsi.thedPhi22 = TMath::Abs(reducedPhi(Jpsi.thedPhi22));
        if (theRPAng23 > -9) Jpsi.thedPhi23=JP->Phi()-theRPAng23;
        Jpsi.thedPhi23 = TMath::Abs(reducedPhi(Jpsi.thedPhi23));
   
        if (RPNUM == -1 || RPNUM == -3 || RPNUM >= 0) {
          if (JP->Eta() < 0) Jpsi.thedPhi = Jpsi.thedPhi22;
          else Jpsi.thedPhi = Jpsi.thedPhi23;
        } else if (RPNUM == -2) {
          if (JP->Eta() < 0) Jpsi.thedPhi = Jpsi.thedPhi23;
          else Jpsi.thedPhi = Jpsi.thedPhi22;
        }

      } else {
        theRPAng = 0;
        theRPAng22 = 0;
        theRPAng23 = 0;
        Jpsi.thedPhi = TMath::Abs(reducedPhi(JP->Phi()));
        Jpsi.thedPhi22 = Jpsi.thedPhi;
        Jpsi.thedPhi23 = Jpsi.thedPhi;
      }

      // Regardless of checkRPNUM option, runType==9 should be filled with Jpsi phi.
      if (runType == 9) {
        Jpsi.thedPhi = JP->Phi();
        Jpsi.thedPhi22 = Jpsi.thedPhi;
        Jpsi.thedPhi23 = Jpsi.thedPhi;
      }

      // get delta Phi between 2 muons to cut out cowboys
      double dPhi2mu = m1P->Phi() - m2P->Phi();
      while (dPhi2mu > TMath::Pi()) dPhi2mu -= 2*TMath::Pi();
      while (dPhi2mu <= -TMath::Pi()) dPhi2mu += 2*TMath::Pi();

      bool cowboy = false, sailor = false;
      if ( 1*dPhi2mu > 0. ) cowboy = true;
      else sailor = true;

      // Check trigger conditions
      bool triggerCondition = checkTriggers(Jpsi, cowboy, sailor);

      bool isAcceptedEP = false;
      if (checkRPNUM && runType != 9) { // for Jpsi v2
        if (RPNUM < 0) {  //combined etHFp+etHFm datasets
          if (RPNUM == -1 || RPNUM == -3) {
            if ((JP->Eta()<0 && theRPAng22 != -10) || (JP->Eta()>=0 && theRPAng23 != -10)) isAcceptedEP = true;
            else isAcceptedEP = false;
          } else if (RPNUM == -2) {
            if ((JP->Eta()<=0 && theRPAng23 != -10) || (JP->Eta()>0 && theRPAng22 != -10)) isAcceptedEP = true;
            else isAcceptedEP = false;
          } else {
            cout << "Wrong RPNUM!\n" << endl;
            return -1;
          }

        } else {  //indivisual event plane datasets
          if ( (JP->Eta()<0 && theRPAng22 != -10) || (JP->Eta()>= 0 && theRPAng23 != -10) ) isAcceptedEP = true;   //auto-correlation removed
          else isAcceptedEP = false;
        }

      } else {  // for Jpsi raa
        isAcceptedEP = true;
      }

      bool passRunType = checkRunType(Jpsi,m1P,m2P);
      double theEff = 0, theEffPt=0, theEffLxy=0, theEffLxyAt0=0;

      if (Jpsi.theMass > Jpsi_MassMin && Jpsi.theMass < Jpsi_MassMax && 
          Jpsi.thePt > Jpsi_PtMin && Jpsi.thePt < Jpsi_PtMax && 
          Jpsi.theCt > Jpsi_CtMin && Jpsi.theCt < Jpsi_CtMax && 
          Jpsi.theCtErr > Jpsi_CtErrMin && Jpsi.theCtErr < Jpsi_CtErrMax && 
          fabs(Jpsi.theRapidity) > Jpsi_YMin && fabs(Jpsi.theRapidity) < Jpsi_YMax &&
          passRunType &&
          triggerCondition &&
          isAcceptedEP &&
          Jpsi.vprob > 0.001
         ) {

        // Test for event numbers in Lxy and Lxyz trees 
        cout << "2D: " << ev << " " << runNb << " " << eventNb << endl;
        if (use3DCtau) {
          int eventLxyz = 0;
          try {
            eventLxyz = mapEvtList.at(ev);
            cout << "3D: " << eventLxyz << " " << runNbLxyz << " " << eventNbLxyz << endl;
          } catch (const std::out_of_range& oor) {
          }
        } 

        // Test for Lxy-Ctau 2D map
        if (fabs(Jpsi.theRapidity)>1.6 && fabs(Jpsi.theRapidity)<2.4 && Jpsi.thePt>3 && Jpsi.thePt<4.5) {
          double lxy;
          if (use3DCtau) lxy = Jpsi.theCt*Jpsi.theP/PDGJpsiM;
          else lxy = Jpsi.theCt*Jpsi.thePt/PDGJpsiM;
          hLxyCtau2[0]->Fill(lxy,Jpsi.theCt);
        } else if (fabs(Jpsi.theRapidity)>1.6 && fabs(Jpsi.theRapidity)<2.4 && Jpsi.thePt>4.5 && Jpsi.thePt<5.5) {
          double lxy;
          if (use3DCtau) lxy = Jpsi.theCt*Jpsi.theP/PDGJpsiM;
          else lxy = Jpsi.theCt*Jpsi.thePt/PDGJpsiM;
          hLxyCtau2[1]->Fill(lxy,Jpsi.theCt);
        } else if (fabs(Jpsi.theRapidity)>1.6 && fabs(Jpsi.theRapidity)<2.4 && Jpsi.thePt>5.5 && Jpsi.thePt<6.5) {
          double lxy;
          if (use3DCtau) lxy = Jpsi.theCt*Jpsi.theP/PDGJpsiM;
          else lxy = Jpsi.theCt*Jpsi.thePt/PDGJpsiM;
          hLxyCtau2[2]->Fill(lxy,Jpsi.theCt);
        }
        if (fabs(Jpsi.theRapidity)>1.6 && fabs(Jpsi.theRapidity)<2.4 && Jpsi.thePt>3 && Jpsi.thePt<6.5) {
          double lxy;
          if (use3DCtau) lxy = Jpsi.theCt*Jpsi.theP/PDGJpsiM;
          else lxy = Jpsi.theCt*Jpsi.thePt/PDGJpsiM;
          hLxyCtau2[3]->Fill(lxy,Jpsi.theCt);
        }

        if (doWeighting) {
          double tmpPt = Jpsi.thePt;
          if (tmpPt >= 30.0) tmpPt = 29.9;
          double lxy = Jpsi.theCt*Jpsi.thePt/PDGJpsiM;
          if (use3DCtau) lxy = Jpsi.theCt*Jpsi.theP/PDGJpsiM;
          lxy = TMath::Abs(lxy);
          if (lxy >= 10) lxy = 9.9;

          cout << "R: " << Jpsi.theRapidity << " Pt: " << Jpsi.thePt << " P: " << Jpsi.theP <<  " C: " << Centrality;
          cout << " cTau: " << Jpsi.theCt << " lxyz: " << lxy << endl;
          
          // 4D efficiency
          if (tmpPt >= 3 && tmpPt < 30 && fabs(Jpsi.theRapidity)>=1.6 && fabs(Jpsi.theRapidity)<2.4) {
            // Pick up a pT eff curve
            for (unsigned int a=0; a<nRapForwArr; a++) {
              if (rapforwarr[a]==-1.6 && rapforwarr[a+1]==1.6) continue;
              for (unsigned int c=0; c<nCentForwArr; c++) {
                unsigned int nidx = a*nCentForwArr + c;
                if ( (Jpsi.theRapidity >= rapforwarr[a] && Jpsi.theRapidity < rapforwarr[a+1]) &&
                     (Centrality >= centforwarr[c] && Centrality < centforwarr[c+1])
                 ) {
                    if (useRapPtEff==1 || useRapPtEff==2) {
                      unsigned int nidx2 = c;
                      if (tmpPt<=6.5) {
                        if (Jpsi.theRapidity>=0) nidx2 = c + nCentForwArr;
                        theEffPt = feffRapPt_LowPt[nidx2]->Eval(Jpsi.theRapidity,tmpPt);
                        cout << "\t" << feffRapPt_LowPt[nidx2]->GetName() << endl;
                        cout << "\t" << feffPt_LowPt[nidx]->GetName() << endl;
                        cout << "\t 1DEffPt " << feffPt_LowPt[nidx]->Eval(tmpPt) << endl;
                      } else {
                        if (Jpsi.theRapidity>=0) nidx2 = c + nCentArr;
                        theEffPt = feffRapPt_ForwHighPt[nidx2]->Eval(Jpsi.theRapidity,tmpPt);
                        cout << "\t" << feffRapPt_ForwHighPt[nidx2]->GetName() << endl;
                        cout << "\t" << feffPt_ForwHighPt[nidx]->GetName() << endl;
                        cout << "\t 1DEffPt " << feffPt_ForwHighPt[nidx]->Eval(tmpPt) << endl;
                      }
                    } else if (useRapPtEff==4) {
                      if (tmpPt<=6.5) {
                        unsigned int nidx2 = c;
                        if (Jpsi.theRapidity>=0) nidx2 = c + nCentForwArr;
                        theEffPt = feffRapPt_LowPt[nidx2]->Eval(Jpsi.theRapidity,tmpPt);
                        cout << "\t" << feffRapPt_LowPt[nidx2]->GetName() << endl;
                        cout << "\t" << feffPt_LowPt[nidx]->GetName() << endl;
                        cout << "\t 1DEffPt " << feffPt_LowPt[nidx]->Eval(tmpPt) << endl;
                      } else {
                        theEffPt = feffPt_ForwHighPt[nidx]->Eval(tmpPt);
                        cout << "\t" << feffPt_ForwHighPt[nidx]->GetName() << endl;
                      }
                    } else if (useRapPtEff==3 || useRapPtEff==7 || useRapPtEff==8) {
                      cout << "\t" << nidx << " " << feffPt_LowPt[nidx]->GetName() << endl;
                      cout << "\t 1DEffPt " << feffPt_LowPt[nidx]->Eval(tmpPt) << endl;
                      theEffPt = feffPt_LowPt[nidx]->Eval(tmpPt);
                    } else if (useRapPtEff==6) {
                      cout << "\t" << geffPt_LowPt[nidx]->GetName() << endl;
                      cout << "\t 1DEffPt " << geffPt_LowPt[nidx]->Eval(tmpPt) << endl;
                      theEffPt = geffPt_LowPt[nidx]->Eval(tmpPt);
                    } else if (useRapPtEff==5) {
                      cout << "\t" << heffPt_LowPt[nidx]->GetName() << endl;
                      cout << "\t 1DEffPt " << feffPt_LowPt[nidx]->Eval(tmpPt) << endl;
                      int binN = heffPt_LowPt[nidx]->FindBin(tmpPt);
                      theEffPt = heffPt_LowPt[nidx]->GetBinContent(binN);
                    } else {
                      theEffPt = feffPt_LowPt[nidx]->Eval(tmpPt);
                      cout << "\t" << feffPt_LowPt[nidx]->GetName() << endl;
                    }
                    
                    if (theEffPt<=0) {
                      heffEmpty_LowPt[nidx]->Fill(tmpPt);
                      int binnumber = heffPt_LowPt[nidx]->FindBin(tmpPt);
                      // Get content from the previous bin
                      while (heffPt_LowPt[nidx]->GetBinContent(binnumber)<=0) binnumber--;
                      theEffPt = heffPt_LowPt[nidx]->GetBinContent(binnumber);
                      cout << "Low eff(Pt): " << geffPt_LowPt[nidx]->Eval(tmpPt) << " " << feffPt_LowPt[nidx]->Eval(tmpPt) << " & " << heffPt_LowPt[nidx]->GetBinContent(binnumber) << " -> " << theEffPt << endl;
                    }

                }
              }
            }

            if (useLxyzCorr) {
              // Pick up a Lxy eff curve
              for (unsigned int a=0; a<_nRapForwArr; a++) {
                if (_rapforwarr[a]==-1.6 && _rapforwarr[a+1]==1.6) continue;
                for (unsigned int b=0; b<_nPtForwArr; b++) {
                  if (tmpPt<=6.5) {
                  for (unsigned int c=0; c<_nCentForwArr; c++) {
                    unsigned int nidx = a*_nPtForwArr*_nCentForwArr + b*_nCentForwArr + c;
                    if ( (Jpsi.theRapidity >= _rapforwarr[a] && Jpsi.theRapidity < _rapforwarr[a+1]) &&
                         (tmpPt >= _ptforwarr[b] && tmpPt < _ptforwarr[b+1]) &&
                         (Centrality >= _centforwarr[c] && Centrality < _centforwarr[c+1])
                     ) {
                        if (useLxyzCorr==1) {
                          cout << "\t" << feffLxy_LowPt[nidx]->GetName() << endl;
                          theEffLxy = feffLxy_LowPt[nidx]->Eval(lxy);
                          theEffLxyAt0 = feffLxy_LowPt[nidx]->Eval(0);
                        } else if (useLxyzCorr==2) {
                          cout << "\t" << heffLxy_LowPt[nidx]->GetName() << endl;
                          int binnumber = heffLxy_LowPt[nidx]->FindBin(lxy);
                          theEffLxy = heffLxy_LowPt[nidx]->GetBinContent(binnumber);
                          theEffLxyAt0 = heffLxy_LowPt[nidx]->GetBinContent(1);
                        }
                         
                        if (theEffLxy <= 0 || std::isnan(theEffLxy)) {
                          int binnumber = heffLxy_LowPt[nidx]->FindBin(lxy);
                          // Get content from the previous bin
                          while ((heffLxy_LowPt[nidx]->GetBinContent(binnumber)<=0) || (std::isnan(heffLxy_LowPt[nidx]->GetBinContent(binnumber)))) binnumber--;
                          theEffLxy = heffLxy_LowPt[nidx]->GetBinContent(binnumber);
                          cout << "Low eff(Lxyz): " << feffLxy_LowPt[nidx]->Eval(lxy) << " & " << heffLxy_LowPt[nidx]->GetBinContent(binnumber) << " -> " << theEffLxy << endl;
                        }
                        hLxyCtau_LowPt[nidx]->Fill(lxy,Jpsi.theCt);
                    }
                  }
                  } else { // forward & high pT (have different cent array)
                  for (unsigned int c=0; c<_nCentArr; c++) {
                    unsigned int nidx = a*_nPtForwArr*_nCentArr + b*_nCentArr + c;
                    if ( (Jpsi.theRapidity >= _rapforwarr[a] && Jpsi.theRapidity < _rapforwarr[a+1]) &&
                         (tmpPt >= _ptforwarr[b] && tmpPt < _ptforwarr[b+1]) &&
                         (Centrality >= _centarr[c] && Centrality < _centarr[c+1])
                     ) {
                        if (useLxyzCorr==1) {
                          cout << "\t" << feffLxy_LowPt[nidx]->GetName() << endl;
                          theEffLxy = feffLxy_LowPt[nidx]->Eval(lxy);
                          theEffLxyAt0 = feffLxy_LowPt[nidx]->Eval(0);
                        } else if (useLxyzCorr==2) {
                          cout << "\t" << heffLxy_LowPt[nidx]->GetName() << endl;
                          int binnumber = heffLxy_LowPt[nidx]->FindBin(lxy);
                          theEffLxy = heffLxy_LowPt[nidx]->GetBinContent(binnumber);
                          theEffLxyAt0 = heffLxy_LowPt[nidx]->GetBinContent(1);
                        }

                        if (theEffLxy <= 0 || std::isnan(theEffLxy)) {
                          int binnumber = heffLxy_LowPt[nidx]->FindBin(lxy);
                          // Get content from the previous bin
                          while ((heffLxy_LowPt[nidx]->GetBinContent(binnumber)<=0) || (std::isnan(heffLxy_LowPt[nidx]->GetBinContent(binnumber)))) binnumber--;
                          theEffLxy = heffLxy_LowPt[nidx]->GetBinContent(binnumber);
                          cout << "Low eff(Lxyz): " << feffLxy_LowPt[nidx]->Eval(lxy) << " & " << heffLxy_LowPt[nidx]->GetBinContent(binnumber) << " -> " << theEffLxy << endl;
                        }
                        hLxyCtau_LowPt[nidx]->Fill(lxy,Jpsi.theCt);
                    }
                  }
                  } // end of forw & high pt

                }
              }
            }
            if (use3DCtau) cout << "\t" << "lxyz: " << Jpsi.theCt*Jpsi.theP/PDGJpsiM << " theEffPt: " << theEffPt;
            else cout << "\t" << "lxy: " << Jpsi.theCt*Jpsi.thePt/PDGJpsiM << " theEffPt: " << theEffPt;
            cout << " theEffLxy: " << theEffLxy << " theEffLxyAt0: " << theEffLxyAt0 << endl;

            if (useEffDiff) { // Difference 
              theEff = theEffPt - theEffLxyAt0;  // Get difference between PR eff and NP eff (lxy=0) to move a lxy eff curve
              theEff = theEffLxy + theEff;       // Lxy efficiency is moved by the difference between PR and NP efficiencies
            } else { // Ratio
              theEff = theEffLxy / theEffLxyAt0; // Get ratio between NP eff (lxyz) and NP eff (lxyz)
              theEff = theEffPt * theEff;        // Ratio is multiplied to PR eff
            }
            if (useLxyzCorr && theEffLxy <= 0) {           // This event is not going to be included!
              theEff = -1;
              cout << "  theEffLxy is negative " << theEffLxy << endl;
            }

            cout << "\t" << "final eff: " << theEff << endl;

          } else if (tmpPt >= 6.5 && fabs(Jpsi.theRapidity)<1.6) {
            // Pick up a pT eff curve
            for (unsigned int a=0; a<nRapArr; a++) {
              for (unsigned int c=0; c<nCentArr; c++) {
                unsigned int nidx = a*nCentArr + c;
                if ( (Jpsi.theRapidity >= raparr[a] && Jpsi.theRapidity < raparr[a+1]) &&
                     (Centrality >= centarr[c] && Centrality < centarr[c+1])
                   ) {
                    if (useRapPtEff==1) {
                      unsigned int nidx2 = c;
                      if (Jpsi.theRapidity>=0) nidx2 = c + nCentArr;
                      theEffPt = feffRapPt[nidx2]->Eval(Jpsi.theRapidity,tmpPt);
                      cout << "\t" << feffRapPt[nidx2]->GetName() << endl;
                      cout << "\t" << feffPt[nidx]->GetName() << endl;
                      cout << "\t 1DEffPt " << feffPt[nidx]->Eval(tmpPt) << endl;
                    } else if (useRapPtEff==5) {
                      cout << "\t" << feffPt[nidx]->GetName() << endl;
                      cout << "\t 1DEffPt " << feffPt[nidx]->Eval(tmpPt) << endl;
                      int binN = heffPt[nidx]->FindBin(tmpPt);
                      theEffPt = heffPt[nidx]->GetBinContent(binN);
                    } else if (useRapPtEff==6) {
                      cout << "\t" << geffPt[nidx]->GetName() << endl;
                      theEffPt = geffPt[nidx]->Eval(tmpPt);
                    } else {
                      cout << "\t" << feffPt[nidx]->GetName() << endl;
                      theEffPt = feffPt[nidx]->Eval(tmpPt);
                    }
                    if (theEffPt<=0) {
                      heffEmpty[nidx]->Fill(tmpPt);
                      int binnumber = heffPt[nidx]->FindBin(tmpPt);
                      // Get content from the previous bin
                      while (heffPt[nidx]->GetBinContent(binnumber)<=0) binnumber--;
                      theEffPt = heffPt[nidx]->GetBinContent(binnumber);
                      cout << "Low eff(Pt): " << geffPt[nidx]->Eval(tmpPt) << " " << feffPt[nidx]->Eval(tmpPt) << " & " << heffPt[nidx]->GetBinContent(binnumber) << " -> " << theEffPt << endl;
                    }

                }
              }
            }

            if (useLxyzCorr) {
              // Pick up a Lxy eff curve
              for (unsigned int a=0; a<_nRapArr; a++) {
                for (unsigned int b=0; b<_nPtArr; b++) {
                  for (unsigned int c=0; c<_nCentArr; c++) {
                    unsigned int nidx = a*_nPtArr*_nCentArr + b*_nCentArr + c;
                    if ( (Jpsi.theRapidity >= _raparr[a] && Jpsi.theRapidity < _raparr[a+1]) &&
                         (tmpPt >= _ptarr[b] && tmpPt < _ptarr[b+1]) &&
                         (Centrality >= _centarr[c] && Centrality < _centarr[c+1])
                       ) {
                        if (useLxyzCorr==1) {
                          cout << "\t" << feffLxy[nidx]->GetName() << endl;
                          theEffLxy = feffLxy[nidx]->Eval(lxy);
                          theEffLxyAt0 = feffLxy[nidx]->Eval(0);
                        } else if (useLxyzCorr==2) {
                          cout << "\t" << heffLxy[nidx]->GetName() << endl;
                          int binnumber = heffLxy[nidx]->FindBin(lxy);
                          theEffLxy = heffLxy[nidx]->GetBinContent(binnumber);
                          theEffLxyAt0 = heffLxy[nidx]->GetBinContent(1);
                        }
                        
                        if (theEffLxy <= 0 || std::isnan(theEffLxy)) {
                          int binnumber = heffLxy[nidx]->FindBin(lxy);
                          // Get content from the previous bin
                          while ((heffLxy_LowPt[nidx]->GetBinContent(binnumber)<=0) || (std::isnan(heffLxy_LowPt[nidx]->GetBinContent(binnumber)))) binnumber--;
                          theEffLxy = heffLxy[nidx]->GetBinContent(binnumber);
                          cout << "Low eff(Lxyz): " << feffLxy[nidx]->Eval(lxy) << " & " << heffLxy[nidx]->GetBinContent(binnumber) << " -> " << theEffLxy << endl;
                        }
                        hLxyCtau[nidx]->Fill(lxy,Jpsi.theCt);
                    }
                  }
                }
              }
            }
            if (use3DCtau) cout << "\t" << "lxyz: " << Jpsi.theCt*Jpsi.theP/PDGJpsiM << " theEffPt: " << theEffPt;
            else cout << "\t" << "lxy: " << Jpsi.theCt*Jpsi.thePt/PDGJpsiM << " theEffPt: " << theEffPt;
            cout << " theEffLxy: " << theEffLxy << " theEffLxyAt0: " << theEffLxyAt0 << endl;

            if (useEffDiff) { // Difference 
              theEff = theEffPt - theEffLxyAt0;  // Get difference between PR eff and NP eff (lxy=0) to move a lxy eff curve
              theEff = theEffLxy + theEff;       // Lxy efficiency is moved by the difference between PR and NP efficiencies
            } else { // Ratio
              theEff = theEffLxy / theEffLxyAt0; // Get ratio between NP eff (lxyz) and NP eff (lxyz)
              theEff = theEffPt * theEff;        // Ratio is multiplied to PR eff
            }
            if (useLxyzCorr && theEffLxy <= 0) {           // This event is not going to be included!
              theEff = -1;
              cout << "  theEffLxy is negative " << theEffLxy << endl;
            }

            cout << "\t" << "final eff: " << theEff << endl;

          } else {
            theEff = 1.0;
          }

          // Apply single muon tnp scale factors
          if (useTnPCorr==1) {
            double singleMuWeight = 1;
            if (TMath::Abs(m1P->Eta()) < 1.6) singleMuWeight = gSingleMuW[0]->Eval(m1P->Pt());
            else singleMuWeight = gSingleMuW_LowPt[0]->Eval(m1P->Pt());

            if (TMath::Abs(m2P->Eta()) < 1.6) singleMuWeight *= gSingleMuW[0]->Eval(m2P->Pt());
            else singleMuWeight *= gSingleMuW_LowPt[0]->Eval(m2P->Pt());
            
            theEff *= singleMuWeight;
            cout << "\t" << "TnPCorr theEff: " << theEff << endl;
          } else if (useTnPCorr==2 || useTnPCorr==3) {
            double singleMuWeight = 1;
            if (TMath::Abs(m1P->Eta()) < 0.9) {
              singleMuWeight = gSingleMuW[0]->Eval(m1P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA->Eval(m1P->Pt());
            } else if (TMath::Abs(m1P->Eta()) >= 0.9 && TMath::Abs(m1P->Eta()) < 1.6) {
              singleMuWeight = gSingleMuW[1]->Eval(m1P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA->Eval(m1P->Pt());
            } else if (TMath::Abs(m1P->Eta()) >= 1.6 && TMath::Abs(m1P->Eta()) < 2.1) {
              singleMuWeight = gSingleMuW_LowPt[0]->Eval(m1P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA_LowPt->Eval(m1P->Pt());
            } else {
              singleMuWeight = gSingleMuW_LowPt[1]->Eval(m1P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA_LowPt->Eval(m1P->Pt());
            }

            if (TMath::Abs(m2P->Eta()) < 0.9) {
              singleMuWeight *= gSingleMuW[0]->Eval(m2P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA->Eval(m2P->Pt());
            } else if (TMath::Abs(m2P->Eta()) >= 0.9 && TMath::Abs(m2P->Eta()) < 1.6) {
              singleMuWeight *= gSingleMuW[1]->Eval(m2P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA->Eval(m2P->Pt());
            } else if (TMath::Abs(m2P->Eta()) >= 1.6 && TMath::Abs(m2P->Eta()) < 2.1) {
              singleMuWeight *= gSingleMuW_LowPt[0]->Eval(m2P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA_LowPt->Eval(m2P->Pt());
            } else {
              singleMuWeight *= gSingleMuW_LowPt[1]->Eval(m2P->Pt());
              if (useTnPCorr==2) singleMuWeight *= gSingleMuWSTA_LowPt->Eval(m2P->Pt());
            }
            
            theEff *= singleMuWeight;
            cout << "\t" << "TnPCorr theEff: " << theEff << endl;
          }

        } else { theEff = 1.0; }  // end of the weighting condition
        if (theEff>0) Jpsi.theEff = 1.0/theEff;
        else {
          Jpsi.theEff = 0;
          continue;
        }
        if (theEff>100) theEff=100;

        double tmpDiff = TMath::Abs(PDGJpsiM - Jpsi.theMass);
        if (runType == 4) {
          if (Jpsi.mupl_numOfMatch > 1 && Jpsi.mumi_numOfMatch > 1 && diffMass > tmpDiff) {
            diffMass = tmpDiff;
            mostJpsi = Jpsi;
            passMostJpsi = true;
          }
        } else if (runType == 5) {
          if (Jpsi.mupl_numOfMatch > 2 && Jpsi.mumi_numOfMatch > 2 && diffMass > tmpDiff) {
            diffMass = tmpDiff;
            mostJpsi = Jpsi;
            passMostJpsi = true;
          }
        } else {
          JpsiPt->Fill(Jpsi.thePt);
          if (Jpsi.theMass > 2.6 && Jpsi.theMass < 3.5) {
            if (TMath::Abs(Jpsi.theRapidity) > 1.6 && TMath::Abs(Jpsi.theRapidity) < 2.4 &&
                Jpsi.thePt > 3 && Jpsi.thePt < 4.5)
              hJpsiCtau[0]->Fill(Jpsi.theCt);
            else if (TMath::Abs(Jpsi.theRapidity) > 1.6 && TMath::Abs(Jpsi.theRapidity) < 2.4 &&
                Jpsi.thePt > 4.5 && Jpsi.thePt < 5.5)
              hJpsiCtau[1]->Fill(Jpsi.theCt);
            else if (TMath::Abs(Jpsi.theRapidity) > 1.6 && TMath::Abs(Jpsi.theRapidity) < 2.4 &&
                Jpsi.thePt > 5.5 && Jpsi.thePt < 6.5)
              hJpsiCtau[2]->Fill(Jpsi.theCt);
            else if (TMath::Abs(Jpsi.theRapidity) > 1.6 && TMath::Abs(Jpsi.theRapidity) < 2.4 &&
                Jpsi.thePt > 6.5 && Jpsi.thePt < 30)
              hJpsiCtau[3]->Fill(Jpsi.theCt);
            else if (TMath::Abs(Jpsi.theRapidity) > 1.6 && TMath::Abs(Jpsi.theRapidity) < 2.4 &&
                Jpsi.thePt > 3.0 && Jpsi.thePt < 6.5)
              hJpsiCtau[4]->Fill(Jpsi.theCt);
            else if (TMath::Abs(Jpsi.theRapidity) < 1.2 && 
                Jpsi.thePt > 6.5 && Jpsi.thePt < 30)
              hJpsiCtau[5]->Fill(Jpsi.theCt);
            else if (TMath::Abs(Jpsi.theRapidity) < 1.2 && TMath::Abs(Jpsi.theRapidity) < 1.6 &&
                Jpsi.thePt > 6.5 && Jpsi.thePt < 30)
              hJpsiCtau[6]->Fill(Jpsi.theCt);
            else if (TMath::Abs(Jpsi.theRapidity) < 0.0 && TMath::Abs(Jpsi.theRapidity) < 2.4 &&
                Jpsi.thePt > 6.5 && Jpsi.thePt < 30)
              hJpsiCtau[7]->Fill(Jpsi.theCt);
          }
          Jpsi_Pt->setVal(Jpsi.thePt); 
          Jpsi_Y->setVal(Jpsi.theRapidity); 
          Jpsi_Phi->setVal(Jpsi.thePhi);
          Jpsi_dPhi->setVal(Jpsi.thedPhi);
          Jpsi_Mass->setVal(Jpsi.theMass);
          Psip_Mass->setVal(Jpsi.theMass);
          Jpsi_Ct->setVal(Jpsi.theCt);
          Jpsi_Lxyz->setVal(Jpsi.Lxyz);
          Jpsi_CtErr->setVal(Jpsi.theCtErr);
          Jpsi_3DEff->setVal(Jpsi.theEff);
          Jpsi_Type->setIndex(Jpsi.theCat,kTRUE);
          Jpsi_Cent->setVal(Jpsi.theCentrality);
          if (Jpsi.Jq == 0){ Jpsi_Sign->setIndex(Jpsi.Jq,kTRUE); }
          else { Jpsi_Sign->setIndex(Jpsi.Jq,kTRUE); }
  //        Jpsi_CtTrue->setVal(Jpsi.theCtTrue);
  //        MCType->setIndex(Jpsi.genType,kTRUE);

  //        RooArgList varlist_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_Type,*Jpsi_Sign,*MCType,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_CtTrue);
  //        RooArgList varlist2_tmp(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_Type,*Jpsi_Sign,*MCType,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_CtTrue);
  /*        RooArgList varlist_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_Type,*Jpsi_Sign,*Jpsi_Ct,*Jpsi_CtErr);
          RooArgList varlist2_tmp(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_Type,*Jpsi_Sign,*Jpsi_Ct,*Jpsi_CtErr);*/
          // Without weighting
          RooArgList varlist_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
          RooArgList varlistSame_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
          RooArgList varlist2_tmp(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
          // With weighting
          RooArgList varlistW_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
          RooArgList varlistSameW_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
          RooArgList varlist2W_tmp(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);


          if (Jpsi.Jq == 0) {
            if (Jpsi.theMass < 3.5) {
              dataJpsi->add(varlist_tmp);
              if (doWeighting) {
                if (runType==8) {
                  if (randomVar[ev]>0.5)
                    dataJpsiW->add(varlistW_tmp);
                  else
                    dataJpsiW2->add(varlistW_tmp);
                } else {
                  dataJpsiW->add(varlistW_tmp);
                }
              }
              PassingEvent->Fill(1);
            }
          } else {
            if (Jpsi.theMass < 3.5) {
              dataJpsiSame->add(varlist_tmp);
              if (doWeighting) dataJpsiSameW->add(varlistW_tmp);
            }
          }

        } // runType == 4 or 5 condition
      } // End of if() statement for cuts

    } // End of Reco_QQ_size loop

    // Fill up the most J/psi mass closest dimuon per event
    if ((runType == 4 || runType == 5) && passMostJpsi) {
      Jpsi_Pt->setVal(mostJpsi.thePt); 
      Jpsi_Y->setVal(mostJpsi.theRapidity); 
      Jpsi_Phi->setVal(mostJpsi.thePhi);
      Jpsi_Mass->setVal(mostJpsi.theMass);
      Psip_Mass->setVal(mostJpsi.theMass);
      Jpsi_Ct->setVal(mostJpsi.theCt);
      Jpsi_CtErr->setVal(mostJpsi.theCtErr);
      Jpsi_dPhi->setVal(mostJpsi.thedPhi);
      Jpsi_3DEff->setVal(mostJpsi.theEff);
      Jpsi_Type->setIndex(mostJpsi.theCat,kTRUE);
      Jpsi_Cent->setVal(mostJpsi.theCentrality);
      if (mostJpsi.Jq == 0){ Jpsi_Sign->setIndex(mostJpsi.Jq,kTRUE); }
      else { Jpsi_Sign->setIndex(mostJpsi.Jq,kTRUE); }

      // Without weighting
      RooArgList varlist_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
      RooArgList varlistSame_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
      RooArgList varlist2_tmp(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
      // With weighting
      RooArgList varlistW_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
      RooArgList varlistSameW_tmp(*Jpsi_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);
      RooArgList varlist2W_tmp(*Psip_Mass,*Jpsi_Pt,*Jpsi_Y,*Jpsi_dPhi,*Jpsi_Cent,*Jpsi_3DEff,*Jpsi_Ct,*Jpsi_CtErr,*Jpsi_Lxyz);

      if (mostJpsi.Jq == 0) {
        if (mostJpsi.theMass < 3.5) {
          dataJpsi->add(varlist_tmp);
          if (doWeighting) dataJpsiW->add(varlistW_tmp);
          PassingEvent->Fill(1);
        }
      } else {
        if (mostJpsi.theMass < 3.5) {
          dataJpsiSame->add(varlist_tmp);
          if (doWeighting) dataJpsiSameW->add(varlistW_tmp);
        }
      }
      
    } // End of runType == 4 or 5
    passMostJpsi = false;
  } // End of tree event loop

  gROOT->Macro("/home/mihee/rootlogon.C");
  char namefile[200];
  TCanvas *canv = new TCanvas("canv","canv",800,600);
  canv->cd();
  JpsiPt->Draw("text");
  sprintf(namefile,"%s/JpsiPt.pdf",outputDir.c_str());
  canv->SaveAs(namefile);
  canv->Clear();
  for (int nidx=0; nidx<10; nidx++) {
    for (int bin=1; bin<=hJpsiCtau[nidx]->GetNbinsX(); bin++) {
      double width = hJpsiCtau[nidx]->GetBinWidth(bin);
      double normCont = hJpsiCtau[nidx]->GetBinContent(bin) / width;
      hJpsiCtau[nidx]->SetBinContent(bin,normCont);
    }
    canv->SetLogy(1);
    hJpsiCtau[nidx]->Draw();
    sprintf(namefile,"%s/hJpsiCtau_%d.pdf",outputDir.c_str(),nidx);
    canv->SaveAs(namefile);
    canv->Clear();
  }

  TLatex *lat = new TLatex(); lat->SetNDC(); lat->SetTextSize(0.035); lat->SetTextColor(kBlack);
/*  TCanvas *canv2 = new TCanvas("canv2","canv2",600,600);
  canv2->SetLeftMargin(0.15);
  canv2->SetRightMargin(0.15);
  canv2->SetLogz(1);
  for (unsigned int a=0; a<_nRapArr; a++) {
    for (unsigned int b=0; b<_nPtArr; b++) {
      for (unsigned int c=0; c<_nCentArr; c++) {
        if (_raparr[a]==-1.6 && _raparr[a+1]==1.6) continue;
        unsigned int nidx = a*_nPtArr*_nCentArr + b*_nCentArr + c;
        canv2->cd();
        hLxyCtau[nidx]->Draw("colz");
        canv2->Update();
        TPaletteAxis *pal = (TPaletteAxis*)hLxyCtau[nidx]->GetListOfFunctions()->FindObject("palette");
        pal->SetX1NDC(0.86);
        pal->SetX2NDC(0.92);
        pal->SetY1NDC(0.15);
        pal->SetY2NDC(0.93);
        lat->DrawLatex(0.2,0.8,Form("%.1f<y<%.1f, %.1f-%.1f GeV/c, %.0f-%.0f%%",
              _raparr[a],_raparr[a+1],_ptarr[b],_ptarr[b+1],_centarr[c]*2.5,_centarr[c+1]*2.5));
        canv2->Update();
        canv2->SaveAs(Form("%s/%s.png",outputDir.c_str(),hLxyCtau[nidx]->GetName()));
        canv2->Clear();
      }
    }
  }
  for (unsigned int a=0; a<_nRapForwArr; a++) {
    for (unsigned int b=0; b<_nPtForwArr; b++) {
      for (unsigned int c=0; c<_nCentForwArr; c++) {
        unsigned int nidx = a*_nPtForwArr*_nCentForwArr + b*_nCentForwArr + c;
        if (_rapforwarr[a]==-1.6 && _rapforwarr[a+1]==1.6) continue;
        canv2->cd();
        hLxyCtau_LowPt[nidx]->Draw("colz");
        canv2->Update();
        TPaletteAxis *pal = (TPaletteAxis*)hLxyCtau_LowPt[nidx]->GetListOfFunctions()->FindObject("palette");
        pal->SetX1NDC(0.86);
        pal->SetX2NDC(0.92);
        pal->SetY1NDC(0.15);
        pal->SetY2NDC(0.93);
        lat->DrawLatex(0.2,0.8,Form("%.1f<y<%.1f, %.1f-%.1f GeV/c, %.1f-%.1f%%",
              _rapforwarr[a],_rapforwarr[a+1],_ptforwarr[b],_ptforwarr[b+1],_centforwarr[c]*2.5,_centforwarr[c+1]*2.5));
        canv2->Update();
        canv2->SaveAs(Form("%s/%s.png",outputDir.c_str(),hLxyCtau_LowPt[nidx]->GetName()));
        canv2->Clear();
      }
    }
  }

  for (int a=0; a<4; a++) {
    canv2->cd();
    hLxyCtau2[a]->Draw("colz");
    canv2->Update();
    TPaletteAxis *pal = (TPaletteAxis*)hLxyCtau2[a]->GetListOfFunctions()->FindObject("palette");
    pal->SetX1NDC(0.86);
    pal->SetX2NDC(0.92);
    pal->SetY1NDC(0.15);
    pal->SetY2NDC(0.93);
    canv2->SaveAs(Form("%s/hLxyCtau2_%d.png",outputDir.c_str(),a));
    canv2->Clear();
  }
*/

  if (doWeighting) {
    for (unsigned int a=0; a<nRapArr; a++) {
      for (unsigned int c=0; c<nCentArr; c++) {
        unsigned int nidx = a*nCentArr + c;
        if (raparr[a]==-1.6 && raparr[a+1]==1.6) continue;

        if (heffEmpty[nidx]->GetEntries() != 0) {
          canv->cd();
          heffEmpty[nidx]->Draw("text l");
          sprintf(namefile,"%s/EmptyEff_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d.pdf",outputDir.c_str(),raparr[a],raparr[a+1],centarr[c],centarr[c+1]);
          lat->DrawLatex(0.45,0.8,Form("Rap%.1f-%.1f_Cent%.0f-%.0f",raparr[a],raparr[a+1],centarr[c]*2.5,centarr[c+1]*2.5));
          canv->SaveAs(namefile);
          canv->Clear();
        }
      }
    }
      
/*    for (unsigned int a=0; a<nRapForwArr; a++) {
      for (unsigned int c=0; c<nCentForwArr; c++) {
        unsigned int nidx = a*nCentForwArr + c;
        if (rapforwarr[a]==-1.6 && rapforwarr[a+1]==1.6) continue;

        if (heffEmpty_LowPt[nidx]->GetEntries() != 0) {
          canv->cd();
          heffEmpty_LowPt[nidx]->Draw("text l");
          sprintf(namefile,"%s/EmptyEff_Rap%.1f-%.1f_Pt6.5-30.0_Cent%d-%d.pdf",outputDir.c_str(),rapforwarr[a],rapforwarr[a+1],centforwarr[c],centforwarr[c+1]);
          lat->DrawLatex(0.45,0.8,Form("Rap%.1f-%.1f_Cent%.0f-%.0f",rapforwarr[a],rapforwarr[a+1],centforwarr[c]*2.5,centforwarr[c+1]*2.5));
          canv->SaveAs(namefile);
          canv->Clear();
        }
      }
    } */
  }

  // Perform the weighting on the dataset
  if (doWeighting) {
    dataJpsiWeight = new RooDataSet("dataJpsiWeight","A sample",*dataJpsiW->get(),Import(*dataJpsiW),WeightVar(*Jpsi_3DEff));
    if (runType==8) {
      dataJpsiWeight2 = new RooDataSet("dataJpsiWeight2","A sample",*dataJpsiW2->get(),Import(*dataJpsiW2),WeightVar(*Jpsi_3DEff));
    }
    dataJpsiSameWeight = new RooDataSet("dataJpsiSameWeight","A sample",*dataJpsiSameW->get(),Import(*dataJpsiSameW),WeightVar(*Jpsi_3DEff));
  }

  /// *** Fill TFiles with RooDataSet
  TFile* Out;
  sprintf(namefile,"%s/%s.root",outputDir.c_str(),outputDir.c_str());
  Out = new TFile(namefile,"RECREATE");
  Out->cd();
  dataJpsi->Write();
  dataJpsiSame->Write();
  if (doWeighting) {
    dataJpsiW->Write();
    dataJpsiSameW->Write();
    dataJpsiWeight->Write();
    dataJpsiSameWeight->Write();
    if (runType==8) {
      dataJpsiW2->Write();
      dataJpsiWeight2->Write();
    }
  }


  Out->Close();
  delete [] randomVar;

  cout << "PassingEvent: " << PassingEvent->GetEntries() << endl;
  delete PassingEvent;

  if (useTnPCorr==1) {
    delete gSingleMuW[0];
    delete gSingleMuW_LowPt[0];
  } else if (useTnPCorr==2) {
    delete gSingleMuW[0];
    delete gSingleMuW[1];
    delete gSingleMuW_LowPt[0];
    delete gSingleMuW_LowPt[1];
  }

  return 0;

}




////////// sub-routines
double reducedPhi(double thedPhi) {
  if(thedPhi < -TMath::Pi()) thedPhi += 2.*TMath::Pi();
  if(thedPhi > TMath::Pi()) thedPhi -= 2.*TMath::Pi();
  if(thedPhi < -TMath::Pi()/2) thedPhi +=TMath::Pi();
  if(thedPhi > TMath::Pi()/2) thedPhi -=TMath::Pi();

  return thedPhi;
}

bool isAccept(const TLorentzVector* aMuon) {
  if (fabs(aMuon->Pt()) > 2.5) return true;
  else return false;
}

bool isMuonInAccept(const TLorentzVector *aMuon) {
  return (fabs(aMuon->Eta()) < 2.4 &&
         ((fabs(aMuon->Eta()) < 1.0 && aMuon->Pt() >= 3.4) ||
         (1.0 <= fabs(aMuon->Eta()) && fabs(aMuon->Eta()) < 1.5 && aMuon->Pt() >= 5.8-2.4*fabs(aMuon->Eta())) ||
         (1.5 <= fabs(aMuon->Eta()) && aMuon->Pt() >= 3.3667-7.0/9.0*fabs(aMuon->Eta()))));
}


bool checkRunType(const struct Condition Jpsi, const TLorentzVector* m1P, const TLorentzVector* m2P) {
  if (runType == 1) {
    if (Jpsi.mupl_nMuValHits > 12 && Jpsi.mumi_nMuValHits > 12) return true;
    else return false;
  }
  else if (runType == 2) {
//    if (isAccept(m1P) || isAccept(m2P)) return true;
    if (isMuonInAccept(m1P) && isMuonInAccept(m2P)) return true;
    else return false;
  }
  else if (runType == 3) {
    if (fabs(Jpsi.zVtx) < 10.0) return true;
    else return false;
  }
  else if (runType == 4 || runType == 5) {
    return true;
  }
  else if (runType == 6) {
    bool Matches      = Jpsi.mupl_numOfMatch > 1 && Jpsi.mumi_numOfMatch > 1;
    bool InnerChiMeas = (Jpsi.mupl_norChi2_inner/Jpsi.mupl_nTrkWMea < 0.15) &&
                        (Jpsi.mumi_norChi2_inner/Jpsi.mumi_nTrkWMea < 0.15);
    bool MuHits       = Jpsi.mupl_nMuValHits > 12 && Jpsi.mumi_nMuValHits > 12; 
    bool TrkHits      = Jpsi.mupl_nTrkHits > 12 && Jpsi.mumi_nTrkHits > 12;
    bool GlobalChi    = (Jpsi.mupl_norChi2_global/(Jpsi.mupl_nMuValHits+Jpsi.mupl_nTrkHits) < 0.15) &&
                        (Jpsi.mumi_norChi2_global/(Jpsi.mumi_nMuValHits+Jpsi.mumi_nTrkHits) < 0.15);

    if (TrkHits && Matches && InnerChiMeas && MuHits && GlobalChi) return true;
    else return false;
  }
  else if (runType == 7) {
    if (m1P->Pt() > 4.0 && m2P->Pt() > 4.0) return true;
    else return false;
  }
  else if (runType == 9) {
    return true;
  }

  return true;
}

bool checkTriggers(const struct Condition Jpsi, bool cowboy, bool sailor) {
  bool singleMu = false, doubleMu = false;
  bool triggerCondition = false;
  if ( ( (Jpsi.HLTriggers&1)==1 && (Jpsi.Reco_QQ_trig&1)==1 ) ||
       ( (Jpsi.HLTriggers&2)==2 && (Jpsi.Reco_QQ_trig&2)==2 ) ||
       ( (Jpsi.HLTriggers&4)==4 && (Jpsi.Reco_QQ_trig&4)==4 ) ||
       ( (Jpsi.HLTriggers&8)==8 && (Jpsi.Reco_QQ_trig&8)==8 ) ) {
/*  if ( ( Jpsi.Reco_QQ_trig&1)==1 ||
       ( Jpsi.Reco_QQ_trig&2)==2 ||
       ( Jpsi.Reco_QQ_trig&4)==4 ||
       ( Jpsi.Reco_QQ_trig&8)==8 ) {*/
    doubleMu = true;
  } else { doubleMu = false; }

  if ( ( (Jpsi.HLTriggers&16)==16 && (Jpsi.Reco_QQ_trig&16)==16 ) ||
       ( (Jpsi.HLTriggers&32)==32 && (Jpsi.Reco_QQ_trig&32)==32 ) ||
       ( (Jpsi.HLTriggers&64)==64 && (Jpsi.Reco_QQ_trig&64)==64 ) ||
       ( (Jpsi.HLTriggers&128)==128 && (Jpsi.Reco_QQ_trig&128)==128 ) ) {
/*  if ( ( Jpsi.Reco_QQ_trig&16)==16 ||
       ( Jpsi.Reco_QQ_trig&32)==32 ||
       ( Jpsi.Reco_QQ_trig&64)==64 ||
       ( Jpsi.Reco_QQ_trig&128)==128 ) {*/
    singleMu = true;
  } else { singleMu = false; }

  if (trigType == 0) { //nominal case
    if ( singleMu || doubleMu ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==1) { //HLT_HIL1DoubleMu0_HighQ, cowboy
    if ( (Jpsi.HLTriggers&1)==1 && (Jpsi.Reco_QQ_trig&1)==1 && cowboy) { triggerCondition = true; }
//    if ( (Jpsi.Reco_QQ_trig&1)==1 && cowboy) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==2) {  //HLT_HIL1DoubleMu0_HighQ, sailor
    if ( (Jpsi.HLTriggers&1)==1 && (Jpsi.Reco_QQ_trig&1)==1 && sailor ) { triggerCondition = true; }
//    if ( (Jpsi.Reco_QQ_trig&1)==1 && sailor ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==3) {  //HLT_HIL1DoubleMu0_HighQ
    if ( (Jpsi.HLTriggers&1)==1 && (Jpsi.Reco_QQ_trig&1)==1 ) { triggerCondition = true; }
//    if ( (Jpsi.Reco_QQ_trig&1)==1 ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==4) {  //HLT_HIL2DoubleMu3
    if ( (Jpsi.HLTriggers&2)==2 && (Jpsi.Reco_QQ_trig&2)==2 ) { triggerCondition = true; }
//    if ( (Jpsi.Reco_QQ_trig&2)==2 ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if ( trigType ==5 ) { // Obsoleted!!!!!!!!!!!
      triggerCondition = true;

  } else if (trigType ==6) {  //Single muon triggers only
    if ( singleMu ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==7) {  //Single muon triggers only & cowboy
    if ( singleMu && cowboy ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==8) {  //Single muon triggers only & sailor
    if ( singleMu && sailor ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==9) {  //one of the single muon trig && one of the double muon trig
    if (singleMu && doubleMu ) { triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==10) {  // HLT_HIL1DoubleMu0_NHitQ || HLT_HIL2DoubleMu3 || HLT_HIL3DoubleMuOpen_Mgt2_OS_NoCowboy
    if ( ( (Jpsi.HLTriggers&1)==1 && (Jpsi.Reco_QQ_trig&1)==1 ) ||
         ( (Jpsi.HLTriggers&2)==2 && (Jpsi.Reco_QQ_trig&2)==2 ) ||
         ( (Jpsi.HLTriggers&8)==8 && (Jpsi.Reco_QQ_trig&8)==8 ) ) {
/*    if ( ( (Jpsi.Reco_QQ_trig&1)==1 ) ||
         ( (Jpsi.Reco_QQ_trig&2)==2 ) ||
         ( (Jpsi.Reco_QQ_trig&8)==8 ) ) {*/
      triggerCondition = true; }
    else { triggerCondition = false; }

  } else if (trigType ==11) {  // HLT_HIL3DoubleMuOpen_Mgt2_OS_NoCowboy
    if ( (Jpsi.HLTriggers&8)==8 && (Jpsi.Reco_QQ_trig&8)==8 ) {
//    if ( (Jpsi.Reco_QQ_trig&8)==8 ) {
      triggerCondition = true; }
    else { triggerCondition = false; }

  } else {
    cout << "Not valid trigType!\n";
    triggerCondition = false;
  }

  return triggerCondition;
}
