#include "pdv/rtlsdr/rtlsdr_analysis_results_panel.h"

#include "pdt/io/wav/wav_output.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace pdv {

RtlSdrAnalysisResultsPanel::RtlSdrAnalysisResultsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(10);

    rootLayout->addWidget(createStatisticsPanel(this), 0, Qt::AlignTop);
    rootLayout->addWidget(createPeaksPanel(this), 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    clear();
}

void RtlSdrAnalysisResultsPanel::clear()
{
    clearStatistics();
    m_peaksListWidget->clear();
    m_peaksListWidget->addItem("No live spectrum yet");
}

void RtlSdrAnalysisResultsPanel::setResults(const RtlSdrAnalysisResult& result)
{
    renderStatistics(result);
    renderPeaks(result);
}

void RtlSdrAnalysisResultsPanel::setStatusText(const QString& text)
{
    m_statusValueLabel->setText(text);
}

QWidget* RtlSdrAnalysisResultsPanel::createStatisticsPanel(QWidget* parent)
{
    auto* statsGroup = new QGroupBox("Statistics", parent);
    auto* statsLayout = new QFormLayout(statsGroup);

    m_statusValueLabel = new QLabel("-", statsGroup);
    m_deviceValueLabel = new QLabel("-", statsGroup);
    m_centerFrequencyValueLabel = new QLabel("-", statsGroup);
    m_sampleRateValueLabel = new QLabel("-", statsGroup);
    m_gainValueLabel = new QLabel("-", statsGroup);
    m_biasTeeValueLabel = new QLabel("-", statsGroup);
    m_frameValueLabel = new QLabel("-", statsGroup);
    m_samplesValueLabel = new QLabel("-", statsGroup);
    m_algorithmValueLabel = new QLabel("-", statsGroup);
    m_windowValueLabel = new QLabel("-", statsGroup);
    m_peakModeValueLabel = new QLabel("-", statsGroup);
    m_detectedPeaksValueLabel = new QLabel("-", statsGroup);
    m_thresholdValueLabel = new QLabel("-", statsGroup);
    m_totalTimeValueLabel = new QLabel("-", statsGroup);

    statsLayout->addRow("Status:", m_statusValueLabel);
    statsLayout->addRow("Device:", m_deviceValueLabel);
    statsLayout->addRow("Frequency:", m_centerFrequencyValueLabel);
    statsLayout->addRow("Sample rate:", m_sampleRateValueLabel);
    statsLayout->addRow("Gain:", m_gainValueLabel);
    statsLayout->addRow("Bias tee:", m_biasTeeValueLabel);
    statsLayout->addRow("Frame:", m_frameValueLabel);
    statsLayout->addRow("IQ samples:", m_samplesValueLabel);
    statsLayout->addRow("Algorithm:", m_algorithmValueLabel);
    statsLayout->addRow("Window:", m_windowValueLabel);
    statsLayout->addRow("Peak mode:", m_peakModeValueLabel);
    statsLayout->addRow("Detected peaks:", m_detectedPeaksValueLabel);
    statsLayout->addRow("Threshold:", m_thresholdValueLabel);
    statsLayout->addRow("Total time:", m_totalTimeValueLabel);

    statsGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    statsGroup->setMinimumWidth(275);
    return statsGroup;
}

QWidget* RtlSdrAnalysisResultsPanel::createPeaksPanel(QWidget* parent)
{
    auto* peaksGroup = new QGroupBox("Peaks", parent);
    auto* peaksLayout = new QVBoxLayout(peaksGroup);

    m_peaksListWidget = new QListWidget(peaksGroup);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    m_peaksListWidget->setFont(font);

    peaksLayout->addWidget(m_peaksListWidget);
    peaksGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return peaksGroup;
}

