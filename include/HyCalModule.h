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
    HyCalModule(PRadEventViewer* const p,
                const std::string &rid,
                const ChannelAddress &daqAddr,
                const std::string &tdc,
                const PRadDAQUnit::Geometry &geo);
    virtual ~HyCalModule();

    void Initialize();
    void CalcGeometry();
    void SetColor(const QColor &c) {color = c;};
    void SetColor(const double &val);
    void ShowPedestal() {SetColor(pedestal.mean);};
    void ShowPedSigma() {SetColor(pedestal.sigma);};
    void ShowOccupancy() {SetColor(occupancy);};
    void ShowEnergy();
    void ShowCustomValue() {SetColor(custom_value);};
    void UpdateHVSetup(ChannelAddress &set) {hv_addr = set;};
    void UpdateCustomValue(double val) {custom_value = val;};
    const double &GetCustomValue() {return custom_value;};
    QString GetReadID() {return name;};
    ChannelAddress GetHVInfo() {return hv_addr;};

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

    bool m_hover;
    bool m_selected;
    QColor color;
    QFont font;
    QPainterPath shape;
    double custom_value;
};

#endif
