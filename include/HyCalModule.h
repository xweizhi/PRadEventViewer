#ifndef PRAD_HYCAL_BLOCK_H
#define PRAD_HYCAL_BLOCK_H

#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QFont>
#include "TH1I.h"
#include "datastruct.h"

#define HYCAL_SHIFT 50


class PRadEventViewer;

class HyCalModule : public QGraphicsItem
{
public:
    enum ModuleType
    {
        LeadGlass,
        LeadTungstate,
        SpecialModule,
    };

    struct Pedestal
    {
        double mean;
        double sigma;
        Pedestal() {};
        Pedestal(double m, double s) : mean(m), sigma(s) {};
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

    struct DAQSetup
    {
        ChannelAddress config;
        Pedestal pedestal;
        unsigned char tdcGroup;
        DAQSetup() {};
        DAQSetup(ChannelAddress c, Pedestal p, unsigned char tdc_id)
        : config(c), pedestal(p), tdcGroup(tdc_id) {};
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
                const DAQSetup &daqInfo,
                const HVSetup &hvInfo,
                const GeoInfo &geo);
    virtual ~HyCalModule();

    void UpdatePedestal(const double &val, const double &sig);
    void Initialize();
    void CalcGeometry();
    void CleanBuffer();
    void SetColor(const double &val);
    void ShowPedestal() {SetColor(daqSetup.pedestal.mean);};
    void ShowPedSigma() {SetColor(daqSetup.pedestal.sigma);};
    void ShowOccupancy() {SetColor(occupancy);};
    void ShowVoltage();
    void UpdateHV(const float &Vmon, const float &Vset, const bool &ON) {hvSetup.volt.Vmon = Vmon; hvSetup.volt.Vset = Vset; hvSetup.volt.ON = ON;};
    void UpdateHVSetup(ChannelAddress &set) {hvSetup.config = set;};
    void DeEnergize() {color = Qt::white; energy = 0;};
    void Energize(const unsigned short &adcVal);
    unsigned short Sparsification(unsigned short &adcVal);
    void AssignID(const int &n) {id = n;};
    unsigned short GetID() {return id;};
    int GetTDCID() {return (int)daqSetup.tdcGroup;};
    int GetOccupancy() {return occupancy;};
    QString GetReadID() {return name;};
    QString GetTDCGroupName();
    Voltage GetVoltage() {return hvSetup.volt;};
    double GetEnergy() {return energy;};
    double Calibration(const unsigned short &val); 
    ChannelAddress GetDAQInfo() {return daqSetup.config;};
    ChannelAddress GetHVInfo() {return hvSetup.config;};
    GeoInfo GetGeometry() {return geometry;};
    Pedestal GetPedestal() {return daqSetup.pedestal;};

    // overload
    QRectF boundingRect() const;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option, QWidget *widget);
    void setSelected(bool selected);
    bool isSelected(){return m_selected;};

    TH1I *adcHist;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    PRadEventViewer *console;
    QString name;
    DAQSetup daqSetup;
    HVSetup hvSetup;
    GeoInfo geometry;
    int occupancy;
    double energy;
    unsigned short id;
    unsigned short sparsify;

    bool m_hover;
    bool m_selected;
    QColor color;
    QFont font;
    QPainterPath shape;
};

#endif
