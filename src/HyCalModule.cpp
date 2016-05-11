//============================================================================//
// HyCal Module class, the basic element of HyCal                             //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include <cmath>
#include <string>
#include "HyCalModule.h"
#include "PRadEventViewer.h"

#if QT_VERSION >= 0x500000
#include <QtWidgets>
#else
#include <QtGui>
#endif

HyCalModule::HyCalModule(PRadEventViewer* const p,
                         const QString &rid,
                         const ChannelAddress &daqAddr,
                         const QString &tdc,
                         const HVSetup &hvInfo,
                         const GeoInfo &geo)
: PRadDAQUnit(rid.toStdString(), daqAddr, tdc.toStdString()),
  console(p), name(rid), hvSetup(hvInfo), geometry(geo),
  m_hover(false), m_selected(false), color(Qt::white), font(QFont("times",10))
{
    // initialize the item
    Initialize();

    // detect if mouse is hovering on this item
    setAcceptHoverEvents(true);

}

HyCalModule::~HyCalModule()
{
}

// initialize the module
void HyCalModule::Initialize()
{
    // calculate position of this module
    // CalcGeometry(); no longer needed since we read everything from list

    setPos(geometry.x - HYCAL_SHIFT, geometry.y);

    sparsify = (unsigned short)(pedestal.mean + 5*pedestal.sigma);

    if(geometry.type == LeadTungstate)
        font.setPixelSize(9);

    // graphical shape
    double size = geometry.cellSize;
    shape.addRect(-size/2.,-size/2.,size,size);
}

// define the bound of this item
QRectF HyCalModule::boundingRect() const
{
    double size = geometry.cellSize;
    return QRectF(-size/2., -size/2., size, size);
}

// how to paint this item
void HyCalModule::paint(QPainter *painter, 
                        const QStyleOptionGraphicsItem *option,
                        QWidget * /* widget */)
{
    QColor fillColor = color;
    QColor fontColor = Qt::black;

    // mouse is hovering around, turns darker
    if(m_hover == true) {
        fillColor = fillColor.darker(125);
        fontColor = Qt::white;
    }

    // if it is zoomed in a lot, put some gradient on the color
    if(option->levelOfDetail < 4.0) {
        painter->fillPath(shape, fillColor);
    } else {
        QLinearGradient gradient(QPoint(-20, -20), QPoint(+20, +20));
        int coeff = 105 + int(std::log(option->levelOfDetail - 4.0));
        gradient.setColorAt(0.0, fillColor.lighter(coeff));
        gradient.setColorAt(1.0, fillColor.darker(coeff));
        painter->fillPath(shape, gradient);
    }

    // draw frame of this item
    painter->drawPath(shape);

    // is selected? turns purple
    if(m_selected) {
        painter->fillPath(shape,QColor(155,0,155,200));
        fontColor = Qt::white;
    }

    // get the current display settings from console
    if(console != nullptr){
        switch(console->GetAnnoType())
        {
        case NoAnnotation: // show nothing
            break;
        case ShowID: // show module id
            painter->setFont(font);
            painter->setPen(fontColor);
            if(geometry.type == LeadTungstate) // ignore W for PbWO4 module due to have a better view
                painter->drawText(boundingRect(), name.mid(1), QTextOption(Qt::AlignCenter));
            else
                painter->drawText(boundingRect(), name, QTextOption(Qt::AlignCenter));
            break;
        case ShowDAQ: // show daq configuration
            painter->setFont(font);
            painter->setPen(fontColor);
            painter->drawText(boundingRect(),
                              QString::number(address.crate) + "," + QString::number(address.slot),
                              QTextOption(Qt::AlignTop | Qt::AlignHCenter));
            painter->drawText(boundingRect(),
                              "Ch" + QString::number(address.channel+1),
                              QTextOption(Qt::AlignBottom | Qt::AlignHCenter));
            break;
        case ShowTDC: // this will be handled in HyCalScene
            break;
        }
    }
}

// hover bool
void HyCalModule::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    m_hover = true;
    update();
}

void HyCalModule::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    m_hover = false;
    update();
}

void HyCalModule::setSelected(bool selected)
{
    m_selected = selected;
    update();
    if(selected)
        console->SelectModule(this);
}

// Get color from the spectrum
void HyCalModule::SetColor(const double &val)
{
    color = console->GetColor(val);
}

// convert ADC value after sparsification to energy
// TODO implement calibration
double HyCalModule::Calibration(const unsigned short &val)
{
    return (double) val/3.;
}

// update high voltage
void HyCalModule::ShowVoltage()
{
    if(!hvSetup.volt.ON) {
        color = Qt::white;
        return;
    }

    SetColor(hvSetup.volt.Vmon);
    // some warning if currentHV is far away from setHV
}

// calculate module position according to its id
// also calculate trigger group because we grouped them by positions
// this function is not needed anymore since we will read all the information from list
void HyCalModule::CalcGeometry()
{
    int xIndex, yIndex;
    double size;

    int copyNo = name.mid(1,-1).toInt() - 1;

    if(name.at(0) == 'W') {
        xIndex = copyNo % 34;
        yIndex = copyNo / 34;
        size = 20.5;
        geometry.type = LeadTungstate;
        geometry.cellSize = size;
        geometry.x = (double)(xIndex - 16)*size - size/2.;
        geometry.y = (double)(yIndex - 17)*size + size/2.;
    } else {
        int groupID;
        double xShift = 0., yShift = 0.;
        xIndex = copyNo%30;
        yIndex = copyNo/30;
        size = 38.2;
        geometry.type = LeadGlass;
        geometry.cellSize = size;

        // Determine 4 groups of Lead glass modules
        // 1. north, 2. west, 3. south, 4. east
        if(copyNo < 180) {
            if(xIndex < 24)
                groupID = 1;
            else
                groupID = 4;
        } else if (copyNo < 726) {
            if(xIndex < 6)
                groupID = 2;
            else
                groupID = 4;
        } else {
            if(xIndex < 6)
                groupID = 2;
            else
                groupID = 3;
        }

        // Set offset in geometry
        switch(groupID) {
            case 1:
                xShift = 9.2;
                break;
            case 2:
                break;
            case 3:
                yShift = 9.2;
                break;
            case 4:
                xShift = 9.2;
                yShift = 9.2;
                break;
        }
        geometry.x = -17.*20.5 - 5.5*size + xIndex*size + xShift;
        geometry.y = -17.*20.5 - 5.5*size + yIndex*size + yShift;
    }
}