void RtlSdrAnalysisResultsPanel::clearStatistics()
{
    m_statusValueLabel->setText("Idle");
    m_deviceValueLabel->setText("-");
    m_centerFrequencyValueLabel->setText("-");
    m_sampleRateValueLabel->setText("-");
    m_gainValueLabel->setText("-");
    m_biasTeeValueLabel->setText("-");
    m_frameValueLabel->setText("-");
    m_samplesValueLabel->setText("-");
    m_algorithmValueLabel->setText("-");
    m_windowValueLabel->setText("-");
    m_peakModeValueLabel->setText("-");
    m_detectedPeaksValueLabel->setText("-");
    m_thresholdValueLabel->setText("-");
    m_totalTimeValueLabel->setText("-");
}

void RtlSdrAnalysisResultsPanel::renderStatistics(const RtlSdrAnalysisResult& result)
{
    const auto& settings = result.settings;

    m_statusValueLabel->setText("Running");
    m_deviceValueLabel->setText(QString::number(settings.device.device_index));
    m_centerFrequencyValueLabel->setText(QString("%1 Hz").arg(settings.device.center_frequency));
    m_sampleRateValueLabel->setText(QString("%1 Hz").arg(settings.device.sample_rate));
    m_gainValueLabel->setText(settings.device.tuner_gain_tenth_db == 0
                                  ? "Auto"
                                  : QString("%1 tenth dB").arg(settings.device.tuner_gain_tenth_db));
    m_biasTeeValueLabel->setText(settings.device.bias_tee ? "On" : "Off");
    m_frameValueLabel->setText(QString::number(static_cast<qulonglong>(result.frame.sequence)));
    m_samplesValueLabel->setText(QString::number(static_cast<qulonglong>(result.frame.samples.size())));
    m_algorithmValueLabel->setText(toString(result.analysis.algorithm));
    m_windowValueLabel->setText(toString(settings.window));
    m_peakModeValueLabel->setText(toString(settings.peakMode));
    m_detectedPeaksValueLabel->setText(QString::number(static_cast<qulonglong>(result.analysis.all_peaks.size())));
    m_thresholdValueLabel->setText(QString::number(settings.threshold, 'g', 10));
    m_totalTimeValueLabel->setText(QString::number(result.analysis.total_time_ms, 'f', 1) + " ms");
}

void RtlSdrAnalysisResultsPanel::renderPeaks(const RtlSdrAnalysisResult& result)
{
    m_peaksListWidget->clear();

    if (result.analysis.top_peaks.empty()) {
        m_peaksListWidget->addItem("No dominant spectral peaks detected");
        return;
    }

    m_peaksListWidget->addItem(
        QString("Detected peaks: %1 | showing top %2")
            .arg(static_cast<qulonglong>(result.analysis.all_peaks.size()))
            .arg(static_cast<qulonglong>(result.analysis.top_peaks.size())));

    for (std::size_t i = 0; i < result.analysis.top_peaks.size(); ++i) {
        m_peaksListWidget->addItem(QString::fromStdString(pdt::format_peak_line(result.analysis.top_peaks[i], i + 1)));
    }
}

QString RtlSdrAnalysisResultsPanel::toString(pdt::SpectrumAlgorithm algorithm) const
{
    using enum pdt::SpectrumAlgorithm;
    switch (algorithm) {
    case Dft:   return "DFT";
    case Fft:   return "FFT";
    case Auto:  return "Auto";
    case cuFft: return "cuFFT";
    }
    return "-";
}

QString RtlSdrAnalysisResultsPanel::toString(pdt::WindowType window) const
{
    using enum pdt::WindowType;
    switch (window) {
    case Hann:    return "Hann";
    case Hamming: return "Hamming";
    case None:    return "None";
    }
    return "-";
}

QString RtlSdrAnalysisResultsPanel::toString(pdt::PeakDetectionMode mode) const
{
    using enum pdt::PeakDetectionMode;
    switch (mode) {
    case ThresholdOnly: return "Threshold-Only";
    case LocalMaxima:   return "Local-Maxima";
    }
    return "-";
}

} // namespace pdv
