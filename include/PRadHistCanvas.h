
#ifndef PRAD_HIST_CANVAS_H
#define PRAD_HIST_CANVAS_H

#include <QWidget>
#include <mutex>

class QRootCanvas;
class QGridLayout;
class TObject;
class TCanvas;
class TColor;
class TF1;

class PRadHistCanvas : public QWidget
{
    Q_OBJECT

public:
    PRadHistCanvas(QWidget *parent = 0);
    virtual ~PRadHistCanvas() {}
    void AddCanvas(int row, int column, int fillColor = 38);
    void UpdateHist(int index, TObject *hist, int range_min = 0, int range_max = 0);

protected:
    QGridLayout *layout;
    TColor *bkgColor;
    TColor *frmColor;
    QVector<QRootCanvas *> canvases;
    QVector<int> fillColors;
    std::mutex histLocker;
};

#endif
