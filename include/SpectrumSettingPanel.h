#ifndef SPECTRUM_SETTING_PANEL_H
#define SPECTRUM_SETTING_PANEL_H

#include <QDialog>


class QSpinBox;
class QSlider;
class QGroupBox;
class QRadioButton;
class Spectrum;

class SpectrumSettingPanel : public QDialog
{
    Q_OBJECT

public:
    SpectrumSettingPanel(QWidget *parent = 0);
    ~SpectrumSettingPanel() {};
    void ConnectSpectrum(Spectrum *s);
    void ChoosePreSetting(int val);

public slots:
    void changeScale();
    void changeType();
    void changeRangeMax(int value);
    void changeRangeMin(int value);
    void changePreSetting();

private:
    Spectrum *spectrum;
    bool preset;

    QGroupBox *createScaleGroup();
    QGroupBox *createTypeGroup();
    QGroupBox *createRangeGroup();
    QGroupBox *createPreSetGroup();

    QRadioButton *linearScale;
    QRadioButton *logScale;

    QRadioButton *rainbow1;
    QRadioButton *rainbow2;
    QRadioButton *greyscale;

    QRadioButton *energyView;
    QRadioButton *occupancyView;
    QRadioButton *voltageView;
    QRadioButton *pedestalView;
    QRadioButton *sigmaView;
    QRadioButton *customView;

    QSpinBox *minSpin;
    QSlider *minSlider;

    QSpinBox *maxSpin;
    QSlider *maxSlider;
};

#endif
