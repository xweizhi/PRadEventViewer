// this class is not implemented
// Chao Peng

#include <QtGui>
#include <QObject>
#include "gem.h"
#include "TQtWidget.h"
#include "TCanvas.h"
#include "TList.h"
#include "TStyle.h"
#include "TF1.h"
#include "TH2F.h"
#include "TRandom3.h"

GEM::GEM()
{
    initView();
    setupWidgets();
}

void GEM::initView()
{
    gemSplitter = new QSplitter(Qt::Vertical);
    gemMain = new TQtWidget(gemSplitter);

    gemStatus1 = new TQtWidget(gemSplitter);
    gemStatus1->GetCanvas()->SetName("Status Window 1");

    gemStatus2 = new TQtWidget(gemSplitter);
    gemStatus2->GetCanvas()->SetName("Status Window 2");

    gemSplitter->addWidget(gemMain);
    gemSplitter->addWidget(gemStatus1);
    gemSplitter->addWidget(gemStatus2);

    // main window : status 1 : status 2 = 2 : 1 : 1
    gemSplitter->setStretchFactor(0, 2);
    gemSplitter->setStretchFactor(1, 1);
    gemSplitter->setStretchFactor(2, 1);
    gemSplitter->setMinimumWidth(400);
}

void GEM::setupWidgets()
{
    setupGemMain();
    connect(gemMain,SIGNAL(RootEventProcessed(TObject *, unsigned int, TCanvas *)),
            this, SLOT(gemStatusWin1(TObject *, unsigned int, TCanvas *)));
    connect(gemMain,SIGNAL(RootEventProcessed(TObject *, unsigned int, TCanvas *)),
            this, SLOT(gemStatusWin2(TObject *, unsigned int, TCanvas *)));
    gemMain->EnableSignalEvents(kMousePressEvent);
}

void GEM::setupGemMain()
{
    //---------A histogram showing GEM hit position for now-----------
    TCanvas *c1 = gemMain->GetCanvas();
    c1->SetName("Main View");
    c1->SetTitle("GEM View");
    c1->SetFillColor(kCyan-10);
    c1->SetFrameFillColor(19);

    gemHist = new TH2F("hist", "GEM", 200, -0.8, 0.8, 200, -0.8, 0.8);
    gemHist->SetStats(0);

    TRandom3 *ran = new TRandom3(0);
    for (int i=0; i<100000; i++){
      float x = ran->Gaus(0, 0.2);
      float y = ran->Gaus(0, 0.2);
      if ((x<0.02&&x>-0.02) && (y>-0.02&&y<0.02)) {i--; continue;}
      gemHist->Fill(x, y);
    }

    c1->GetListOfPrimitives()->Add(gemHist,"colz");
    c1->Modified();
}

void GEM::gemStatusWin1(TObject *select, unsigned int /*event*/, TCanvas *c1)
{
    if(!select) return;
    if(!select->InheritsFrom("TH2")) {
        c1->SetUniqueID(0);
        return;
    }

    TH2 *h = (TH2*)select;

    int px = c1->GetEventX();
    c1->SetUniqueID(px);

    // Find the clicked position
    Float_t upx = c1->AbsPixeltoX(px);
    Float_t x = c1->PadtoX(upx);

    TVirtualPad *padsav = gPad;
    TCanvas *c2 = gemStatus1->GetCanvas();

    // Release memory
    delete c2->GetPrimitive("Projection");

    c2->SetGrid();
    c2->cd();

    // Draw projection
    Int_t binx = h->GetXaxis()->FindBin(x);
    TH1D *hp = h->ProjectionY("",binx,binx);
    hp->SetFillColor(38);
    char title[80];
    sprintf(title,"Projection of binx=%d",binx);
    hp->SetName("Projection");
    hp->SetTitle(title);
    hp->Fit("gaus","ql");
    hp->GetFunction("gaus")->SetLineColor(kRed);
    hp->GetFunction("gaus")->SetLineWidth(6);
    gemStatus1->Refresh();
    padsav->cd();
}

void GEM::gemStatusWin2(TObject *select, unsigned int /*event*/, TCanvas *c1)
{
    if(!select) return;
    if(!select->InheritsFrom("TH2")) {
        c1->SetUniqueID(0);
        return;
    }

    TH2 *h = (TH2*)select;

    int py = c1->GetEventY();
    c1->SetUniqueID(py);

    // Find the clicked position
    Float_t upy = c1->AbsPixeltoY(py);
    Float_t y = c1->PadtoY(upy);

    TVirtualPad *padsav = gPad;
    TCanvas *c2 = gemStatus2->GetCanvas();

    // Release memory
    delete c2->GetPrimitive("Projection");

    c2->SetGrid();
    c2->cd();

    // Draw projection
    Int_t biny = h->GetYaxis()->FindBin(y);
    TH1D *hp = h->ProjectionX("",biny,biny);
    hp->SetFillColor(38);
    char title[80];
    sprintf(title,"Projection of biny=%d",biny);
    hp->SetName("Projection");
    hp->SetTitle(title);
    hp->Fit("gaus","ql");
    hp->GetFunction("gaus")->SetLineColor(kRed);
    hp->GetFunction("gaus")->SetLineWidth(6);
    gemStatus2->Refresh();
    padsav->cd();
}


