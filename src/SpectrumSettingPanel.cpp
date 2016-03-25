//============================================================================//
// Derived from QDialog, provide a setting panel to change Spectrum settings  //
//                                                                            //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include "SpectrumSettingPanel.h"
#include "Spectrum.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>

SpectrumSettingPanel::SpectrumSettingPanel(QWidget *parent)
: QDialog(parent), spectrum(nullptr)
{
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(createScaleGroup(), 0, 0);
    grid->addWidget(createTypeGroup(), 0, 1);
    grid->addWidget(createRangeGroup(), 1, 0, 1, 2);
    setLayout(grid);
    setWindowTitle(tr("Spectrum Settings"));
}

void SpectrumSettingPanel::ConnectSpectrum(Spectrum *s)
{
    if(!s)
        return;

    if(!spectrum) {
        connect(linearScale, SIGNAL(clicked()), this, SLOT(changeScale()));
        connect(logScale, SIGNAL(clicked()), this, SLOT(changeScale()));
        connect(rainbow1, SIGNAL(clicked()), this, SLOT(changeType()));
        connect(rainbow2, SIGNAL(clicked()), this, SLOT(changeType()));
        connect(greyscale, SIGNAL(clicked()), this, SLOT(changeType()));
        connect(minSpin, SIGNAL(valueChanged(int)), this, SLOT(changeRangeMin(int)));
        connect(maxSpin, SIGNAL(valueChanged(int)), this, SLOT(changeRangeMax(int)));
    }

    spectrum = s;

    switch(s->GetCurrentSetting().type) {
    case Spectrum::Rainbow1:
        rainbow1->setChecked(true); break;
    case Spectrum::Rainbow2:
        rainbow2->setChecked(true); break;
    case Spectrum::GreyScale:
        greyscale->setChecked(true); break;
    }

    switch(s->GetCurrentSetting().scale) {
    case Spectrum::LinearScale:
        linearScale->setChecked(true); break;
    case Spectrum::LogScale:
        logScale->setChecked(true); break;
    }

    maxSpin->setValue(s->GetCurrentSetting().range_max);
    minSpin->setValue(s->GetCurrentSetting().range_min);
}

QGroupBox *SpectrumSettingPanel::createScaleGroup()
{
    QGroupBox *scaleGroup = new QGroupBox(tr("Scale Setting"));
    linearScale = new QRadioButton(tr("Linear Scale"));
    logScale = new QRadioButton(tr("Logarithmic Scale"));


    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(linearScale);
    layout->addWidget(logScale);

    scaleGroup->setLayout(layout);

    return scaleGroup;
}

QGroupBox *SpectrumSettingPanel::createTypeGroup()
{
    QGroupBox *typeGroup = new QGroupBox(tr("Spectrum Type"));
    rainbow1 = new QRadioButton(tr("Rainbow 1"));
    rainbow2 = new QRadioButton(tr("Rainbow 2"));
    greyscale = new QRadioButton(tr("Grey Scale"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rainbow1);
    layout->addWidget(rainbow2);
    layout->addWidget(greyscale);

    typeGroup->setLayout(layout);

    return typeGroup;
}

QGroupBox *SpectrumSettingPanel::createRangeGroup()
{
    QGroupBox *rangeGroup = new QGroupBox(tr("Spectrum Range Setting"));
    minSpin = new QSpinBox();
    minSlider = new QSlider(Qt::Horizontal);
    maxSpin = new QSpinBox();
    maxSlider = new QSlider(Qt::Horizontal);

    minSpin->setRange(0, MAX_RANGE);
    minSlider->setRange(0, MAX_RANGE);
    maxSpin->setRange(10, MAX_RANGE);
    maxSlider->setRange(10, MAX_RANGE);

    connect(minSlider, SIGNAL(valueChanged(int)), minSpin, SLOT(setValue(int)));
    connect(minSpin, SIGNAL(valueChanged(int)), minSlider, SLOT(setValue(int)));

    connect(maxSlider, SIGNAL(valueChanged(int)), maxSpin, SLOT(setValue(int)));
    connect(maxSpin, SIGNAL(valueChanged(int)), maxSlider, SLOT(setValue(int)));

    QFormLayout *layout = new QFormLayout;
    layout->addRow(new QLabel(tr("Minimum value")), minSpin);
    layout->addRow(minSlider);
    layout->addRow(new QLabel(tr("Maximum value")), maxSpin);
    layout->addRow(maxSlider);

    rangeGroup->setLayout(layout);

    return rangeGroup;
}

void SpectrumSettingPanel::changeScale()
{
    if(linearScale->isChecked())
        spectrum->SetSpectrumScale(Spectrum::LinearScale);
    else
        spectrum->SetSpectrumScale(Spectrum::LogScale);
}

void SpectrumSettingPanel::changeType()
{
    if(rainbow1->isChecked())
        spectrum->SetSpectrumType(Spectrum::Rainbow1);
    else if(rainbow2->isChecked())
        spectrum->SetSpectrumType(Spectrum::Rainbow2);
    else
        spectrum->SetSpectrumType(Spectrum::GreyScale);
}

void SpectrumSettingPanel::changeRangeMin(int value)
{
    minSlider->setValue(value);
    spectrum->SetSpectrumRangeMin(value);
}

void SpectrumSettingPanel::changeRangeMax(int value)
{
    maxSlider->setValue(value);
    minSlider->setRange(0, value - 10);
    minSpin->setRange(0, value - 10);
    spectrum->SetSpectrumRangeMax(value);
}
