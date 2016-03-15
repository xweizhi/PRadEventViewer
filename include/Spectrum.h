#ifndef PRAD_SPECTRUM_H
#define PRAD_SPECTRUM_H

#include <QColor>
#include <QGraphicsObject>
#include <QObject>
#include <QPainter>

class Spectrum : public QGraphicsObject
{
    Q_OBJECT

public:
    enum SpectrumType
    {
        Rainbow1,
        Rainbow2,
        GreyScale,
    };

    enum SpectrumScale
    {
        LogScale,
        LinearScale,
    };

    struct SettingData
    {
        SpectrumType type;
        SpectrumScale scale;
        int range_min;
        int range_max;
    };

    Spectrum(int w, int h, int x1, int x2,
             double l1 = 440, double l2 = 640,
             SpectrumType type = Rainbow1, SpectrumScale scale = LogScale);
    void SetSpectrumRange(const int &x1, const int &x2);
    void SetSpectrumRangeMin(int &min);
    void SetSpectrumRangeMax(int &max);
    void SetSpectrumType(const SpectrumType &type);
    void SetSpectrumScale(const SpectrumScale &scale);
    SettingData &GetCurrentSetting() {return settings;};
    QColor GetColor(const double &val);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void paintTicks(QPainter *painter);
    QRectF boundingRect() const;

signals:
    void spectrumChanged();

private:
    void updateGradient();
    double scaling(const double &val);
    QColor scaleToColor(const double &scale);
    SettingData settings;
    double wavelength1;
    double wavelength2;
    int width;
    int height;
    QLinearGradient gradient;
    QPainterPath shape;
};

#endif
