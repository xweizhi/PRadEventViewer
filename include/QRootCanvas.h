#ifndef Q_ROOT_CANVAS_H
#define Q_ROOT_CANVAS_H

#include <QWidget>
#include "TCanvas.h"

class TObject;
class QObject;
class QEvent;
class QPaintEvent;
class QResizeEvent;
class QMouseEvent;

class QRootCanvas : public QWidget
{
    Q_OBJECT

public:
    QRootCanvas( QWidget *parent = 0);
    virtual ~QRootCanvas() {}
    void Refresh() {fCanvas->Modified(); fCanvas->Update();};
    TCanvas* GetCanvas() {return fCanvas;};

signals:
    void TObjectSelected(TObject *, TCanvas *);

protected:
    TCanvas *fCanvas;

    virtual bool eventFilter(QObject *o, QEvent *e);
    virtual void resizeEvent(QResizeEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void paintEvent(QPaintEvent *e);
    virtual void leaveEvent(QEvent *e);

private:
    bool fNeedResize;
};

#endif
