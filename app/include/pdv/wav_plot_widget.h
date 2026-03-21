#pragma once

#include <QtCharts/QChartView>

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
    void updatePlot(std::span<const double> segment,
                    const QString& fromInfo,
                    const QString& title);

private:
    QLineSeries* m_series = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
};

class SpectrumChartWidget : public QChartView
{
    Q_OBJECT

public:
    explicit SpectrumChartWidget(QWidget* parent = nullptr);

    void resetPlot();
    void updatePlot(
        std::span<const double> frequencies,
        std::span<const double> magnitudes,
        const QString& title
        );

private:
    QLineSeries* m_series = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
};

} // namespace pdv
