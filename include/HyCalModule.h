#ifndef PRAD_HYCAL_BLOCK_H
#define PRAD_HYCAL_BLOCK_H

#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QFont>
#include "PRadDAQUnit.h"

#define HYCAL_SHIFT 50

class PRadEventViewer;

class HyCalModule : public QGraphicsItem, public PRadDAQUnit
{
public:
    enum ModuleType
    {
        LeadGlass,
        LeadTungstate,
        Scintillator,
        LightMonitor,
    };

    struct GeoInfo
    {
        ModuleType type;
        double size_x;
        double size_y;
        double x;
        double y;
        GeoInfo() {};
        GeoInfo(ModuleType t, double sx, double sy, double xx, double yy)
        : type(t), size_x(sx), size_y(sy), x(xx), y(yy) {};
    };

public:
    HyCalModule(PRadEventViewer* const p,
                const QString &rid,
                const ChannelAddress &daqAddr,
                const QString &tdc,
                const GeoInfo &geo);
    virtual ~HyCalModule();

    void Initialize();
    void CalcGeometry();
    void SetColor(const QColor &c) {color = c;};
    void SetColor(const double &val);
    void ShowPedestal() {SetColor(pedestal.mean);};
    void ShowPedSigma() {SetColor(pedestal.sigma);};
    void ShowOccupancy() {SetColor(occupancy);};
    void ShowEnergy() {SetColor(energy);};
    void UpdateHVSetup(ChannelAddress &set) {hv_addr = set;};
    QString GetReadID() {return name;};
    ChannelAddress GetHVInfo() {return hv_addr;};
    GeoInfo GetGeometry() {return geometry;};

    // overload
    QRectF boundingRect() const;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option, QWidget *widget);
    void setSelected(bool selected);
    bool isSelected(){return m_selected;};

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    PRadEventViewer *console;
    QString name;
    ChannelAddress hv_addr;
    GeoInfo geometry;

    bool m_hover;
    bool m_selected;
    QColor color;
    QFont font;
    QPainterPath shape;
};

#endif
