#include "pdv/wav_analysis_plot_widget.h"

#include <QPointF>
#include <QVector>
#include <QFont>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <cmath>
#include <algorithm>

namespace pdv {

SignalChartWidget::SignalChartWidget(QWidget* parent)
    : QChartView(parent)
{
    auto* chart = new QChart();
    m_series = new QLineSeries();

    chart->addSeries(m_series);
    chart->legend()->hide();
    chart->setTitle("Signal");

    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);

    m_axisX = new QValueAxis(this);
    m_axisY = new QValueAxis(this);

    m_axisX->setTitleText("Sample index");
    m_axisY->setTitleText("Amplitude");
    m_axisX->setLabelFormat("%.0f");
    m_axisY->setLabelFormat("%.3f");
    m_axisX->setTickCount(9);
    m_axisX->setMinorTickCount(3);

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(250);

    resetPlot();
}

void SignalChartWidget::resetPlot()
{
    m_series->clear();
    m_axisX->setRange(0, 1);
    m_axisY->setRange(-1, 1);
}

void SignalChartWidget::updatePlot(std::span<const double> segment,
                                   const QString& fromInfo,
                                   const QString& title)
{
    resetPlot();

    if (segment.empty()) {
        return;
    }

    if (fromInfo.isEmpty()) {
        chart()->setTitle(title);
    } else {
        chart()->setTitle(QString("%1 - From sample: %2").arg(title, fromInfo));
    }

    // Downsample data to limit number of plotted points (performance)
    constexpr std::size_t kMaxPoints = 4000;
    const std::size_t step = std::max<std::size_t>(1, (segment.size() + kMaxPoints - 1) / kMaxPoints);

    QVector<QPointF> pts;
    pts.reserve(static_cast<int>((segment.size() + step - 1) / step));

    double minValue = segment.front();
    double maxValue = segment.front();

    for (std::size_t i = 0; i < segment.size(); i += step) {
        const double y = segment[i];
        pts.append(QPointF(static_cast<qreal>(i), static_cast<qreal>(y)));

        if (y < minValue) minValue = y;
        if (y > maxValue) maxValue = y;
    }

    if (pts.isEmpty()) {
        return;
    }

    m_series->replace(pts);
    m_axisX->setRange(0, static_cast<qreal>(segment.size() - 1));

    if (minValue == maxValue) {
        const double pad = (minValue == 0.0) ? 1.0 : std::abs(minValue) * 0.1;
        m_axisY->setRange(minValue - pad, maxValue + pad);
    } else {
        const double pad = (maxValue - minValue) * 0.05;
        m_axisY->setRange(minValue - pad, maxValue + pad);
    }
}

SpectrumChartWidget::SpectrumChartWidget(QWidget* parent)
    : QChartView(parent)
{
    auto* chart = new QChart();
    m_series = new QLineSeries();

    chart->addSeries(m_series);
    chart->legend()->hide();
    chart->setTitle("Spectrum");

    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);

    m_axisX = new QValueAxis(this);
    m_axisY = new QValueAxis(this);

    m_axisX->setTitleText("Frequency [Hz]");
    m_axisY->setTitleText("Magnitude");
    m_axisX->setLabelFormat("%.0f");
    m_axisY->setLabelFormat("%.0f");
    m_axisX->setTickCount(9);
    m_axisX->setMinorTickCount(2);

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(250);

    resetPlot();
}

void SpectrumChartWidget::resetPlot()
{
    m_series->clear();
    m_axisX->setRange(0, 1);
    m_axisY->setRange(0, 1);
}

void SpectrumChartWidget::updatePlot(const pdt::Spectrum& spectrum, const QString& title)
{
    resetPlot();

    const auto& frequencies = spectrum.frequencies;
    const auto& magnitudes = spectrum.magnitudes;

    if (frequencies.empty() || magnitudes.empty() || frequencies.size() != magnitudes.size()) {
        return;
    }

    chart()->setTitle(title);

    // Downsample data to limit number of plotted points (performance)
    constexpr std::size_t kMaxPoints = 4000;
    const std::size_t step = std::max<std::size_t>(1, (frequencies.size() + kMaxPoints - 1) / kMaxPoints);

    QVector<QPointF> pts;
    pts.reserve(static_cast<int>((frequencies.size() + step - 1) / step));

    double minX = frequencies.front();
    double maxX = frequencies.front();
    double minY = magnitudes.front();
    double maxY = magnitudes.front();

    for (std::size_t i = 0; i < frequencies.size(); i += step) {
        const double x = frequencies[i];
        const double y = magnitudes[i];

        pts.append(QPointF(static_cast<qreal>(x), static_cast<qreal>(y)));

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
        const double right = (maxX == 0.0) ? 1.0 : maxX;
        m_axisX->setRange(0.0, right);
    } else {
        m_axisX->setRange(0.0, maxX);
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
