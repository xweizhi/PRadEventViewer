#ifndef PRAD_GEM_H
#define PRAD_GEM_H

#include <QObject>
#include "TQtWidget.h"

class TH2F;
class QSplitter;

class GEM : public QObject
{
    Q_OBJECT

public:
    GEM();
    QSplitter *gemSplitter;
    QSplitter *GetGemLayout() {return gemSplitter;};

public slots:
    void gemStatusWin1(TObject *select, unsigned int /*event*/, TCanvas *c1);
    void gemStatusWin2(TObject *select, unsigned int /*event*/, TCanvas *c1);

private:
    void initView();
    void setupWidgets();
    void setupGemMain();
    TQtWidget *gemMain;
    TQtWidget *gemStatus1;
    TQtWidget *gemStatus2;
    TH2F *gemHist;
};

#endif
