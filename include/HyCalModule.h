#ifndef PRAD_HYCAL_BLOCK_H
#define PRAD_HYCAL_BLOCK_H

#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QFont>
#include "PRadDAQUnit.h"
#include "datastruct.h"

#define HYCAL_SHIFT 50

class PRadEventViewer;

class HyCalModule : public QGraphicsItem, public PRadDAQUnit
{
public:
    enum ModuleType
    {
        LeadGlass,
        LeadTungstate,
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
                const int &tdc,
                const HVSetup &hvInfo,
                const GeoInfo &geo);
    virtual ~HyCalModule();

    void Initialize();
    void CalcGeometry();
    void SetColor(const double &val);
    void ShowPedestal() {SetColor(pedestal.mean);};
    void ShowPedSigma() {SetColor(pedestal.sigma);};
    void ShowOccupancy() {SetColor(occupancy);};
    void ShowVoltage();
    void UpdateHV(const float &Vmon, const float &Vset, const bool &ON) {hvSetup.volt.Vmon = Vmon; hvSetup.volt.Vset = Vset; hvSetup.volt.ON = ON;};
    void UpdateHVSetup(ChannelAddress &set) {hvSetup.config = set;};
    void DeEnergize() {color = Qt::white; energy = 0;};
    void Energize(const unsigned short &adcVal);
    void AssignID(const int &n) {id = n;};
    unsigned short GetID() {return id;};
    double GetEnergy() {return energy;};
    double Calibration(const unsigned short &val);
    QString GetReadID() {return name;};
    QString GetTDCGroupName();
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
    double energy;
    unsigned short id;

    bool m_hover;
    bool m_selected;
    QColor color;
    QFont font;
    QPainterPath shape;
};

#endif
