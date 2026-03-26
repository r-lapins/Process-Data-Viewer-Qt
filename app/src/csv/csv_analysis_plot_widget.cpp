#include "pdv/csv/csv_analysis_plot_widget.h"

#include <QFont>
#include <QPointF>
#include <QVector>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>

#include <numeric>
#include <algorithm>
#include <cmath>

namespace pdv {

CsvAnalysisPlotWidget::CsvAnalysisPlotWidget(QWidget* parent)
    : QChartView(parent)
{
    auto* chart = new QChart();
    m_series = new QLineSeries();
    m_anomalySeries = new QScatterSeries();

    chart->addSeries(m_series);
    chart->addSeries(m_anomalySeries);
    chart->legend()->hide();
    chart->setTitle("CSV plot");

    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);

    m_axisX = new QValueAxis(this);
    m_axisY = new QValueAxis(this);

    m_axisX->setTitleText("Sample index");
    m_axisY->setTitleText("Value");
    m_axisX->setLabelFormat("%.0f");
    m_axisY->setLabelFormat("%.3f");
    m_axisX->setTickCount(9);
    m_axisX->setMinorTickCount(1);

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
    m_anomalySeries->attachAxis(m_axisX);
    m_anomalySeries->attachAxis(m_axisY);
    m_anomalySeries->setMarkerSize(9.0);
    m_anomalySeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    m_anomalySeries->setColor(QColorConstants::DarkMagenta);

    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(250);

    resetPlot();
}

void CsvAnalysisPlotWidget::resetPlot()
{
    m_series->clear();
    m_anomalySeries->clear();
    chart()->setTitle("CSV plot");
    m_axisX->setRange(1, 2);
    m_axisY->setRange(0, 1);
}

void CsvAnalysisPlotWidget::updatePlot(std::span<const double> yValues, const QString& title)
{
    if (yValues.empty()) {
        resetPlot();
        return;
    }

    // X axis: sequential sample indices (1, 2, ..., N)
    std::vector<double> xValues(yValues.size());
    std::iota(xValues.begin(), xValues.end(), 1.0);

    updatePlot(xValues, yValues, title);
}

void CsvAnalysisPlotWidget::updatePlot(
    std::span<const double> xValues,
    std::span<const double> yValues,
    const QString& title)
{
    resetPlot();

    if (xValues.empty() || yValues.empty() || xValues.size() != yValues.size()) {
        return;
    }

    chart()->setTitle(title);

    // Downsample data to limit number of plotted points (performance)
    constexpr std::size_t kMaxPoints = 4000;
    const std::size_t step = std::max<std::size_t>(1, (xValues.size() + kMaxPoints - 1) / kMaxPoints);

    QVector<QPointF> pts;
    pts.reserve(static_cast<int>((xValues.size() + step - 1) / step));

    double minX = xValues.front();
    double maxX = xValues.front();
    double minY = yValues.front();
    double maxY = yValues.front();

    for (std::size_t i = 0; i < xValues.size(); i += step) {
        const double x = xValues[i];
        const double y = yValues[i];

        pts.append(QPointF(x, y));

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    if (pts.isEmpty()) {
        return;
    }

    m_series->replace(pts);

    if (minX == maxX) {
        m_axisX->setRange(minX, minX + 1.0);
    } else {
        m_axisX->setRange(minX, maxX);
    }

    if (minY == maxY) {
        const double padY = (minY == 0.0) ? 1.0 : std::abs(minY) * 0.1;
        m_axisY->setRange(minY - padY, maxY + padY);
    } else {
        const double padY = (maxY - minY) * 0.05;
        m_axisY->setRange(minY - padY, maxY + padY);
    }
}

void CsvAnalysisPlotWidget::updatePlotWithMarkers(
    std::span<const double> xValues,
    std::span<const double> yValues,
    std::span<const double> markerXValues,
    double markerY,
    const QString& title
    )
{
    resetPlot();

    if (xValues.empty() || yValues.empty() || xValues.size() != yValues.size()) {
        return;
    }

    chart()->setTitle(title);
    m_axisX->setTitleText("Sample index");
    m_axisY->setTitleText("Value");

    // Downsample data to limit number of plotted points (performance)
    constexpr std::size_t kMaxPoints = 4000;
    const std::size_t step = std::max<std::size_t>(1, (xValues.size() + kMaxPoints - 1) / kMaxPoints);

    QVector<QPointF> linePts;
    linePts.reserve(static_cast<int>((xValues.size() + step - 1) / step));

    double minX = xValues.front();
    double maxX = xValues.front();
    double minY = yValues.front();
    double maxY = yValues.front();

    for (std::size_t i = 0; i < xValues.size(); i += step) {
        const double x = xValues[i];
        const double y = yValues[i];

        linePts.append(QPointF(x, y));

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    if (linePts.isEmpty()) {
        return;
    }

    m_series->replace(linePts);

    QVector<QPointF> markerPts;
    markerPts.reserve(static_cast<int>(markerXValues.size()));

    for (double markerX : markerXValues) {
        markerPts.append(QPointF(markerX, markerY));
    }

    m_anomalySeries->replace(markerPts);

    if (!markerXValues.empty()) {
        if (markerY < minY) minY = markerY;
        if (markerY > maxY) maxY = markerY;
    }

    if (minX == maxX) {
        m_axisX->setRange(minX, minX + 1.0);
    } else {
        m_axisX->setRange(minX, maxX);
    }

    if (minY == maxY) {
        const double padY = (minY == 0.0) ? 1.0 : std::abs(minY) * 0.1;
        m_axisY->setRange(minY - padY, maxY + padY);
    } else {
        const double padY = (maxY - minY) * 0.05;
        m_axisY->setRange(minY - padY, maxY + padY);
    }
}

} // namespace pdv
