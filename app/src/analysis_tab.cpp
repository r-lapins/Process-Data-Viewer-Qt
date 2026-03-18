#include "pdv/analysis_tab.h"

#include <pdt/core/anomaly.h>
#include <pdt/signal/fft.h>
#include <pdt/signal/peak_detection.h>
#include <pdt/signal/window.h>

#include <algorithm>
#include <cmath>
#include <numeric>

#include <QDateTime>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QPointF>
#include <QVector>

namespace pdv {

AnalysisTab::AnalysisTab(const SessionData& session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    m_csvSamplesModel = new CsvSamplesTableModel(this);

    createUi();
    displaySessionData();
    updateStatisticsPanel();
    updateAlertsPanel();
}

QString AnalysisTab::tabTitle() const
{
    const QFileInfo fileInfo(m_session.filePath);
    return fileInfo.fileName();
}

void AnalysisTab::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // ===== LEFT: DATA =====
    auto* dataGroup = new QGroupBox("Data", splitter);
    auto* dataLayout = new QVBoxLayout(dataGroup);

    m_dataPlaceholderLabel = new QLabel("No data", dataGroup);
    m_dataPlaceholderLabel->setWordWrap(true);

    m_samplesTableView = new QTableView(dataGroup);
    m_samplesTableView->setModel(m_csvSamplesModel);
    m_samplesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_samplesTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_samplesTableView->setAlternatingRowColors(true);
    m_samplesTableView->setSortingEnabled(false);
    m_samplesTableView->horizontalHeader()->setStretchLastSection(true);
    m_samplesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    // ===== SIGNAL CHART =====
    auto* chart = new QChart();
    m_signalSeries = new QLineSeries();
    chart->addSeries(m_signalSeries);
    chart->legend()->hide();
    chart->setTitle("Signal");

    m_signalAxisX = new QValueAxis(this);
    m_signalAxisY = new QValueAxis(this);

    m_signalAxisX->setTitleText("Sample");
    m_signalAxisY->setTitleText("Amplitude");
    m_signalAxisX->setLabelFormat("%.0f");
    m_signalAxisY->setLabelFormat("%.3f");

    chart->addAxis(m_signalAxisX, Qt::AlignBottom);
    chart->addAxis(m_signalAxisY, Qt::AlignLeft);

    m_signalSeries->attachAxis(m_signalAxisX);
    m_signalSeries->attachAxis(m_signalAxisY);

    m_signalChartView = new QChartView(chart, dataGroup);
    m_signalChartView->setRenderHint(QPainter::Antialiasing);

    dataLayout->addWidget(m_dataPlaceholderLabel);
    dataLayout->addWidget(m_samplesTableView);
    dataLayout->addWidget(m_signalChartView);

    m_samplesTableView->hide();
    m_signalChartView->hide();

    // ===== RIGHT PANEL =====
    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);

    // === STATS ===
    auto* statsGroup = new QGroupBox("Statistics", rightPanel);
    auto* statsLayout = new QFormLayout(statsGroup);

    m_statsFileTypeValueLabel = new QLabel("-");
    m_statsSampleRateValueLabel = new QLabel("-");
    m_statsChannelsValueLabel = new QLabel("-");
    m_statsCountValueLabel = new QLabel("-");
    m_statsMinValueLabel = new QLabel("-");
    m_statsMaxValueLabel = new QLabel("-");
    m_statsMeanValueLabel = new QLabel("-");
    m_statsStddevValueLabel = new QLabel("-");

    statsLayout->addRow("File type:", m_statsFileTypeValueLabel);
    statsLayout->addRow("Sample rate:", m_statsSampleRateValueLabel);
    statsLayout->addRow("Channels:", m_statsChannelsValueLabel);
    statsLayout->addRow("Count:", m_statsCountValueLabel);
    statsLayout->addRow("Min:", m_statsMinValueLabel);
    statsLayout->addRow("Max:", m_statsMaxValueLabel);
    statsLayout->addRow("Mean:", m_statsMeanValueLabel);
    statsLayout->addRow("Stddev:", m_statsStddevValueLabel);

    // === ALERTS ===
    auto* alertsGroup = new QGroupBox("Alerts", rightPanel);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    rightLayout->addWidget(statsGroup);
    rightLayout->addWidget(alertsGroup);

    splitter->addWidget(dataGroup);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    rootLayout->addWidget(splitter);

    resetSignalPlot();
}

