//============================================================================//
// A class derived from QGraphicsScene                                        //
// This class is used for self-defined selection behavior                     //
// in the future will be used for display the reconstructed clusters          //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "HyCalScene.h"
#include "PRadEventViewer.h"
#include "HyCalModule.h"

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

void HyCalScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);
    painter->save();

    // print scalar boxes
    if(showScalars) {
        painter->setFont(QFont("times", 16, QFont::Bold));
        for(auto it = scalarBoxList.begin(); it != scalarBoxList.end(); ++it)
        {
            QPen pen(it->textColor);
            pen.setWidth(2);
            pen.setCosmetic(true);
            painter->setPen(pen);
            QPainterPath path;
            path.addRect(it->bound);
            painter->fillPath(path, it->bgColor);
            painter->drawPath(path);
            QRectF name_box = it->bound.translated(0, -it->bound.height());
            painter->drawText(name_box,
                              it->name,
                              QTextOption(Qt::AlignBottom | Qt::AlignHCenter));
             painter->drawText(it->bound,
                              it->text,
                              QTextOption(Qt::AlignCenter | Qt::AlignHCenter));
        }
    }

    // this to be impelmented to show the tdc groups and cluster reconstruction
    if(console->GetAnnoType() == ShowTDC)
    {
       painter->setFont(QFont("times", 24, QFont::Bold));
       for (auto it = tdcBoxList.begin(); it != tdcBoxList.end(); ++it)
        {
            QPen pen(it->textColor);
            pen.setWidth(2);
            pen.setCosmetic(true);
            painter->setPen(pen);
            QPainterPath path;
            path.addRect(it->bound);
            painter->fillPath(path, it->bgColor);
            painter->drawPath(path);
            painter->drawText(it->bound,
                              it->name,
                              QTextOption(Qt::AlignCenter | Qt::AlignHCenter));

        }
    }

    painter->restore();
}

void HyCalScene::addItem(QGraphicsItem *item)
{
    QGraphicsScene::addItem(item);
}

void HyCalScene::addModule(HyCalModule *module)
{
    QGraphicsScene::addItem(module);
    moduleList.push_back(module);
}

void HyCalScene::AddTDCBox(const QString &text, const QColor &textColor, const QRectF &textBox, const QColor &bkgColor)
{
    tdcBoxList.append(TextBox(text, textColor, textBox, bkgColor));
}

void HyCalScene::AddScalarBox(const QString &text, const QColor &textColor, const QRectF &textBox, const QColor &bkgColor)
{
    scalarBoxList.push_back(TextBox(text, textColor, textBox, bkgColor));
}

void HyCalScene::UpdateScalarCount(const int &group, const unsigned int &count)
{
    if(group < 0 || group >= scalarBoxList.size())
        return;

    scalarBoxList[group].text = QString::number(count);
}

void HyCalScene::UpdateScalarsCount(const std::vector<unsigned int> &counts)
{
    for(size_t i = 0; i < counts.size(); ++i)
    {
        UpdateScalarCount((int)i, counts[i]);
    }
}

// overload to change the default selection behavior
void HyCalScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->modifiers().testFlag(Qt::ControlModifier))
        return;
//    QGraphicsScene::mousePressEvent(event);
    if(event->button() == Qt::LeftButton)
        pModule = dynamic_cast<HyCalModule*>(itemAt(event->scenePos(), QTransform()));
    else if(event->button() == Qt::RightButton)
        rModule = dynamic_cast<HyCalModule*>(itemAt(event->scenePos(), QTransform()));
}

// overload to change the default selection behavior
void HyCalScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        if(pModule &&
           !pModule->isSelected() &&
           pModule == dynamic_cast<HyCalModule*>(itemAt(event->scenePos(), QTransform()))
          ) {
            pModule->setSelected(true);
            if(sModule) {
                sModule->setSelected(false);
            }
            sModule = pModule;
        }
    } else if(event->button() == Qt::RightButton) {
        if(rModule &&
           rModule->isSelected() &&
           rModule == dynamic_cast<HyCalModule*>(itemAt(event->scenePos(), QTransform()))
          ) {
            rModule->setSelected(false);
            sModule = nullptr;
        }
    }
}

