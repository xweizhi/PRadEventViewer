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
: QDialog(parent), spectrum(nullptr), preset(false)
{
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(createScaleGroup(), 0, 0);
    grid->addWidget(createTypeGroup(), 0, 1);
    grid->addWidget(createRangeGroup(), 1, 0, 1, 2);
    grid->addWidget(createPreSetGroup(), 2, 0, 1, 2);
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
        connect(energyView, SIGNAL(clicked()), this, SLOT(changePreSetting()));
        connect(occupancyView, SIGNAL(clicked()), this, SLOT(changePreSetting()));
        connect(pedestalView, SIGNAL(clicked()), this, SLOT(changePreSetting()));
        connect(sigmaView, SIGNAL(clicked()), this, SLOT(changePreSetting()));
        connect(voltageView, SIGNAL(clicked()), this, SLOT(changePreSetting()));
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

QGroupBox *SpectrumSettingPanel::createPreSetGroup()
{
    QGroupBox *presetGroup = new QGroupBox(tr("Spectrum Pre-Settings"));

    energyView = new QRadioButton("Energy View");
    occupancyView = new QRadioButton("Occupancy View");
    pedestalView = new QRadioButton("Pedestal View");
    sigmaView = new QRadioButton("Ped. Sigma View");
    voltageView = new QRadioButton("High Voltage View");
    customView = new QRadioButton("Custom View");

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(energyView, 0, 0);
    layout->addWidget(occupancyView, 0, 1);
    layout->addWidget(pedestalView, 1, 0);
    layout->addWidget(sigmaView, 1, 1);
    layout->addWidget(voltageView, 2, 0);
    layout->addWidget(customView, 2, 1);

    presetGroup->setLayout(layout);

    return presetGroup;
}

void SpectrumSettingPanel::changeScale()
{
    if(!preset)
        customView->setChecked(true);

    if(linearScale->isChecked())
        spectrum->SetSpectrumScale(Spectrum::LinearScale);
    else
        spectrum->SetSpectrumScale(Spectrum::LogScale);
}

void SpectrumSettingPanel::changeType()
{
    if(!preset)
        customView->setChecked(true);

    if(rainbow1->isChecked())
        spectrum->SetSpectrumType(Spectrum::Rainbow1);
    else if(rainbow2->isChecked())
        spectrum->SetSpectrumType(Spectrum::Rainbow2);
    else
        spectrum->SetSpectrumType(Spectrum::GreyScale);
}

void SpectrumSettingPanel::changeRangeMin(int value)
{
    if(!preset)
        customView->setChecked(true);

    minSlider->setValue(value);
    spectrum->SetSpectrumRangeMin(value);
}

void SpectrumSettingPanel::changeRangeMax(int value)
{
    if(!preset)
        customView->setChecked(true);

    maxSlider->setValue(value);
    minSlider->setRange(0, value - 10);
    minSpin->setRange(0, value - 10);
    spectrum->SetSpectrumRangeMax(value);
}

void SpectrumSettingPanel::changePreSetting()
{
    preset = true;
    if(energyView->isChecked()) {
        maxSpin->setValue(1000);
        minSpin->setValue(1);
        logScale->setChecked(true);
    } else if(occupancyView->isChecked()) {
        maxSpin->setValue(100000);
        minSpin->setValue(1);
        logScale->setChecked(true);
    } else if(voltageView->isChecked()) {
        maxSpin->setValue(1700);
        minSpin->setValue(900);
        linearScale->setChecked(true);
    } else if(pedestalView->isChecked()) {
        maxSpin->setValue(1000);
        minSpin->setValue(200);
        linearScale->setChecked(true);
    } else if(sigmaView->isChecked()) {
        maxSpin->setValue(30);
        minSpin->setValue(0);
        linearScale->setChecked(true);
    }

    changeScale();
    preset = false;
}

void SpectrumSettingPanel::ChoosePreSetting(int val)
{
    switch(val)
    {
    case 0:
        energyView->setChecked(true);
        break;
    case 1:
        occupancyView->setChecked(true);
        break;
    case 2:
        pedestalView->setChecked(true);
        break;
    case 3:
        sigmaView->setChecked(true);
        break; 
    case 4:
    case 5:
        voltageView->setChecked(true);
        break;
    default: return;
    }
    changePreSetting();
}
