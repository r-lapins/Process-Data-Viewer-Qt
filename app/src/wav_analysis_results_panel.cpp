#include "pdv/wav_analysis_results_panel.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace pdv {

WavAnalysisResultsPanel::WavAnalysisResultsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(10);

    auto* statsPanel = createStatisticsPanel(this);
    auto* alertsPanel = createAlertsPanel(this);

    rootLayout->addWidget(statsPanel, 0, Qt::AlignTop);
    rootLayout->addWidget(alertsPanel, 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    clear();
}

void WavAnalysisResultsPanel::clear()
{
    clearStatistics();
    clearAlerts();
}

void WavAnalysisResultsPanel::setResults(const SessionData& session, const WavAnalysisEngine::AnalysisResult& result)
{
    renderStatistics(session, result);
    renderAlerts(session, result);
}

QWidget* WavAnalysisResultsPanel::createStatisticsPanel(QWidget* parent)
{
    auto* statsGroup = new QGroupBox("Statistics", parent);
    auto* statsLayout = new QFormLayout(statsGroup);

    m_statsFileTypeValueLabel = new QLabel("-", statsGroup);
    m_statsSampleRateValueLabel = new QLabel("-", statsGroup);
    m_statsChannelsValueLabel = new QLabel("-", statsGroup);
    m_statsTotalSamplesValueLabel = new QLabel("-", statsGroup);
    m_statsUsedFromValueLabel = new QLabel("-", statsGroup);
    m_statsWindowSizeValueLabel = new QLabel("-", statsGroup);
    m_statsWindowValueLabel = new QLabel("-", statsGroup);
    m_statsAlgorithmValueLabel = new QLabel("-", statsGroup);
    m_statsThresholdValueLabel = new QLabel("-", statsGroup);
    m_statsPeakModeValueLabel = new QLabel("-", statsGroup);
    m_statsDetectedPeaksValueLabel = new QLabel("-", statsGroup);
    m_statsMinValueLabel = new QLabel("-", statsGroup);
    m_statsMaxValueLabel = new QLabel("-", statsGroup);
    m_statsMeanValueLabel = new QLabel("-", statsGroup);
    m_statsStddevValueLabel = new QLabel("-", statsGroup);

    statsLayout->addRow("File type:", m_statsFileTypeValueLabel);
    statsLayout->addRow("Sample rate:", m_statsSampleRateValueLabel);
    statsLayout->addRow("Channels:", m_statsChannelsValueLabel);
    statsLayout->addRow("Algorithm:", m_statsAlgorithmValueLabel);
    statsLayout->addRow("Window:", m_statsWindowValueLabel);
    statsLayout->addRow("Peak mode:", m_statsPeakModeValueLabel);
    statsLayout->addRow("Total samples:", m_statsTotalSamplesValueLabel);
    statsLayout->addRow("From sample:", m_statsUsedFromValueLabel);
    statsLayout->addRow("Window size:", m_statsWindowSizeValueLabel);
    statsLayout->addRow("Detected peaks:", m_statsDetectedPeaksValueLabel);
    statsLayout->addRow("Threshold:", m_statsThresholdValueLabel);

    auto* signalGroup = new QGroupBox("Signal", statsGroup);
    auto* signalLayout = new QHBoxLayout(signalGroup);
    signalLayout->setContentsMargins(0, 0, 0, 0);

    auto* signalLeftWidget = new QWidget(signalGroup);
    auto* signalLeftLayout = new QFormLayout(signalLeftWidget);
    signalLeftLayout->setContentsMargins(0, 0, 0, 0);

    auto* signalRightWidget = new QWidget(signalGroup);
    auto* signalRightLayout = new QFormLayout(signalRightWidget);
    signalRightLayout->setContentsMargins(0, 0, 0, 0);

    signalLeftLayout->addRow("Min:", m_statsMinValueLabel);
    signalLeftLayout->addRow("Max:", m_statsMaxValueLabel);

    signalRightLayout->addRow("Mean:", m_statsMeanValueLabel);
    signalRightLayout->addRow("Stddev:", m_statsStddevValueLabel);

    signalLayout->addWidget(signalLeftWidget);
    signalLayout->addWidget(signalRightWidget);

    statsLayout->addRow(signalGroup);

    statsLayout->setLabelAlignment(Qt::AlignLeft);
    statsLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    statsGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    statsGroup->setMinimumWidth(250);

    return statsGroup;
}

QWidget* WavAnalysisResultsPanel::createAlertsPanel(QWidget* parent)
{
    auto* alertsGroup = new QGroupBox("Alerts", parent);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    alertsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return alertsGroup;
}

