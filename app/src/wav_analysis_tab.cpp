#include "pdv/wav_analysis_tab.h"
#include "pdv/wav_analysis_charts.h"

#include <algorithm>
#include <vector>
#include <limits>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QStackedWidget>

namespace pdv {

WavAnalysisTab::WavAnalysisTab(const SessionData& session, QWidget* parent)
    : AnalysisTab(session, parent)
{
    createUi();
    connectControls();
    recomputeAnalysis();
}

void WavAnalysisTab::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    // TOP / BOTTOM
    auto* verticalSplitter = new QSplitter(Qt::Vertical, this);

    // ===== TOP
    auto* topWidget = new QWidget(verticalSplitter);
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    topLayout->addStretch();
    topLayout->addWidget(createControlsPanel(topWidget));
    topLayout->addWidget(createStatisticsPanel(topWidget));
    topLayout->addWidget(createAlertsPanel(topWidget));
    topLayout->addStretch();

    // ===== BOTTOM
    auto* bottomWidget = new QWidget(verticalSplitter);
    auto* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    // == PLOTS-SPLITTER
    auto* plotsSplitter = new QSplitter(Qt::Vertical, bottomWidget);
    plotsSplitter->addWidget(createSignalPlot(plotsSplitter));
    plotsSplitter->addWidget(createSpectrumPlot(plotsSplitter));
    plotsSplitter->setStretchFactor(0, 1);
    plotsSplitter->setStretchFactor(1, 1);

    bottomLayout->addWidget(plotsSplitter);

    // ===== SPLITTER
    verticalSplitter->addWidget(topWidget);
    verticalSplitter->addWidget(bottomWidget);
    verticalSplitter->setStretchFactor(0, 2);
    verticalSplitter->setStretchFactor(1, 3);

    rootLayout->addWidget(verticalSplitter);
}

QWidget* WavAnalysisTab::createStatisticsPanel(QWidget* parent)
{
    auto* statsGroup = new QGroupBox("Statistics", parent);
    auto* statsLayout = new QFormLayout(statsGroup);

    m_statsFileTypeValueLabel = new QLabel("-", statsGroup);
    m_statsSampleRateValueLabel = new QLabel("-", statsGroup);
    m_statsChannelsValueLabel = new QLabel("-", statsGroup);
    m_statsTotalSamplesValueLabel = new QLabel("-", statsGroup);
    m_statsUsedFromValueLabel = new QLabel("-", statsGroup);
    m_statsUsedBinsValueLabel = new QLabel("-", statsGroup);
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
    statsLayout->addRow("Bins:", m_statsUsedBinsValueLabel);
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

    statsGroup->setFixedWidth(250);

    return statsGroup;
}

