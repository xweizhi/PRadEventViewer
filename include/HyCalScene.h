#ifndef PRAD_HYCAL_SCENE_H
#define PRAD_HYCAL_SCENE_H

#include <QGraphicsScene>

class PRadEventViewer;
class HyCalModule;
class QString;
class QPoint;
class QRectF;
class QColor;

class HyCalScene : public QGraphicsScene
{
    Q_OBJECT

public:
    typedef struct {
        QString boxName;
        QRectF boxBounding;
        QColor backgroundColor;
    } TextBox;

    HyCalScene(PRadEventViewer *p, QObject *parent = 0)
    : QGraphicsScene(parent), console(p),
      pModule(NULL), sModule(NULL), rModule(NULL) {};

    HyCalScene(PRadEventViewer *p, const QRectF & sceneRect, QObject *parent = 0)
    : QGraphicsScene(sceneRect, parent), console(p),
      pModule(NULL), sModule(NULL), rModule(NULL) {};

    HyCalScene(PRadEventViewer*p, qreal x, qreal y, qreal width, qreal height, QObject *parent = 0)
    : QGraphicsScene(x, y, width, height, parent), console(p),
      pModule(NULL), sModule(NULL), rModule(NULL) {};

    void AddTextBox(QString &name, QRectF &textBox, QColor &bkgColor);
    void addItem(QGraphicsItem *item);

protected:
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    PRadEventViewer *console;
    HyCalModule *pModule, *sModule, *rModule;
    QList<TextBox> tdcBoxList;
};

#endif
