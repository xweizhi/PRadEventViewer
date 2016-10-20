#ifndef PRAD_HYCAL_SCENE_H
#define PRAD_HYCAL_SCENE_H

#include <QGraphicsScene>
#include <vector>
#include <map>

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
    struct TextBox {
        QString name;
        QString text;
        QColor textColor;
        QRectF bound;
        QColor bgColor;

        TextBox() {};
        TextBox(const QString &n, const QString &t, const QColor &tc, const QRectF &b, const QColor &c)
        : name(n), text(t), textColor(tc), bound(b), bgColor(c) {};
        TextBox(const QString &n, const QColor &tc, const QRectF &b, const QColor &c)
        : name(n), text(""), textColor(tc), bound(b), bgColor(c) {};
     };

    HyCalScene(PRadEventViewer *p, QObject *parent = 0)
    : QGraphicsScene(parent), console(p),
      pModule(nullptr), sModule(nullptr), rModule(nullptr), showScalers(false)
    {};

    HyCalScene(PRadEventViewer *p, const QRectF & sceneRect, QObject *parent = 0)
    : QGraphicsScene(sceneRect, parent), console(p),
      pModule(nullptr), sModule(nullptr), rModule(nullptr), showScalers(false)
    {};

    HyCalScene(PRadEventViewer*p, qreal x, qreal y, qreal width, qreal height, QObject *parent = 0)
    : QGraphicsScene(x, y, width, height, parent), console(p),
      pModule(nullptr), sModule(nullptr), rModule(nullptr), showScalers(false)
    {};

    void AddTDCBox(const QString &name, const QColor &textColor, const QRectF &textBox, const QColor &bgColor);
    void AddScalerBox(const QString &name, const QColor &textColor, const QRectF &textBox, const QColor &bgColor);
    void UpdateScalerBox(const QString &text, const int &group = 0);
    void UpdateScalerBox(const QStringList &texts);
    void ShowScalers(const bool &s = true) {showScalers = s;};
    void addModule(HyCalModule *module);
    void addItem(QGraphicsItem *item);
    QVector<HyCalModule *> GetModuleList() {return moduleList;};
    void ClearHits();
    void AddHyCalHits(const QPointF &hit);
    void AddGEMHits(int igem, const QPointF &hit);
    void AddEnergyValue(QString s, const QRectF &module);

protected:
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    PRadEventViewer *console;
    HyCalModule *pModule, *sModule, *rModule;
    bool showScalers;
    QList<TextBox> tdcBoxList;
    QVector<TextBox> scalarBoxList;
    QVector<HyCalModule *> moduleList;
    QList<QPointF> recon_hits;
    std::map< int, QList<QPointF> > gem_hits;
    std::vector< std::pair<QString, const QRectF> > module_energy;
};

#endif
