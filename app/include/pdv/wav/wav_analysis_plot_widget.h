#pragma once

#include <QtCharts/QChartView>
#include "pdt/dsp/spectrum.h"

#include <complex>
#include <span>

class QValueAxis;
class QLineSeries;
class QWidget;
class QString;

namespace pdv {

class SignalChartWidget : public QChartView
{
    Q_OBJECT

public:
    explicit SignalChartWidget(QWidget* parent = nullptr);

    void resetPlot();
    void updatePlot(std::span<const double> segment, const QString& fromInfo, const QString& title);
    void updateIqPlot(std::span<const std::complex<float>> samples, const QString& title);

private:
    QLineSeries* m_series = nullptr;
    QLineSeries* m_qSeries = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
};

class SpectrumChartWidget : public QChartView
{
    Q_OBJECT

public:
    explicit SpectrumChartWidget(QWidget* parent = nullptr);

    void resetPlot();
    void updatePlot(const pdt::Spectrum& spectrum, const QString& title);

private:
    QLineSeries* m_series = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
};

} // namespace pdv