QWidget* WavAnalysisTab::createControlsPanel(QWidget* parent)
{
    auto* controlsGroup = new QGroupBox("Controls", parent);
    auto* controlsLayout = new QFormLayout(controlsGroup);

    m_fromSpinBox = new QSpinBox(controlsGroup);

    m_binsSpinBox = new QSpinBox(controlsGroup);
    m_binsComboBox = new QComboBox(controlsGroup);
    m_binsInputStack = new QStackedWidget(controlsGroup);

    m_windowComboBox = new QComboBox(controlsGroup);
    m_algorithmComboBox = new QComboBox(controlsGroup);
    m_thresholdSpinBox = new QDoubleSpinBox(controlsGroup);
    m_peakModeComboBox = new QComboBox(controlsGroup);
    m_topPeaksSpinBox = new QSpinBox(controlsGroup);
    m_autoUpdateCheckBox = new QCheckBox("Auto update", controlsGroup);
    m_recomputeButton = new QPushButton("Recompute", controlsGroup);

    int sampleCount = 0;
    if (m_session.wavData.has_value()) {
        sampleCount = static_cast<int>(m_session.wavData->samples.size());
    }

    m_fromSpinBox->setRange(0, std::max(0, sampleCount > 0 ? sampleCount - 1 : 0));
    m_fromSpinBox->setValue(0);

    m_binsSpinBox->setRange(1, std::max(1, sampleCount));
    m_binsSpinBox->setValue(std::min(1024, std::max(1, sampleCount)));

    m_binsInputStack->addWidget(m_binsSpinBox);
    m_binsInputStack->addWidget(m_binsComboBox);
    m_binsInputStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_windowComboBox->addItem("Hann", static_cast<int>(pdt::WindowType::Hann));
    m_windowComboBox->addItem("Hamming", static_cast<int>(pdt::WindowType::Hamming));
    m_windowComboBox->addItem("None", -1);

    m_algorithmComboBox->addItem("FFT", static_cast<int>(SpectrumAlgorithm::Fft));
    m_algorithmComboBox->addItem("DFT", static_cast<int>(SpectrumAlgorithm::Dft));

    m_thresholdSpinBox->setRange(0.0, 1.0);
    m_thresholdSpinBox->setSingleStep(0.05);
    m_thresholdSpinBox->setDecimals(2);
    m_thresholdSpinBox->setValue(0.20);

    m_peakModeComboBox->addItem("Local maxima", static_cast<int>(pdt::PeakDetectionMode::LocalMaxima));
    m_peakModeComboBox->addItem("Threshold only", static_cast<int>(pdt::PeakDetectionMode::ThresholdOnly));

    m_topPeaksSpinBox->setRange(1, 100);
    m_topPeaksSpinBox->setValue(20);

    m_autoUpdateCheckBox->setChecked(true);

    controlsLayout->addRow("Algorithm:", m_algorithmComboBox);
    controlsLayout->addRow("Window:", m_windowComboBox);
    controlsLayout->addRow("Peak mode:", m_peakModeComboBox);
    controlsLayout->addRow("Threshold:", m_thresholdSpinBox);
    controlsLayout->addRow("Top peaks:", m_topPeaksSpinBox);
    controlsLayout->addRow("Bins:", m_binsInputStack);
    controlsLayout->addRow("From sample:", m_fromSpinBox);
    controlsLayout->addRow("", m_recomputeButton);
    controlsLayout->addRow("", m_autoUpdateCheckBox);

    m_recomputeButton->setEnabled(!m_autoUpdateCheckBox->isChecked());

    rebuildFftBinsCombo();
    updateBinsInputMode();
    updateFromSpinRange();

    controlsGroup->setFixedWidth(300);

    return controlsGroup;
}

QWidget* WavAnalysisTab::createAlertsPanel(QWidget* parent)
{
    auto* alertsGroup = new QGroupBox("Alerts", parent);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    alertsGroup->setFixedWidth(350);

    return alertsGroup;
}

void WavAnalysisTab::connectControls()
{
    connect(m_autoUpdateCheckBox, &QCheckBox::toggled, this, [this](bool checked) {m_recomputeButton->setEnabled(!checked);});

    connect(m_recomputeButton, &QPushButton::clicked, this, &WavAnalysisTab::recomputeAnalysis);

    connect(m_algorithmComboBox, &QComboBox::currentIndexChanged, this, &WavAnalysisTab::triggerAutoRecompute);
    connect(m_peakModeComboBox, &QComboBox::currentIndexChanged, this, &WavAnalysisTab::triggerAutoRecompute);
    connect(m_windowComboBox, &QComboBox::currentIndexChanged, this, &WavAnalysisTab::triggerAutoRecompute);
    connect(m_thresholdSpinBox, &QDoubleSpinBox::valueChanged, this, &WavAnalysisTab::triggerAutoRecompute);
    connect(m_binsComboBox, &QComboBox::currentIndexChanged, this, &WavAnalysisTab::triggerAutoRecompute);
    connect(m_topPeaksSpinBox, &QSpinBox::valueChanged, this, &WavAnalysisTab::triggerAutoRecompute);
    connect(m_fromSpinBox, &QSpinBox::valueChanged, this, &WavAnalysisTab::triggerAutoRecompute);
    connect(m_binsSpinBox, &QSpinBox::valueChanged, this, &WavAnalysisTab::triggerAutoRecompute);
}

void WavAnalysisTab::recomputeAnalysis()
{
    WavAnalysisEngine::AnalysisResult result{};

    if (m_session.wavData.has_value()) {
        result = WavAnalysisEngine::analyze(*m_session.wavData, currentSettings());
    }

    updateStatisticsPanel(result);
    updateAlertsPanel(result);
    updateSignalPlot(result);
    updateSpectrumPlot(result);
    updateFromSpinStep();
    updateBinsInputMode();
    updateFromSpinRange();
}

