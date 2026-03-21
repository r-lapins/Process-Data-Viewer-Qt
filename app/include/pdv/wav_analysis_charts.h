#pragma once

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <vector>

namespace pdv {

class SignalChartWidget : public QChartView
{
    Q_OBJECT

public:
    explicit SignalChartWidget(QWidget* parent = nullptr);

    void resetPlot();
    void updatePlot(const std::vector<double>& segment,
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
        const std::vector<double>& frequencies,
        const std::vector<double>& magnitudes,
        const QString& title
        );

private:
    QLineSeries* m_series = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
};

} // namespace pdv
