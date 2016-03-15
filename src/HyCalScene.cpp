//============================================================================//
// A class derived from QGraphicsScene                                        //
// This class is used for self-defined selection behavior                     //
// in the future will be used for display the reconstructed clusters          //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include <QtGui>
#include "PRadEventViewer.h"
#include "HyCalScene.h"
#include "HyCalModule.h"

void HyCalScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);
    // this to be impelmented to show the tdc groups and cluster reconstruction
    if(console->GetAnnoType() == ShowTDC && !tdcBoxList.empty()) {
        painter->save();
        painter->setFont(QFont("times", 22, QFont::Bold));
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
        pModule = dynamic_cast<HyCalModule*>(itemAt(event->scenePos()));
    else if(event->button() == Qt::RightButton)
        rModule = dynamic_cast<HyCalModule*>(itemAt(event->scenePos()));
}

// overload to change the default selection behavior
void HyCalScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        if(pModule &&
           !pModule->isSelected() &&
           pModule == dynamic_cast<HyCalModule*>(itemAt(event->scenePos()))
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
           rModule == dynamic_cast<HyCalModule*>(itemAt(event->scenePos()))
          ) {
            rModule->setSelected(false);
            sModule = NULL;
        }
    }
}