void AnalysisTab::displaySessionData()
{
    switch (m_session.kind) {
    case SessionData::FileKind::Csv: {
        m_csvSamplesModel->setDataSet(m_session.dataSet);

        // m_samplesTableView->setModel(m_csvSamplesModel);
        m_samplesTableView->show();
        m_signalChartView->hide();
        m_dataPlaceholderLabel->hide();

        const int col = 0;
        int width = m_samplesTableView->columnWidth(col);
        m_samplesTableView->setColumnWidth(col, width + 20);

        break;
    }

    case SessionData::FileKind::Wav: {
        m_csvSamplesModel->clear();
        m_samplesTableView->hide();

        updateSignalPlot();
        m_dataPlaceholderLabel->hide();

        break;
    }

    default:
        break;
    }
}

void AnalysisTab::resetStatisticsPanel()
{
    m_statsFileTypeValueLabel->setText("-");
    m_statsSampleRateValueLabel->setText("-");
    m_statsChannelsValueLabel->setText("-");
    m_statsCountValueLabel->setText("-");
    m_statsMinValueLabel->setText("-");
    m_statsMaxValueLabel->setText("-");
    m_statsMeanValueLabel->setText("-");
    m_statsStddevValueLabel->setText("-");
}

void AnalysisTab::updateStatisticsPanel()
{
    resetStatisticsPanel();

    switch (m_session.kind) {
    case SessionData::FileKind::Csv: {
        m_statsFileTypeValueLabel->setText("CSV");

        const auto stats = m_session.dataSet->stats();

        m_statsCountValueLabel->setText(QString::number(stats.count));
        m_statsMinValueLabel->setText(QString::number(stats.min));
        m_statsMaxValueLabel->setText(QString::number(stats.max));
        m_statsMeanValueLabel->setText(QString::number(stats.mean));
        m_statsStddevValueLabel->setText(QString::number(stats.stddev));

        break;
    }

    case SessionData::FileKind::Wav: {
        m_statsFileTypeValueLabel->setText("WAV");

        const auto& wav = *m_session.wavData;
        const auto& samples = wav.samples;

        m_statsSampleRateValueLabel->setText(QString::number(wav.sample_rate));
        m_statsChannelsValueLabel->setText(QString::number(wav.channels));
        m_statsCountValueLabel->setText(QString::number(samples.size()));
        // m_statsCountValueLabel->setText(QString::number(static_cast<qulonglong>(samples.size())));


        if (!samples.empty()) {
            const auto [minIt, maxIt] = std::minmax_element(samples.begin(), samples.end());
            const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
            const double mean = sum / samples.size();
            // const double mean = sum / static_cast<double>(samples.size());

            double sqSum = 0.0;
            for (double s : samples) {
                double d = s - mean;
                sqSum += d * d;
            }

            const double stddev = std::sqrt(sqSum / samples.size());

            m_statsMinValueLabel->setText(QString::number(*minIt));
            m_statsMaxValueLabel->setText(QString::number(*maxIt));
            m_statsMeanValueLabel->setText(QString::number(mean));
            m_statsStddevValueLabel->setText(QString::number(stddev));
        }

        break;
    }

    default:
        break;
    }
}

void AnalysisTab::resetAlertsPanel()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

