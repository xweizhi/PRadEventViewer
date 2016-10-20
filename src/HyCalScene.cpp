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
    if(showScalers) {
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

    if(!recon_hits.isEmpty()) {
        QPen pen(Qt::red);
        pen.setWidth(2);
        pen.setCosmetic(true);
        painter->setPen(pen);
        for(auto &hit : recon_hits)
        {
            painter->drawEllipse(hit, 7., 7.);
        }
    }
  
    if (!module_energy.empty()){
       QPen pen(Qt::black);
       pen.setWidth(2);
       pen.setCosmetic(true);
       painter->setPen(pen);
       painter->setFont(QFont("times", 3));

       for (unsigned int i=0; i<module_energy.size(); i++){
          painter->drawText((module_energy.at(i)).second, Qt::TextWordWrap, (module_energy.at(i)).first);
       }
     }
     
     if (!gem_hits.empty()){
        std::map< int, QList<QPointF> >::iterator it;
        QPen pen1(Qt::blue);
        pen1.setWidth(2);
        pen1.setCosmetic(true);
        QPen pen2(Qt::magenta);
        pen2.setWidth(2);
        pen2.setCosmetic(true);
        
        for(it = gem_hits.begin(); it != gem_hits.end(); it++)
        {
          if (it->first == 0) painter->setPen(pen1);
          else painter->setPen(pen2);
          for (auto &hit : it->second ){
              painter->drawEllipse(hit, 3.5, 3.5);
          }
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

void HyCalScene::AddScalerBox(const QString &text, const QColor &textColor, const QRectF &textBox, const QColor &bkgColor)
{
    scalarBoxList.push_back(TextBox(text, textColor, textBox, bkgColor));
}

void HyCalScene::UpdateScalerBox(const QString &text, const int &group)
{
    if(group < 0 || group >= scalarBoxList.size())
        return;

    scalarBoxList[group].text = text;
}

void HyCalScene::UpdateScalerBox(const QStringList &texts)
{
    for(int i = 0; i < texts.size(); ++i)
    {
        UpdateScalerBox(texts.at(i), i);
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
void HyCalScene::AddHyCalHits(const QPointF &p)
{
    recon_hits.append(p);
}

void HyCalScene::AddGEMHits(int igem, const QPointF &hit)
{
    std::map< int, QList<QPointF> >::iterator it;
    it = gem_hits.find(igem);
    if (it != gem_hits.end()){
      (it->second).append(hit);
    }else{
      QList<QPointF> thisList;
      thisList.append(hit);
      gem_hits[igem] = thisList;
    }

}

void HyCalScene::ClearHits()
{
    recon_hits.clear();
    module_energy.clear();
    gem_hits.clear();
}

void HyCalScene::AddEnergyValue(QString s, const QRectF &p)
{
    module_energy.push_back(std::pair<QString, QRectF>(s, p) );
}