void WavAnalysisTab::triggerAutoRecompute()
{
    if (m_autoUpdateCheckBox->isChecked()) {
        recomputeAnalysis();
    }
}

std::size_t WavAnalysisTab::selectedBins() const
{
    const auto selected =
        static_cast<SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());

    if (selected == SpectrumAlgorithm::Fft) {
        return static_cast<std::size_t>(m_binsComboBox->currentData().toULongLong());
    }

    return static_cast<std::size_t>(m_binsSpinBox->value());
}

void WavAnalysisTab::updateBinsInputMode()
{
    const auto selected = static_cast<SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());

    if (selected == SpectrumAlgorithm::Fft) {
        m_binsInputStack->setCurrentWidget(m_binsComboBox);
    } else {
        m_binsInputStack->setCurrentWidget(m_binsSpinBox);
    }
}

void WavAnalysisTab::rebuildFftBinsCombo()
{
    const qulonglong previousValue =
        m_binsComboBox->currentData().toULongLong();

    m_binsComboBox->clear();

    if (!m_session.wavData.has_value()) {
        return;
    }

    const auto& samples = m_session.wavData->samples;
    if (samples.empty()) {
        return;
    }

    const std::size_t availableFromZero = samples.size();

    std::vector<std::size_t> values;
    for (std::size_t value = 1; value <= availableFromZero; value *= 2) {
        values.push_back(value);
        if (value > (std::numeric_limits<std::size_t>::max() / 2)) {
            break;
        }
    }

    for (std::size_t value : values) {
        m_binsComboBox->addItem(
            QString::number(static_cast<qulonglong>(value)),
            static_cast<qulonglong>(value)
            );
    }

    if (m_binsComboBox->count() == 0) {
        return;
    }

    qulonglong targetValue = previousValue;

    if (targetValue == 0) {
        targetValue = m_binsSpinBox->value() > 0
                          ? static_cast<qulonglong>(m_binsSpinBox->value())
                          : m_binsComboBox->itemData(m_binsComboBox->count() - 1).toULongLong();
    }

    int bestIndex = 0;
    for (int i = 0; i < m_binsComboBox->count(); ++i) {
        const qulonglong value = m_binsComboBox->itemData(i).toULongLong();
        if (value <= targetValue) {
            bestIndex = i;
        } else {
            break;
        }
    }

    m_binsComboBox->setCurrentIndex(bestIndex);
}

void WavAnalysisTab::updateFromSpinRange()
{
    if (!m_session.wavData.has_value()) {
        m_fromSpinBox->setRange(0, 0);
        return;
    }

    const auto& samples = m_session.wavData->samples;
    if (samples.empty()) {
        m_fromSpinBox->setRange(0, 0);
        return;
    }

    const std::size_t bins = selectedBins();
    const std::size_t total = samples.size();

    const int maxFrom = (bins >= total) ? 0 : static_cast<int>(total - bins);

    m_fromSpinBox->setRange(0, maxFrom);

    if (m_fromSpinBox->value() > maxFrom) {
        m_fromSpinBox->setValue(maxFrom);
    }
}

