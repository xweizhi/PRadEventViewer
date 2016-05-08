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
    // this to be impelmented to show the tdc groups and cluster reconstruction
    if(console->GetAnnoType() == ShowTDC && !tdcBoxList.empty()) {
        painter->save();
        painter->setFont(QFont("times", 24, QFont::Bold));
        QPen pen(Qt::black);
        pen.setWidth(2);
        pen.setCosmetic(true);
        painter->setPen(pen);
        for (QList<TextBox>::iterator it = tdcBoxList.begin(); it != tdcBoxList.end(); ++it)
        {
            QPainterPath path;
            path.addRect(it->boxBounding);
            painter->fillPath(path, it->backgroundColor);
            painter->drawPath(path);
            painter->drawText(it->boxBounding,
                              it->boxName,
                              QTextOption(Qt::AlignCenter | Qt::AlignHCenter));

        }
        painter->restore();
    }
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

void HyCalScene::AddTextBox(QString &name, QRectF &textBox, QColor &bkgColor)
{
    TextBox newBox;
    newBox.boxName = name;
    newBox.boxBounding = textBox;
    newBox.backgroundColor = bkgColor;
    tdcBoxList.append(newBox);
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

