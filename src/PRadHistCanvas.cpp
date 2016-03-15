//============================================================================//
// A class contains a few root canvas                                         //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include <QLayout>

#include "TSystem.h"
#include "TStyle.h"
#include "TColor.h"
#include "TF1.h"
#include "TH1.h"
#include "TAxis.h"

#include "PRadHistCanvas.h"
#include "QRootCanvas.h"

#define HIST_FONT_SIZE 0.07
#define HIST_LABEL_SIZE 0.07

PRadHistCanvas::PRadHistCanvas(QWidget *parent) : QWidget(parent)
{
    // add canvas in vertical layout
    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(canvas1 = new QRootCanvas(this));
    l->addWidget(canvas2 = new QRootCanvas(this));

    // global settings for root
    gStyle->SetTitleFontSize(HIST_FONT_SIZE);
    gStyle->SetStatFontSize(HIST_FONT_SIZE);

    TColor *color = new TColor(123, 1, 1, 0.96);
    canvas1->GetCanvas()->SetFillColor(color->GetNumber());
    canvas1->GetCanvas()->SetFrameFillColor(10);
    canvas2->GetCanvas()->SetFillColor(color->GetNumber());
    canvas2->GetCanvas()->SetFrameFillColor(10);
}

// show the histogram in first slot, try a Gaussian fit with given parameters
void PRadHistCanvas::UpdateHyCalHist(TObject *hist, double ped_mean, double ped_sigma)
{
    TCanvas *c1 = canvas1->GetCanvas();
    c1->cd();
    c1->SetGrid();
    gPad->SetLogy();

    TH1I *adcValHist = (TH1I*)hist;

    adcValHist->GetXaxis()->SetRangeUser(adcValHist->FindFirstBinAbove(0,1) - 10,
                                         adcValHist->FindLastBinAbove(0,1) + 10);

    adcValHist->GetXaxis()->SetLabelSize(HIST_LABEL_SIZE);
    adcValHist->GetYaxis()->SetLabelSize(HIST_LABEL_SIZE);

    // try to fit out pedestal with given ped information
    double pedMin = ped_mean - 5*ped_sigma;
    double pedMax = ped_mean + 5*ped_sigma;
    if(adcValHist->Integral((int)pedMin, (int)pedMax + 1) > 0) {
        TF1 *pedFit = new TF1("", "gaus", pedMin, pedMax);
        pedFit->SetLineColor(kRed);
        pedFit->SetLineWidth(2);
        adcValHist->Fit(pedFit,"qlR");
    }

    adcValHist->SetFillColor(38);
    adcValHist->Draw();
    canvas1->Refresh();
}

// show the histogram in second slot
void PRadHistCanvas::UpdateEnergyHist(TObject *hist)
{
    TCanvas *c2 = canvas2->GetCanvas();
    c2->cd();
    c2->SetGrid();
    gPad->SetLogy();

    TH1D *energyHist = (TH1D*)hist;

    energyHist->GetXaxis()->SetLabelSize(HIST_LABEL_SIZE);
    energyHist->GetYaxis()->SetLabelSize(HIST_LABEL_SIZE);

    energyHist->SetFillColor(46);
    energyHist->Draw();
    canvas2->Refresh();
}