void WavAnalysisResultsPanel::clearStatistics()
{
    m_statsFileTypeValueLabel->setText("-");
    m_statsSampleRateValueLabel->setText("-");
    m_statsChannelsValueLabel->setText("-");
    m_statsTotalSamplesValueLabel->setText("-");
    m_statsUsedFromValueLabel->setText("-");
    m_statsWindowSizeValueLabel->setText("-");
    m_statsWindowValueLabel->setText("-");
    m_statsAlgorithmValueLabel->setText("-");
    m_statsThresholdValueLabel->setText("-");
    m_statsPeakModeValueLabel->setText("-");
    m_statsDetectedPeaksValueLabel->setText("-");
    m_statsMinValueLabel->setText("-");
    m_statsMaxValueLabel->setText("-");
    m_statsMeanValueLabel->setText("-");
    m_statsStddevValueLabel->setText("-");
}

void WavAnalysisResultsPanel::renderStatistics(const SessionData& session, const WavAnalysisEngine::AnalysisResult& result)
{
    clearStatistics();

    if (!session.wavData.has_value()) {
        return;
    }

    const auto& wav = *session.wavData;
    const auto& settings = result.usedSettings;

    m_statsFileTypeValueLabel->setText("WAV");
    m_statsSampleRateValueLabel->setText(QString::number(wav.sample_rate));
    m_statsChannelsValueLabel->setText(QString::number(wav.channels));
    m_statsTotalSamplesValueLabel->setText(QString::number(static_cast<qulonglong>(wav.samples.size())));
    m_statsUsedFromValueLabel->setText(QString::number(static_cast<qulonglong>(settings.from)));
    m_statsWindowSizeValueLabel->setText(QString::number(static_cast<qulonglong>(result.rawSegment.size())));
    m_statsAlgorithmValueLabel->setText(toString(settings.algorithm));
    m_statsThresholdValueLabel->setText(QString::number(settings.threshold, 'g', 10));
    m_statsPeakModeValueLabel->setText(toString(settings.peakMode));
    m_statsDetectedPeaksValueLabel->setText(QString::number(static_cast<qulonglong>(result.allPeaks.size())));

    m_statsMinValueLabel->setText(QString::number(result.minValue, 'f', 3));
    m_statsMaxValueLabel->setText(QString::number(result.maxValue, 'f', 3));
    m_statsMeanValueLabel->setText(QString::number(result.meanValue, 'e', 1));
    m_statsStddevValueLabel->setText(QString::number(result.stddevValue, 'f', 3));

    if (settings.useWindow) {
        m_statsWindowValueLabel->setText(toString(settings.window));
    } else {
        m_statsWindowValueLabel->setText("None");
    }
}

void WavAnalysisResultsPanel::clearAlerts()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

void WavAnalysisResultsPanel::renderAlerts(const SessionData& session, const WavAnalysisEngine::AnalysisResult& result)
{
    clearAlerts();

    if (!session.wavData.has_value()) {
        return;
    }

    if (result.processedSegment.empty()) {
        m_alertsListWidget->clear();
        m_alertsListWidget->addItem("Selected segment is empty");
        return;
    }

    m_alertsListWidget->clear();

    if (result.dominantPeaks.empty()) {
        m_alertsListWidget->addItem("No dominant spectral peaks detected");
        return;
    }

    m_alertsListWidget->addItem(
        QString("Detected peaks: %1 | showing top %2")
            .arg(static_cast<qulonglong>(result.allPeaks.size()))
            .arg(static_cast<qulonglong>(result.dominantPeaks.size()))
    );

    for (std::size_t i = 0; i < result.dominantPeaks.size(); ++i) {
        m_alertsListWidget->addItem(QString::fromStdString(pdt::format_peak_line(result.dominantPeaks[i], i + 1)));
    }
}

QString WavAnalysisResultsPanel::toString(WavAnalysisEngine::SpectrumAlgorithm algorithm) const
{
    using enum WavAnalysisEngine::SpectrumAlgorithm;
    switch (algorithm) {
    case Dft: return "DFT";
    case Fft: return "FFT";
    }
    return "-";
}

QString WavAnalysisResultsPanel::toString(pdt::WindowType window) const
{
    using enum pdt::WindowType;
    switch (window) {
    case Hann: return "Hann";
    case Hamming: return "Hamming";
    }
    return "-";
}

QString WavAnalysisResultsPanel::toString(pdt::PeakDetectionMode mode) const
{
    using enum pdt::PeakDetectionMode;
    switch (mode) {
    case ThresholdOnly: return "Threshold-Only";
    case LocalMaxima: return "Local-Maxima";
    }
    return "-";
}

} // namespace pdv