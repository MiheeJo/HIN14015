#include "lJpsiEff.h"
#include "binArrays.h"

void LxyEff_diff3D(bool absRapidity=true, bool logy=false, bool isPbPb=false) {
  gROOT->Macro("/home/mihee/cms/RegIt_JpsiRaa/Efficiency/pp/root604/RegionsDividedInEta_noTnPCorr/JpsiStyle.C");
  
  TFile *NPOut = TFile::Open("./NPMC_eff.root","read");
  TFile *PROut = TFile::Open("./PRMC_eff.root","read");

  TLatex *lat = new TLatex(); lat->SetNDC(kTRUE); lat->SetTextSize(0.04);
   
  double _ymin=yarray[0]; double _ymax=yarray[nbinsy-1];
  double _ptmin=ptarray[0]; double _ptmax=ptarray[nbinspt-1];
  int _centmin=centarray[0]; int _centmax=centarray[nbinscent-1];
  
  const int tnum = (nbinsy-1)*(nbinspt-1)*(nbinscent-1);
  TH1D *hNPEff[tnum];
  TH1D *hPREff[tnum];
  TH1D *hPRt = new TH1D("hPRt","",1,0,1);
  TH1D *hNPt = new TH1D("hNPt","",1,0,1);
  SetHistStyle(hPRt,0,1,0.0,1.3);
  SetHistStyle(hNPt,0,0,0.0,1.3);
  hPRt->SetMarkerColor(kBlack);
  hNPt->SetMarkerColor(kBlack);
  
  TH1D *ghost = new TH1D("ghost","",1,0,1);
  ghost->SetMarkerColor(kWhite);
  ghost->SetMarkerStyle(kOpenCircle);
  ghost->SetLineColor(kWhite);

  for (int a=0; a<nbinsy-1; a++) {
    double ymin=yarray[a]; double ymax=yarray[a+1];
    TCanvas *canvNP = new TCanvas("canvNP","c",600,600);
    canvNP->Draw();
    if (logy) canvNP->SetLogy(1);
    
    TLegend *leg = new TLegend(0.13,0.15,0.5,0.82);
    SetLegendStyle(leg);
    leg->AddEntry(hNPt,Form("Non-prompt"),"pe");
/*    if (nbinspt < 7 && nbinspt >= 4) {
      leg->SetY1(0.45);
    } else if (nbinspt < 4) {
      leg->SetY1(0.55);
    }*/
   
    for (int c=0; c<nbinscent-1; c++) {
      int centmin=centarray[c]; int centmax=centarray[c+1];
      
      for (int b=0; b<nbinspt-1; b++) {
        double ptmin=ptarray[b]; double ptmax=ptarray[b+1];
        int i = a*(nbinspt-1)*(nbinscent-1) + b*(nbinscent-1) + c;

        string className = "NPJpsi";
        hNPEff[i] = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        className = "PRJpsi";
        hPREff[i] = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        if (logy) {
          SetHistStyle(hNPEff[i],b,c,1E-3,5.3);
          SetHistStyle(hPREff[i],b,c,1E-3,5.3);
        } else {
          SetHistStyle(hNPEff[i],b,c,0,1.3);
          SetHistStyle(hPREff[i],b,c,0,1.3);
        }

        if (b==0 && c==0) {
          hNPEff[i]->Draw();
        } else {
          hNPEff[i]->Draw("same");
        }

        std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
        if (b==0) {
          if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
          else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
          lat->DrawLatex(0.15,0.85,testStr.second.c_str());
        }
        leg->AddEntry(hNPEff[i],Form("p_{T} %.1f-%.1f, %.0f-%.0f%%",ptmin,ptmax,centmin*2.5,centmax*2.5),"pe");
        
      } // end of pt loop plotting

    } // end of cent loop plotting
    
    canvNP->SaveAs(Form("./Diff_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,_ptmin,_ptmax,_centmin,_centmax));
    canvNP->SaveAs(Form("./Diff_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,_ptmin,_ptmax,_centmin,_centmax));
    
    canvNP->Clear();
    leg->Draw();
    canvNP->SaveAs(Form("./DiffLegend_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,_ptmin,_ptmax,_centmin,_centmax));
    canvNP->SaveAs(Form("./DiffLegend_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,_ptmin,_ptmax,_centmin,_centmax));

    delete leg;
    delete canvNP;
  } // end of y loop plotting
  
  NPOut->Close();
  PROut->Close();

}


void LxyEff_diffPt(bool absRapidity=true, bool logy=false, bool isPbPb=false) {
  gROOT->Macro("/home/mihee/cms/RegIt_JpsiRaa/Efficiency/pp/root604/RegionsDividedInEta_noTnPCorr/JpsiStyle.C");
  
  TFile *NPOut = TFile::Open("./NPMC_eff.root","read");
  TFile *PROut = TFile::Open("./PRMC_eff.root","read");

  TLatex *lat = new TLatex(); lat->SetNDC(kTRUE);
  TLegend *leg = new TLegend(0.13,0.15,0.5,0.82);
  SetLegendStyle(leg);
  if (nbinspt < 7 && nbinspt >= 4) {
    leg->SetY1(0.45);
  } else if (nbinspt < 4) {
    leg->SetY1(0.55);
  }
    
  TCanvas *canvNP = new TCanvas("canvNP","c",600,600);
  canvNP->Draw();
  canvNP->SetLogy(0);

  double _ymin=yarray[0]; double _ymax=yarray[nbinsy-1];
  double _ptmin=ptarray[0]; double _ptmax=ptarray[nbinspt-1];
  int _centmin=centarray[0]; int _centmax=centarray[nbinscent-1];
  
  TH1D *hNPEff[nbinspt-1];
  TH1D *hPREff[nbinspt-1];
  TH1D *hPRt = new TH1D("hPRt","",1,0,1);
  TH1D *hNPt = new TH1D("hNPt","",1,0,1);
  SetHistStyle(hPRt,0,1,0,1.3);
  SetHistStyle(hNPt,0,0,0,1.3);
  hPRt->SetMarkerColor(kBlack);
  hNPt->SetMarkerColor(kBlack);

  TH1D *ghost = new TH1D("ghost","",1,0,1);
  ghost->SetMarkerColor(kWhite);
  ghost->SetMarkerStyle(kOpenCircle);
  ghost->SetLineColor(kWhite);

  leg->AddEntry(hPRt,Form("Prompt"),"pe");
  leg->AddEntry(hNPt,Form("Non-prompt"),"pe");
  leg->AddEntry(ghost,"PR eff, NP eff");
  
  for (int i=0; i<nbinspt-1; i++) {
    double ymin=yarray[0]; double ymax=yarray[nbinsy-1];
    double ptmin=ptarray[i]; double ptmax=ptarray[i+1];
    int centmin=centarray[0]; int centmax=centarray[nbinscent-1];

    string className = "NPJpsi";
    hNPEff[i] = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
    className = "PRJpsi";
    hPREff[i] = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
    SetHistStyle(hNPEff[i],i,0,0,1.3);
    SetHistStyle(hPREff[i],i,1,0,1.3);
    
    double hNPEffAvg = getAvgEffInRapPt(hNPEff[i],-0.1,0.25);
    double hPREffAvg = getAvgEffInRapPt(hPREff[i],-0.1,0.1);
/*    double yaxismin = 1, yaxismax;
    int xaxisa = hNPEff[i]->FindBin(0);
    int xaxisb = hNPEff[i]->FindBin(0.5);
    for (int mini=xaxisa; mini<xaxisb; mini++) {
      if (yaxismin > hNPEff[i]->GetBinContent(mini))
        yaxismin = hNPEff[i]->GetBinContent(mini);
    }
    hNPEff[i]->GetYaxis()->SetRangeUser(yaxismin*0.6,1.);
    hPREff[i]->GetYaxis()->SetRangeUser(yaxismin*0.6,1.);
    hNPEff[i]->GetXaxis()->SetRangeUser(-0.3,0.5);
    hPREff[i]->GetXaxis()->SetRangeUser(-0.3,0.5);
*/
    if (i==0) {
      hNPEff[i]->Draw();
      hPREff[i]->Draw("same");
    } else {
      hNPEff[i]->Draw("same");
      hPREff[i]->Draw("same");
    }

    std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
    if (i==0) {
      if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
      else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
      lat->DrawLatex(0.15,0.85,testStr.second.c_str());
      if (isPbPb) lat->DrawLatex(0.6,0.85,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
      lat->DrawLatex(0.6,0.80,"PR Eff < 0.1 mm");
      lat->DrawLatex(0.6,0.75,"NP Eff < 0.3 mm");
    }
    leg->AddEntry(hNPEff[i],Form("%s",testStr.first.c_str()),"pe");
    leg->AddEntry(ghost,Form("%.1f %, %.1f %",hPREffAvg*100,hNPEffAvg*100),"");
    
  } // end of rap loop plotting
  
  canvNP->SaveAs(Form("./PtLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
  canvNP->SaveAs(Form("./PtLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
  
  canvNP->Clear();
  leg->Draw();
  canvNP->SaveAs(Form("./PtLxyLegend_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
  
  delete canvNP;
  NPOut->Close();
  PROut->Close();

}

void LxyEff_diffRap(bool absRapidity=true, bool logy=false, bool isPbPb=false) {
  gROOT->Macro("/home/mihee/cms/RegIt_JpsiRaa/Efficiency/pp/root604/RegionsDividedInEta_noTnPCorr/JpsiStyle.C");
  
  TFile *NPOut = TFile::Open("./NPMC_eff.root","read");
  TFile *PROut = TFile::Open("./PRMC_eff.root","read");

  TLatex *lat = new TLatex(); lat->SetNDC(kTRUE);
  TLegend *leg = new TLegend(0.13,0.15,0.5,0.82);
  SetLegendStyle(leg);
  if (nbinsy < 7 && nbinsy >= 4) {
    leg->SetY1(0.45);
  } else if (nbinsy < 4) {
    leg->SetY1(0.55);
  }
    
  TCanvas *canvNP = new TCanvas("canvNP","c",600,600);
  canvNP->Draw();
  canvNP->SetLogy(0);

  double _ymin=yarray[0]; double _ymax=yarray[nbinsy-1];
  double _ptmin=ptarray[0]; double _ptmax=ptarray[nbinspt-1];
  int _centmin=centarray[0]; int _centmax=centarray[nbinscent-1];
  
  TH1D *hNPEff[nbinsy-1];
  TH1D *hPREff[nbinsy-1];
  TH1D *hPRt = new TH1D("hPRt","",1,0,1);
  TH1D *hNPt = new TH1D("hNPt","",1,0,1);
  SetHistStyle(hPRt,0,1,0,1.3);
  SetHistStyle(hNPt,0,0,0,1.3);
  hPRt->SetMarkerColor(kBlack);
  hNPt->SetMarkerColor(kBlack);

  TH1D *ghost = new TH1D("ghost","",1,0,1);
  ghost->SetMarkerColor(kWhite);
  ghost->SetMarkerStyle(kOpenCircle);
  ghost->SetLineColor(kWhite);

  leg->AddEntry(hPRt,Form("Prompt"),"pe");
  leg->AddEntry(hNPt,Form("Non-prompt"),"pe");
  leg->AddEntry(ghost,"PR eff, NP eff");
  
  for (int i=0; i<nbinsy-1; i++) {
    double ymin=yarray[i]; double ymax=yarray[i+1];
    double ptmin=ptarray[0]; double ptmax=ptarray[nbinspt-1];
    int centmin=centarray[0]; int centmax=centarray[nbinscent-1];

    string className = "NPJpsi";
    hNPEff[i] = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
    className = "PRJpsi";
    hPREff[i] = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
    SetHistStyle(hNPEff[i],i,0,0,1.3);
    SetHistStyle(hPREff[i],i,1,0,1.3);

    double hNPEffAvg = getAvgEffInRapPt(hNPEff[i],-0.1,0.25);
    double hPREffAvg = getAvgEffInRapPt(hPREff[i],-0.1,0.1);
/*    double yaxismin = 1, yaxismax;
    int xaxisa = hNPEff[i]->FindBin(0);
    int xaxisb = hNPEff[i]->FindBin(0.5);
    for (int mini=xaxisa; mini<xaxisb; mini++) {
      if (yaxismin > hNPEff[i]->GetBinContent(mini))
        yaxismin = hNPEff[i]->GetBinContent(mini);
    }
    hNPEff[i]->GetYaxis()->SetRangeUser(yaxismin*0.6,1);
    hPREff[i]->GetYaxis()->SetRangeUser(yaxismin*0.6,1);
    hNPEff[i]->GetXaxis()->SetRangeUser(-0.3,0.5);
    hPREff[i]->GetXaxis()->SetRangeUser(-0.3,0.5);
*/
    if (i==0) {
      canvNP->cd();
      hNPEff[i]->Draw();
      hPREff[i]->Draw("same");
    } else {
      canvNP->cd();
      hNPEff[i]->Draw("same");
      hPREff[i]->Draw("same");
    }

    std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
    canvNP->cd();
    if (i==0) {
      if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
      else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
      lat->DrawLatex(0.15,0.85,testStr.first.c_str());
      if (isPbPb) lat->DrawLatex(0.6,0.85,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
      lat->DrawLatex(0.6,0.80,"PR Eff < 0.1 mm");
      lat->DrawLatex(0.6,0.75,"NP Eff < 0.3 mm");
    }
    leg->AddEntry(hNPEff[i],Form("%s",testStr.second.c_str()),"pe");
    leg->AddEntry(ghost,Form("%.1f %, %.1f %",hPREffAvg*100,hNPEffAvg*100),"");
    
  } // end of rap loop plotting
  
  canvNP->cd();
  canvNP->SaveAs(Form("./RapLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
  canvNP->SaveAs(Form("./RapLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
  
  canvNP->Clear();
  leg->Draw();
  canvNP->SaveAs(Form("./RapLxyLegend_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));

  delete canvNP;
  NPOut->Close();
  PROut->Close();

}


void LxyEff_diffCent(bool absRapidity=true, bool logy=false, bool isPbPb=false) {
  gROOT->Macro("/home/mihee/cms/RegIt_JpsiRaa/Efficiency/pp/root604/RegionsDividedInEta_noTnPCorr/JpsiStyle.C");
  
  TFile *NPOut = TFile::Open("./NPMC_eff.root","read");
  TFile *PROut = TFile::Open("./PRMC_eff.root","read");

  TLatex *lat = new TLatex(); lat->SetNDC(kTRUE);
  TLegend *leg = new TLegend(0.13,0.15,0.5,0.82);
  SetLegendStyle(leg);
  if (nbinscent < 7 && nbinscent >= 4) {
    leg->SetY1(0.45);
  } else if (nbinscent < 4) {
    leg->SetY1(0.55);
  }
    
  TCanvas *canvNP = new TCanvas("canvNP","c",600,600);
  canvNP->Draw();
  canvNP->SetLogy(0);

  double _ymin=yarray[0]; double _ymax=yarray[nbinsy-1];
  double _ptmin=ptarray[0]; double _ptmax=ptarray[nbinspt-1];
  int _centmin=centarray[0]; int _centmax=centarray[nbinscent-1];
  
  TH1D *hNPEff[nbinsy-1];
  TH1D *hPREff[nbinsy-1];
  TH1D *hPRt = new TH1D("hPRt","",1,0,1);
  TH1D *hNPt = new TH1D("hNPt","",1,0,1);
  SetHistStyle(hPRt,0,1,0,1.3);
  SetHistStyle(hNPt,0,0,0,1.3);
  hPRt->SetMarkerColor(kBlack);
  hNPt->SetMarkerColor(kBlack);

  TH1D *ghost = new TH1D("ghost","",1,0,1);
  ghost->SetMarkerColor(kWhite);
  ghost->SetMarkerStyle(kOpenCircle);
  ghost->SetLineColor(kWhite);

  leg->AddEntry(hPRt,Form("Prompt"),"pe");
  leg->AddEntry(hNPt,Form("Non-prompt"),"pe");
  leg->AddEntry(ghost,"PR eff, NP eff");
  
  for (int i=0; i<nbinscent-1; i++) {
    double ymin=yarray[0]; double ymax=yarray[nbinsy-1];
    double ptmin=ptarray[0]; double ptmax=ptarray[nbinspt-1];
    int centmin=centarray[i]; int centmax=centarray[i+1];

    string className = "NPJpsi";
    hNPEff[i] = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
    className = "PRJpsi";
    hPREff[i] = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
    SetHistStyle(hNPEff[i],i,0,0,1.3);
    SetHistStyle(hPREff[i],i,1,0,1.3);

    double hNPEffAvg = getAvgEffInRapPt(hNPEff[i],-0.1,0.25);
    double hPREffAvg = getAvgEffInRapPt(hPREff[i],-0.1,0.1);
/*    double yaxismin = 1, yaxismax;
    int xaxisa = hNPEff[i]->FindBin(0);
    int xaxisb = hNPEff[i]->FindBin(0.5);
    for (int mini=xaxisa; mini<xaxisb; mini++) {
      if (yaxismin > hNPEff[i]->GetBinContent(mini))
        yaxismin = hNPEff[i]->GetBinContent(mini);
    }
    hNPEff[i]->GetYaxis()->SetRangeUser(yaxismin*0.6,1);
    hPREff[i]->GetYaxis()->SetRangeUser(yaxismin*0.6,1);
    hNPEff[i]->GetXaxis()->SetRangeUser(-0.3,0.5);
    hPREff[i]->GetXaxis()->SetRangeUser(-0.3,0.5);
*/
    if (i==0) {
      canvNP->cd();
      hNPEff[i]->Draw();
      hPREff[i]->Draw("same");
    } else {
      canvNP->cd();
      hNPEff[i]->Draw("same");
      hPREff[i]->Draw("same");
    }

    std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
    canvNP->cd();
    if (i==0) {
      if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
      else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
      lat->DrawLatex(0.15,0.85,testStr.second.c_str());
      lat->DrawLatex(0.6,0.85,testStr.first.c_str());
      lat->DrawLatex(0.6,0.80,"PR Eff < 0.1 mm");
      lat->DrawLatex(0.6,0.75,"NP Eff < 0.3 mm");
    }
    leg->AddEntry(hNPEff[i],Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5),"pe");
    leg->AddEntry(ghost,Form("%.1f %, %.1f %",hPREffAvg*100,hNPEffAvg*100),"");
    
  } // end of cent loop plotting
  
  canvNP->cd();
  canvNP->SaveAs(Form("./CentLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
  canvNP->SaveAs(Form("./CentLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));

  canvNP->Clear();
  leg->Draw();
  canvNP->SaveAs(Form("./CentLxyLegend_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
  
  delete canvNP;
  NPOut->Close();
  PROut->Close();

}



void LxyEff_all (bool absRapidity=true, bool logy=false, bool isPbPb=false) {
  gROOT->Macro("/home/mihee/cms/RegIt_JpsiRaa/Efficiency/pp/root604/RegionsDividedInEta_noTnPCorr/JpsiStyle.C");
  
  TFile *NPOut = TFile::Open("./NPMC_eff.root","read");
  TFile *PROut = TFile::Open("./PRMC_eff.root","read");

  TLatex *lat = new TLatex(); lat->SetNDC(kTRUE);
  TLegend *leg = new TLegend(0.13,0.14,0.5,0.23);
  SetLegendStyle(leg);
  TH1D *hGenT = new TH1D("hGenT","",1,0,1);
  TH1D *hGenT2 = new TH1D("hGenT2","",1,0,1);
  TH1D *hGenT3 = new TH1D("hGenT3","",1,0,1);
  TH1D *hGenT4 = new TH1D("hGenT4","",1,0,1);
  TH1D *hRecT = new TH1D("hRecT","",1,0,1);
  SetHistStyle(hGenT2,1,0,0,1);
  SetHistStyle(hGenT3,3,1,0,1);
  SetHistStyle(hGenT4,5,1,0,1);
  SetHistStyle(hRecT,0,0,0,1);
  leg->AddEntry(hGenT2,"Gen dimuon","lp");
  leg->AddEntry(hRecT,"Reco dimuon","lp");

  TLegend *legA = new TLegend(0.13,0.64,0.5,0.73);
  SetLegendStyle(legA);
  SetHistStyle(hGenT,3,1,0,1);
  legA->AddEntry(hGenT,"Prompt J/#psi","lp");
  legA->AddEntry(hRecT,"Non-prompt J/#psi","lp");


  // All integrated y and pT efficiency isn't done with TGraph
  double _ymin=yarray[0]; double _ymax=yarray[nbinsy-1];
  double _ptmin=ptarray[0]; double _ptmax=ptarray[nbinspt-1];
  int _centmin=centarray[0]; int _centmax=centarray[nbinscent-1];

//  TH1D *hNPEffA = (TH1D*)NPOut->Get(Form("hEffLxy1D_NPJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
//  TH1D *hPREffA = (TH1D*)PROut->Get(Form("hEffLxy1D_PRJpsi_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
//  SetHistStyle(hNPEffA,0,0,0,1.3);
//  SetHistStyle(hPREffA,3,1,0,1.3);
//  
//  TCanvas *can = new TCanvas("c","c",600,600);
//  can->SetLogy(0);
//  hNPEffA->Draw();
//  hPREffA->Draw("same");
//
//  std::pair< string, string > _testStr = FillLatexInfo(_ymin, _ymax, _ptmin, _ptmax, absRapidity);
//  if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
//  else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
//  lat->DrawLatex(0.15,0.85,_testStr.second.c_str());
//  lat->DrawLatex(0.15,0.80,_testStr.first.c_str());
//  if (isPbPb) lat->DrawLatex(0.15,0.75,Form("Cent. %.0f-%.0f%%",_centmin*2.5,_centmax*2.5));
//  legA->Draw();
//
//  can->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
//  can->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",_ymin,_ymax,_ptmin,_ptmax,_centmin,_centmax));
//  delete can;



  //////// 1DEffLxy for really used histos, these are done with TGraph
  TLegend *leg1d = new TLegend(0.13,0.14,0.5,0.23);
  SetLegendStyle(leg1d);
  leg1d->AddEntry(hRecT,"Reco Nonprompt MC dimuon","lp");
  leg1d->AddEntry(hGenT2,"Gen Nonprompt MC dimuon","lp");
  leg1d->AddEntry(hGenT3,"Gen Prompt MC dimuon","lp");
  leg1d->AddEntry(hGenT4,"Reco Prompt MC dimuon","lp");

  for (int i=0; i<nbinsy-1; i++) {
    for (int j=0; j<nbinspt-1; j++) {
      for (int k=0; k<nbinscent-1; k++) {
        double ymin=yarray[i]; double ymax=yarray[i+1];
        double ptmin=ptarray[j]; double ptmax=ptarray[j+1];
        int centmin=centarray[k]; int centmax=centarray[k+1];

        cout << "1DEffLxy diff in all 3D: " << i << " " << j << " " << k << " ";

        TCanvas *canv = new TCanvas("c","c",600,600);
        canv->Draw();

        string className = "PRJpsi";
        TH1D *hPRGen = (TH1D*)PROut->Get(Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        TH1D *hPRRec = (TH1D*)PROut->Get(Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        SetHistStyle(hPRGen,3,1,1E-3,hPRGen->GetMaximum()*15);
        SetHistStyle(hPRRec,5,1,1E-3,hPRGen->GetMaximum()*15);
        className = "NPJpsi";
        TH1D *hNPGen = (TH1D*)NPOut->Get(Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        TH1D *hNPRec = (TH1D*)NPOut->Get(Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        SetHistStyle(hNPGen,1,0,1E-3,hNPGen->GetMaximum()*15);
        SetHistStyle(hNPRec,0,0,1E-3,hNPGen->GetMaximum()*15);

        canv->SetLogy(1);
        hNPGen->Draw();
        hNPRec->Draw("same");
        hPRGen->Draw("same");
        hPRRec->Draw("same");

        std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
        if (isPbPb) lat->DrawLatex(0.15,0.40,"PbPb 2.76 TeV RegIt J/#psi NPMC");
        else lat->DrawLatex(0.15,0.40,"pp 2.76 TeV GlbGlb J/#psi NPMC");
        lat->DrawLatex(0.15,0.35,testStr.second.c_str());
        lat->DrawLatex(0.15,0.30,testStr.first.c_str());
        if (isPbPb) lat->DrawLatex(0.15,0.25,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
        leg1d->Draw();

        canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
        canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
        delete canv;

        canv = new TCanvas("c","c",600,600);
        canv->Draw();
        canv->SetLogy(0);

        className = "NPJpsi";
        TH1D *hNPEff = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        cout << Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax) << endl;
        className = "PRJpsi";
        TH1D *hPREff = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
        SetHistStyle(hNPEff,0,0,0,1.3);
        SetHistStyle(hPREff,3,1,0,1.3);
        hNPEff->GetXaxis()->SetTitle("L_{xyz} (reco) (mm)");
        hNPEff->GetXaxis()->SetRangeUser(0,10);

        hNPEff->Draw();
        hPREff->Draw("same");
        if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
        else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
        lat->DrawLatex(0.15,0.85,testStr.second.c_str());
        lat->DrawLatex(0.15,0.80,testStr.first.c_str());
        if (isPbPb) lat->DrawLatex(0.15,0.75,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
        legA->Draw();

        canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
        canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
        delete canv;

      } //end of cent loop plot
    } //end of pt loop plot
  } // end of rap loop plotting


  //// 1D integrated plots
//  for (int i=0; i<nbinsy-1; i++) {
//    double ymin=yarray[i]; double ymax=yarray[i+1];
//    double ptmin=ptarray[0]; double ptmax=ptarray[nbinspt-1];
//    int centmin=centarray[0]; int centmax=centarray[nbinscent-1];
//
//    TCanvas *canv = new TCanvas("c","c",600,600);
//    canv->Draw();
//
//    string className = "NPJpsi";
//    TH1D *hNPGen = (TH1D*)NPOut->Get(Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    TH1D *hNPRec = (TH1D*)NPOut->Get(Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    SetHistStyle(hNPGen,1,0,1E-3,hNPGen->GetMaximum()*15);
//    SetHistStyle(hNPRec,0,0,1E-3,hNPGen->GetMaximum()*15);
//
//    canv->SetLogy(1);
//    hNPGen->Draw();
//    hNPRec->Draw("same");
//
//    std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
//    if (isPbPb) lat->DrawLatex(0.15,0.40,"PbPb 2.76 TeV RegIt J/#psi NPMC");
//    else lat->DrawLatex(0.15,0.40,"pp 2.76 TeV GlbGlb J/#psi NPMC");
//    lat->DrawLatex(0.15,0.35,testStr.second.c_str());
//    lat->DrawLatex(0.15,0.30,testStr.first.c_str());
//    if (isPbPb) lat->DrawLatex(0.15,0.25,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
//    leg->Draw();
//
//    canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    delete canv;
//
//    canv = new TCanvas("c","c",600,600);
//    canv->Draw();
//    canv->SetLogy(0);
//
//    className = "NPJpsi";
//    TH1D *hNPEff = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    className = "PRJpsi";
//    TH1D *hPREff = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    SetHistStyle(hNPEff,0,0,0,1.3);
//    SetHistStyle(hPREff,3,1,0,1.3);
//    hNPEff->GetXaxis()->SetTitle("L_{xyz} (true) (mm)");
//
//    hNPEff->Draw();
//    hPREff->Draw("same");
//    if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
//    else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
//    lat->DrawLatex(0.15,0.85,testStr.second.c_str());
//    lat->DrawLatex(0.15,0.80,testStr.first.c_str());
//    if (isPbPb) lat->DrawLatex(0.15,0.75,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
//    legA->Draw();
//
//    canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    delete canv;
//
//  } // end of rap loop plotting
//
//  for (int j=0; j<nbinspt-1; j++) {
//    double ymin=yarray[0]; double ymax=yarray[nbinsy-1];
//    double ptmin=ptarray[j]; double ptmax=ptarray[j+1];
//    int centmin=centarray[0]; int centmax=centarray[nbinscent-1];
//
//    TCanvas *canv = new TCanvas("c","c",600,600);
//    canv->Draw();
//    canv->SetLogy(1);
//
//    string className = "NPJpsi";
//    TH1D *hNPGen = (TH1D*)NPOut->Get(Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    TH1D *hNPRec = (TH1D*)NPOut->Get(Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    SetHistStyle(hNPGen,1,0,1E-3,hNPGen->GetMaximum()*15);
//    SetHistStyle(hNPRec,0,0,1E-3,hNPGen->GetMaximum()*15);
//
//    hNPGen->Draw();
//    hNPRec->Draw("same");
//
//    std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
//    if (isPbPb) lat->DrawLatex(0.15,0.40,"PbPb 2.76 TeV RegIt J/#psi NPMC");
//    else lat->DrawLatex(0.15,0.40,"pp 2.76 TeV GlbGlb J/#psi NPMC");
//    lat->DrawLatex(0.15,0.35,testStr.second.c_str());
//    lat->DrawLatex(0.15,0.30,testStr.first.c_str());
//    if (isPbPb) lat->DrawLatex(0.15,0.25,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
//    leg->Draw();
//
//    canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    delete canv;
//
//    canv = new TCanvas("c","c",600,600);
//    canv->Draw();
//    canv->SetLogy(0);
//
//    className = "NPJpsi";
//    TH1D *hNPEff = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    className = "PRJpsi";
//    TH1D *hPREff = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    SetHistStyle(hNPEff,0,0,0,1.3);
//    SetHistStyle(hPREff,3,1,0,1.3);
//    hNPEff->GetXaxis()->SetTitle("L_{xyz} (true) (mm)");
//
//    hNPEff->Draw();
//    hPREff->Draw("same");
//
//    if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
//    else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
//    lat->DrawLatex(0.15,0.85,testStr.second.c_str());
//    lat->DrawLatex(0.15,0.80,testStr.first.c_str());
//    if (isPbPb) lat->DrawLatex(0.15,0.75,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
//    legA->Draw();
//
//    canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    delete canv;
//
//  } //end of pt loop plot
//
//  for (int j=0; j<nbinscent-1; j++) {
//    double ymin=yarray[0]; double ymax=yarray[nbinsy-1];
//    double ptmin=ptarray[0]; double ptmax=ptarray[nbinspt-1];
//    int centmin=centarray[j]; int centmax=centarray[j+1];
//
//    TCanvas *canv = new TCanvas("c","c",600,600);
//    canv->Draw();
//    canv->SetLogy(1);
//
//    string className = "NPJpsi";
//    TH1D *hNPGen = (TH1D*)NPOut->Get(Form("hGenLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    TH1D *hNPRec = (TH1D*)NPOut->Get(Form("hRecLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    SetHistStyle(hNPGen,1,0,1E-3,hNPGen->GetMaximum()*15);
//    SetHistStyle(hNPRec,0,0,1E-3,hNPGen->GetMaximum()*15);
//
//    hNPGen->Draw();
//    hNPRec->Draw("same");
//
//    std::pair< string, string > testStr = FillLatexInfo(ymin, ymax, ptmin, ptmax, absRapidity);
//    if (isPbPb) lat->DrawLatex(0.15,0.40,"PbPb 2.76 TeV RegIt J/#psi NPMC");
//    else lat->DrawLatex(0.15,0.40,"pp 2.76 TeV GlbGlb J/#psi NPMC");
//    lat->DrawLatex(0.15,0.35,testStr.second.c_str());
//    lat->DrawLatex(0.15,0.30,testStr.first.c_str());
//    if (isPbPb) lat->DrawLatex(0.15,0.25,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
//    leg->Draw();
//
//    canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    canv->SaveAs(Form("./1DHistLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    delete canv;
//
//    canv = new TCanvas("c","c",600,600);
//    canv->Draw();
//    canv->SetLogy(0);
//
//    className = "NPJpsi";
//    TH1D *hNPEff = (TH1D*)NPOut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    className = "PRJpsi";
//    TH1D *hPREff = (TH1D*)PROut->Get(Form("hEffLxy_%s_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d",className.c_str(),ymin,ymax,ptmin,ptmax,centmin,centmax));
//    SetHistStyle(hNPEff,0,0,0,1.3);
//    SetHistStyle(hPREff,3,1,0,1.3);
//    hNPEff->GetXaxis()->SetTitle("L_{xyz} (true) (mm)");
//
//    hNPEff->Draw();
//    hPREff->Draw("same");
//
//    if (isPbPb) lat->DrawLatex(0.15,0.90,"PbPb 2.76 TeV RegIt J/#psi MC");
//    else lat->DrawLatex(0.15,0.90,"pp 2.76 TeV GlbGlb J/#psi MC");
//    lat->DrawLatex(0.15,0.85,testStr.second.c_str());
//    lat->DrawLatex(0.15,0.80,testStr.first.c_str());
//    if (isPbPb) lat->DrawLatex(0.15,0.75,Form("Cent. %.0f-%.0f%%",centmin*2.5,centmax*2.5));
//    legA->Draw();
//
//    canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.pdf",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    canv->SaveAs(Form("./1DEffLxy_Rap%.1f-%.1f_Pt%.1f-%.1f_Cent%d-%d.png",ymin,ymax,ptmin,ptmax,centmin,centmax));
//    delete canv;
//
//  } //end of cent loop plot

  NPOut->Close();
  PROut->Close();

  return;
}


int main(int argc, char *argv[]) {
//void LxyEff_draw(bool absRapidity=true) {

  if (argc != 4) {
    cout << "./a.out [absRapidity[0 or 1]] [logy[0 or 1]] [isPbPb[1 or 0]]" << endl;
    return -1;
  }

  gErrorIgnoreLevel = kWarning, kError, kBreak, kSysError, kFatal;

  bool absRapidity = atoi(argv[1]);
  bool logy= atoi(argv[2]);
  bool isPbPb = atoi(argv[3]);
  cout << absRapidity << " " << logy << " " << isPbPb << endl;

  LxyEff_all(absRapidity, logy, isPbPb);
//  LxyEff_diff3D(absRapidity, logy, isPbPb);
//  LxyEff_diffRap(absRapidity, logy, isPbPb);
//  LxyEff_diffPt(absRapidity, logy, isPbPb);
//  LxyEff_diffCent(absRapidity, logy, isPbPb);

  return 0;
  
}