void AnalysisTab::updateAlertsPanel()
{
    resetAlertsPanel();

    switch (m_session.kind) {
    case SessionData::FileKind::Csv: {
        const auto summary = pdt::detect_zscore_global(*m_session.dataSet, 3.0, 20);

        m_alertsListWidget->clear();

        if (summary.top.empty()) {
            m_alertsListWidget->addItem("No anomalies");
            return;
        }

        for (const auto& a : summary.top) {
            const auto ts = std::chrono::system_clock::to_time_t(a.timestamp);
            const QString timestampText = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts)).toString(Qt::ISODate);

            m_alertsListWidget->addItem(
                QString("time=%1 | sensor=%2 | value=%3 | z=%4")
                    .arg(timestampText,
                         QString::fromStdString(a.sensor),
                         QString::number(a.value, 'g', 10),
                         QString::number(a.zscore, 'g', 10))
            );
        }

        break;
    }

    case SessionData::FileKind::Wav: {
        const auto& wav = *m_session.wavData;
        const auto& samples = wav.samples;

        constexpr std::size_t kMaxSpectrumSamples = 4096;
        const std::size_t n = std::min(samples.size(), kMaxSpectrumSamples);

        std::vector<double> segment(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(n));

        const auto windowed = pdt::apply_window(segment, pdt::WindowType::Hann);
        const auto spectrum = pdt::compute_spectrum(windowed, static_cast<double>(wav.sample_rate));


        const auto peaks = pdt::detect_dominant_peaks(
            spectrum,
            0.20,
            pdt::PeakDetectionMode::ThresholdOnly,
            30
            );

        if (peaks.empty()) {
            m_alertsListWidget->addItem("No dominant spectral peaks detected");
            return;
        }

        m_alertsListWidget->clear();

        m_alertsListWidget->addItem(
            QString("Detected spectral peaks: %1 (first %2 samples)")
                .arg(static_cast<qulonglong>(peaks.size()))
                .arg(static_cast<qulonglong>(n))
            );

        for (const auto& peak : peaks) {
            const QString itemText = QString("bin=%1 | freq=%2 Hz | mag=%3")
            .arg(static_cast<qulonglong>(peak.index))
                .arg(peak.frequency, 0, 'g', 10)
                .arg(peak.magnitude, 0, 'g', 10);

            m_alertsListWidget->addItem(itemText);
        }

        break;
    }

    default:
        m_alertsListWidget->addItem("No alerts available");
        break;
    }
}

void AnalysisTab::resetSignalPlot()
{
    m_signalSeries->clear();
    m_signalAxisX->setRange(0, 1);
    m_signalAxisY->setRange(-1, 1);
    m_signalChartView->hide();
}

void AnalysisTab::updateSignalPlot()
{
    resetSignalPlot();

    const auto& samples = m_session.wavData->samples;

    if (samples.empty()) {
        return;
    }

    constexpr std::size_t maxPoints = 2000;
    const std::size_t step = std::max<std::size_t>(1, (samples.size() + maxPoints - 1) / maxPoints);

    if (m_signalChartView != nullptr && m_signalChartView->chart() != nullptr) {
        const QFileInfo fileInfo(m_session.filePath);
        m_signalChartView->chart()->setTitle(QString("Signal plot - %1").arg(fileInfo.fileName()));
    }

    QVector<QPointF> pts;
    pts.reserve(static_cast<int>((samples.size() + step - 1) / step));

    double min = samples[0];
    double max = samples[0];

    for (std::size_t i = 0; i < samples.size(); i += step) {
        const double y = samples[i];
        pts.append(QPointF(static_cast<qreal>(i), static_cast<qreal>(y)));

        if (y < min) min = y;
        if (y > max) max = y;
    }

    if (pts.isEmpty()) {
        return;
    }

    m_signalSeries->replace(pts);

    m_signalAxisX->setRange(0, static_cast<qreal>(samples.size() - 1));

    if (min == max) {
        const double pad = (min == 0.0) ? 1.0 : std::abs(min) * 0.1;
        m_signalAxisY->setRange(min - pad, max + pad);
    } else {
        const double pad = (max - min) * 0.05;
        m_signalAxisY->setRange(min - pad, max + pad);
    }

    m_signalChartView->show();
}

} // namespace pdv
