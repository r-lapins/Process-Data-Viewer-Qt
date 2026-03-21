#pragma once

#include <QtCharts/QChartView>

#include <span>

class QValueAxis;
class QLineSeries;
class QScatterSeries;
class QWidget;
class QString;

namespace pdv {

class CsvPlotWidget : public QChartView
{
    Q_OBJECT

public:
    explicit CsvPlotWidget(QWidget* parent = nullptr);

    void resetPlot();
    void updatePlot(std::span<const double> yValues, const QString& title);

    void updatePlot(
        std::span<const double> xValues,
        std::span<const double> yValues,
        const QString& title
        );

    void updatePlotWithMarkers(
        std::span<const double> xValues,
        std::span<const double> yValues,
        std::span<const double> markerXValues,
        double markerY,
        const QString& title
        );

private:
    QLineSeries* m_series = nullptr;
    QScatterSeries* m_anomalySeries = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;

};

} // namespace pdv