void WavAnalysisTab::resetStatisticsPanel()
{
    m_statsFileTypeValueLabel->setText("-");
    m_statsSampleRateValueLabel->setText("-");
    m_statsChannelsValueLabel->setText("-");
    m_statsTotalSamplesValueLabel->setText("-");
    m_statsUsedFromValueLabel->setText("-");
    m_statsUsedBinsValueLabel->setText("-");
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

void WavAnalysisTab::updateStatisticsPanel(const AnalysisResult& result)
{
    resetStatisticsPanel();

    if (!m_session.wavData.has_value()) {
        return;
    }

    const auto& wav = *m_session.wavData;
    const auto settings = currentSettings();

    m_statsFileTypeValueLabel->setText("WAV");
    m_statsSampleRateValueLabel->setText(QString::number(wav.sample_rate));
    m_statsChannelsValueLabel->setText(QString::number(wav.channels));
    m_statsTotalSamplesValueLabel->setText(QString::number(static_cast<qulonglong>(wav.samples.size())));
    m_statsUsedFromValueLabel->setText(QString::number(static_cast<qulonglong>(settings.from)));
    m_statsUsedBinsValueLabel->setText(QString::number(static_cast<qulonglong>(result.rawSegment.size())));
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

void WavAnalysisTab::resetAlertsPanel()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

AnalysisSettings WavAnalysisTab::currentSettings() const
{
    AnalysisSettings s{};

    s.algorithm = selectedAlgorithm();
    s.useWindow = useWindow();

    if (s.useWindow) {
        s.window = static_cast<pdt::WindowType>(m_windowComboBox->currentData().toInt());
    }

    s.peakMode = static_cast<pdt::PeakDetectionMode>(m_peakModeComboBox->currentData().toInt());
    s.threshold = m_thresholdSpinBox->value();
    s.topPeaks = static_cast<std::size_t>(m_topPeaksSpinBox->value());
    s.from = static_cast<std::size_t>(m_fromSpinBox->value());
    s.bins = selectedBins();

    return s;
}

void WavAnalysisTab::updateAlertsPanel(const AnalysisResult& result)
{
    resetAlertsPanel();

    if (!m_session.wavData.has_value()) {
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

    for (const auto& peak : result.dominantPeaks) {
        m_alertsListWidget->addItem(
            QString("bin=%1 | freq=%2 Hz | mag=%3")
                .arg(static_cast<qulonglong>(peak.index))
                .arg(peak.frequency, 0, 'f', 1)
                .arg(peak.magnitude, 0, 'f', 2)
            );
    }
}

SpectrumAlgorithm WavAnalysisTab::selectedAlgorithm() const
{
    return static_cast<SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());
}

QString WavAnalysisTab::toString(SpectrumAlgorithm algorithm) const
{
    switch (algorithm) {
    case SpectrumAlgorithm::Dft: return "DFT";
    case SpectrumAlgorithm::Fft: return "FFT";
    }
    return "-";
}

QString WavAnalysisTab::toString(pdt::WindowType window) const
{
    switch (window) {
    case pdt::WindowType::Hann: return "Hann";
    case pdt::WindowType::Hamming: return "Hamming";
    }
    return "-";
}

QString WavAnalysisTab::toString(pdt::PeakDetectionMode mode) const
{
    switch (mode) {
    case pdt::PeakDetectionMode::ThresholdOnly: return "Threshold-Only";
    case pdt::PeakDetectionMode::LocalMaxima: return "Local-Maxima";
    }
    return "-";
}

bool WavAnalysisTab::useWindow() const
{
    return m_windowComboBox->currentData().toInt() != -1;
}

void WavAnalysisTab::updateFromSpinStep()
{
    const std::size_t bins = selectedBins();
    const int step = std::max<int>(1, static_cast<int>(bins / 10));
    m_fromSpinBox->setSingleStep(step);
}

QWidget* WavAnalysisTab::createSignalPlot(QWidget* parent)
{
    m_signalChartWidget = new SignalChartWidget(parent);
    return m_signalChartWidget;
}

QWidget* WavAnalysisTab::createSpectrumPlot(QWidget* parent)
{
    m_spectrumChartWidget = new SpectrumChartWidget(parent);
    return m_spectrumChartWidget;
}

void WavAnalysisTab::updateSignalPlot(const AnalysisResult& result)
{
    if (m_signalChartWidget == nullptr) {
        return;
    }

    const QFileInfo fileInfo(m_session.filePath);
    m_signalChartWidget->updatePlot(
        result.rawSegment,
        QString("Signal plot - %1").arg(fileInfo.fileName())
        );
}

void WavAnalysisTab::updateSpectrumPlot(const AnalysisResult& result)
{
    if (m_spectrumChartWidget == nullptr) {
        return;
    }

    const QFileInfo fileInfo(m_session.filePath);
    m_spectrumChartWidget->updatePlot(
        result.spectrum.frequencies,
        result.spectrum.magnitudes,
        QString("Spectrum plot - %1").arg(fileInfo.fileName())
        );
}

} // namespace pdv
