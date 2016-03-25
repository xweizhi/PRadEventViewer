#ifndef SPECTRUM_SETTING_PANEL_H
#define SPECTRUM_SETTING_PANEL_H

#include <QDialog>

#define MAX_RANGE 1000000

class QSpinBox;
class QSlider;
class QGroupBox;
class QRadioButton;
class Spectrum;

class SpectrumSettingPanel : public QDialog {
    Q_OBJECT

public:
    SpectrumSettingPanel(QWidget *parent = 0);
    ~SpectrumSettingPanel() {};
    void ConnectSpectrum(Spectrum *s);

public slots:
    void changeScale();
    void changeType();
    void changeRangeMax(int value);
    void changeRangeMin(int value);

private:
    Spectrum *spectrum;
    QGroupBox *createScaleGroup();
    QGroupBox *createTypeGroup();
    QGroupBox *createRangeGroup();

    QRadioButton *linearScale;
    QRadioButton *logScale;

    QRadioButton *rainbow1;
    QRadioButton *rainbow2;
    QRadioButton *greyscale;

    QSpinBox *minSpin;
    QSlider *minSlider;

    QSpinBox *maxSpin;
    QSlider *maxSlider;
};

#endif
