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
    layout = new QGridLayout(this);

    bkgColor = new TColor(200, 1, 1, 0.96);
    gStyle->SetTitleFontSize(HIST_FONT_SIZE);
    gStyle->SetStatFontSize(HIST_FONT_SIZE);
}

void PRadHistCanvas::AddCanvas(int row, int column, int color)
{
    QRootCanvas *newCanvas = new QRootCanvas(this);
    canvases.push_back(newCanvas);
    fillColors.push_back(color);
    
    // add canvas in vertical layout
    layout->addWidget(newCanvas, row, column);
    newCanvas->SetFillColor(bkgColor->GetNumber());
    newCanvas->SetFrameFillColor(10); // white
}

// show the histogram in first slot, try a Gaussian fit with given parameters
void PRadHistCanvas::UpdateHist(int index, TObject *tob, int range_min, int range_max)
{
    --index;
    if(index < 0 || index >= canvases.size())
        return;

    canvases[index]->cd();
    canvases[index]->SetGrid();

    //gPad->SetLogy();

    TH1 *hist = (TH1*)tob;

    int firstBin = hist->FindFirstBinAbove(0,1)*0.7;
    int lastBin = hist->FindLastBinAbove(0,1)*1.3;

    hist->GetXaxis()->SetRange(firstBin, lastBin);

    hist->GetXaxis()->SetLabelSize(HIST_LABEL_SIZE);
    hist->GetYaxis()->SetLabelSize(HIST_LABEL_SIZE);

    // try to fit gaussian in certain range
    if(range_max > range_min
       && hist->Integral(range_min, range_max + 1) > 0)
    {
        TF1 *fit = new TF1("", "gaus", range_min, range_max);
        fit->SetLineColor(kRed);
        fit->SetLineWidth(2);
        hist->Fit(fit,"qlR");
        delete fit;
    }

    hist->SetFillColor(fillColors[index]);
    hist->Draw();

    canvases[index]->Refresh();
}

