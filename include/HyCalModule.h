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
    };

    struct GeoInfo
    {
        ModuleType type;
        double cellSize;
        double x;
        double y;
        GeoInfo() {};
        GeoInfo(ModuleType t, double s, double xx, double yy)
        : type(t), cellSize(s), x(xx), y(yy) {};
    };

    struct Voltage
    {
       float Vmon;
       float Vset;
       bool ON;
       Voltage() {};
       Voltage(float vm, float vs, bool o = false) : Vmon(vm), Vset(vs), ON(o) {};
    };

    struct HVSetup
    {
        ChannelAddress config;
        Voltage volt;
        HVSetup() {};
        HVSetup(ChannelAddress c, Voltage v) : config(c), volt(v) {};
    };

public:
    HyCalModule(PRadEventViewer* const p,
                const QString &rid,
                const ChannelAddress &daqAddr,
                const QString &tdc,
                const HVSetup &hvInfo,
                const GeoInfo &geo);
    virtual ~HyCalModule();

    void Initialize();
    void CalcGeometry();
    void SetColor(const double &val);
    void ShowPedestal() {SetColor(pedestal.mean);};
    void ShowPedSigma() {SetColor(pedestal.sigma);};
    void ShowOccupancy() {SetColor(occupancy);};
    void ShowEnergy() {SetColor(energy);};
    void ShowVoltage();
    void ShowVSet();
    void UpdateHV(float Vmon, float Vset, bool on) {hvSetup.volt.Vmon = Vmon; hvSetup.volt.Vset = Vset; hvSetup.volt.ON = on;};
    void UpdateHVSetup(ChannelAddress &set) {hvSetup.config = set;};
    QString GetReadID() {return name;};
    Voltage GetVoltage() {return hvSetup.volt;};
    ChannelAddress GetHVInfo() {return hvSetup.config;};
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
    HVSetup hvSetup;
    GeoInfo geometry;

    bool m_hover;
    bool m_selected;
    QColor color;
    QFont font;
    QPainterPath shape;
};

#endif
