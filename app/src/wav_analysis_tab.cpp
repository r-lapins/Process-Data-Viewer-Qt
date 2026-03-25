#include "pdv/wav_analysis_tab.h"
#include "pdv/wav_analysis_plot_widget.h"
#include "pdv/wav_analysis_controller.h"
#include "pdv/wav_analysis_controls_widget.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>

#include <fstream>

namespace pdv {

WavAnalysisTab::WavAnalysisTab(const SessionData& session, QWidget* parent)
    : AnalysisTab(session, parent)
{
    m_controller = new WavAnalysisController(m_session, this);

    createUi();
    connectControls();
    updatePlotVisibility();
    recomputeAnalysis();
}

void WavAnalysisTab::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 0);

    // ===== TOP
    auto* topWidget = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    m_controlsWidget = new WavAnalysisControlsWidget(m_session, topWidget);
    m_controlsWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    auto* resultsWidget = new QWidget(this);
    auto* resultsLayout = new QHBoxLayout(resultsWidget);
    resultsLayout->setContentsMargins(0, 0, 0, 0);

    auto* statsPanel = createStatisticsPanel(resultsWidget);
    auto* alertsPanel = createAlertsPanel(resultsWidget);

    resultsLayout->addWidget(statsPanel, 0, Qt::AlignTop);
    resultsLayout->addWidget(alertsPanel, 1);

    resultsWidget->setFixedWidth(600);

    topLayout->addSpacing(20);
    topLayout->addWidget(m_controlsWidget, 0, Qt::AlignTop);
    topLayout->addWidget(resultsWidget, 0, Qt::AlignTop);
    topLayout->addStretch();

    topWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // ===== BOTTOM
    auto* plotGroup = new QGroupBox(this);
    auto* bottomLayout = new QVBoxLayout(plotGroup);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    m_signalPlotContainer = createSignalPlot(plotGroup);
    m_spectrumPlotContainer = createSpectrumPlot(plotGroup);

    m_signalPlotContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_spectrumPlotContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_signalPlotContainer->setVisible(false);
    m_spectrumPlotContainer->setVisible(false);

    bottomLayout->addWidget(m_signalPlotContainer);
    bottomLayout->addWidget(m_spectrumPlotContainer);
    bottomLayout->addStretch(0);

    rootLayout->addWidget(topWidget, 0);
    rootLayout->addWidget(plotGroup, 1);
    rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
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

QWidget* WavAnalysisTab::createAlertsPanel(QWidget* parent)
{
    auto* alertsGroup = new QGroupBox("Alerts", parent);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    alertsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return alertsGroup;
}

void WavAnalysisTab::connectControls()
{
    if (m_controlsWidget == nullptr) {
        return;
    }

    connect(m_controlsWidget, &WavAnalysisControlsWidget::analysisRequested, this, &WavAnalysisTab::recomputeAnalysis);

    connect(m_controlsWidget, &WavAnalysisControlsWidget::signalPlotToggled, this, [this]() { updatePlotVisibility(); });
    connect(m_controlsWidget, &WavAnalysisControlsWidget::spectrumPlotToggled, this, [this]() { updatePlotVisibility(); });

    connect(m_controller, &WavAnalysisController::resultChanged, this, [this](const WavAnalysisEngine::AnalysisResult& result) { renderAnalysis(result); });

    connect(m_controller, &WavAnalysisController::busyChanged,
            this, [this](bool busy) {
                m_controlsWidget->setBusy(busy);
                emit analysisStatusChanged(busy, busy ? "Analyzing WAV data..." : "Ready");
            });

    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSignalPlotRequested, this, &WavAnalysisTab::exportSignalPlotPng);
    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSpectrumPlotRequested, this, &WavAnalysisTab::exportSpectrumPlotPng);

    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSpectrumCsvRequested, this, &WavAnalysisTab::exportSpectrumCsv);
    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSpectrumReportRequested, this, &WavAnalysisTab::exportSpectrumReport);
}

void WavAnalysisTab::recomputeAnalysis()
{
    if (m_controller == nullptr || m_controlsWidget == nullptr) {
        return;
    }

    m_controller->setSettings(m_controlsWidget->settings());
    m_controller->recompute();
}

void WavAnalysisTab::clearStatistics()
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

void WavAnalysisTab::renderAnalysis(const WavAnalysisEngine::AnalysisResult& result)
{
    renderStatistics(result);
    renderAlerts(result);
    renderSignalPlot(result);
    renderSpectrumPlot(result);
}

void WavAnalysisTab::renderStatistics(const WavAnalysisEngine::AnalysisResult& result)
{
    clearStatistics();

    if (!m_session.wavData.has_value() || m_controller == nullptr) {
        return;
    }

    const auto& wav = *m_session.wavData;
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

void WavAnalysisTab::clearAlerts()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

void WavAnalysisTab::renderAlerts(const WavAnalysisEngine::AnalysisResult& result)
{
    clearAlerts();

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
            QString("f = %1 Hz | |X| = %2 | bin = %3")
                .arg(peak.frequency, 0, 'f', 1)
                .arg(peak.magnitude, 0, 'f', 2)
                .arg(static_cast<qulonglong>(peak.index))
            );
    }
}

QString WavAnalysisTab::toString(WavAnalysisEngine::SpectrumAlgorithm algorithm) const
{
    using enum WavAnalysisEngine::SpectrumAlgorithm;
    switch (algorithm) {
    case Dft: return "DFT";
    case Fft: return "FFT";
    }
    return "-";
}

QString WavAnalysisTab::toString(pdt::WindowType window) const
{
    using enum pdt::WindowType;
    switch (window) {
    case Hann: return "Hann";
    case Hamming: return "Hamming";
    }
    return "-";
}

QString WavAnalysisTab::toString(pdt::PeakDetectionMode mode) const
{
    using enum pdt::PeakDetectionMode;
    switch (mode) {
    case ThresholdOnly: return "Threshold-Only";
    case LocalMaxima: return "Local-Maxima";
    }
    return "-";
}

void WavAnalysisTab::updatePlotVisibility()
{
    // Show only plots explicitly enabled by the user
    const bool signalVisible = (m_controlsWidget != nullptr && m_controlsWidget->isSignalPlotEnabled());
    const bool spectrumVisible = (m_controlsWidget != nullptr && m_controlsWidget->isSpectrumPlotEnabled());

    if (m_signalPlotContainer != nullptr) {
        m_signalPlotContainer->setVisible(signalVisible);
        m_signalPlotContainer->updateGeometry();
    }

    if (m_spectrumPlotContainer != nullptr) {
        m_spectrumPlotContainer->setVisible(spectrumVisible);
        m_spectrumPlotContainer->updateGeometry();
    }

    updateGeometry();
    emit preferredSizeChanged();
}

void WavAnalysisTab::exportSignalPlotPng()
{
    if (m_signalChartWidget == nullptr) {
        QMessageBox::warning(this, "Export signal PNG", "Signal plot is not available.");
        return;
    }

    const QFileInfo sourceInfo(m_session.filePath);
    const QString defaultName = sourceInfo.dir().filePath(sourceInfo.completeBaseName() + "_signal.png");

    const QString filePath = QFileDialog::getSaveFileName(this, "Export signal PNG", defaultName, "PNG files (*.png)");

    if (filePath.isEmpty()) { return; }

    const QPixmap pixmap = m_signalChartWidget->grab();
    if (!pixmap.save(filePath, "PNG")) {
        QMessageBox::critical(this, "Export signal PNG", QString("Failed to save PNG file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export signal PNG", QString("Signal plot exported to:\n%1").arg(filePath));
}

void WavAnalysisTab::exportSpectrumPlotPng()
{
    if (m_spectrumChartWidget == nullptr) {
        QMessageBox::warning(this, "Export spectrum PNG", "Spectrum plot is not available.");
        return;
    }

    const QFileInfo sourceInfo(m_session.filePath);
    const QString defaultName = sourceInfo.dir().filePath(sourceInfo.completeBaseName() + "_spectrum.png");

    const QString filePath = QFileDialog::getSaveFileName(this, "Export spectrum PNG", defaultName, "PNG files (*.png)");

    if (filePath.isEmpty()) { return; }

    const QPixmap pixmap = m_spectrumChartWidget->grab();
    if (!pixmap.save(filePath, "PNG")) {
        QMessageBox::critical(this, "Export spectrum PNG", QString("Failed to save PNG file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export spectrum PNG", QString("Spectrum plot exported to:\n%1").arg(filePath));
}

void WavAnalysisTab::exportSpectrumCsv()
{
    if (m_controller == nullptr || !m_controller->hasResult()) {
        QMessageBox::warning(this, "Export spectrum CSV", "No spectrum data available for export.");
        return;
    }

    const auto& result = m_controller->result();

    const QString filePath = QFileDialog::getSaveFileName(
        this, "Export spectrum CSV", defaultExportPath("_spectrum.csv"), "CSV files (*.csv)");

    if (filePath.isEmpty()) { return; }

    std::ofstream out(filePath.toStdString());
    if (!out) {
        QMessageBox::critical(this, "Export spectrum CSV", QString("Failed to open output file:\n%1").arg(filePath));
        return;
    }

    if (!pdt::write_spectrum_csv(out, result.spectrum) || !out) {
        QMessageBox::critical(this, "Export spectrum CSV", QString("Failed to write CSV file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export spectrum CSV", QString("Spectrum exported to:\n%1").arg(filePath));
}

void WavAnalysisTab::exportSpectrumReport()
{
    if (m_controller == nullptr || !m_controller->hasResult()) {
        QMessageBox::warning(this, "Export spectrum report", "No analysis result available for export.");
        return;
    }

    const auto& result = m_controller->result();
    const auto report = buildSpectrumReport(result);

    const QString filePath = QFileDialog::getSaveFileName(
        this, "Export spectrum report", defaultExportPath("_spectrum_report.txt"), "Text files (*.txt);;All files (*)");

    if (filePath.isEmpty()) { return; }

    std::ofstream out(filePath.toStdString());
    if (!out) {
        QMessageBox::critical(this, "Export spectrum report", QString("Failed to open output file:\n%1").arg(filePath));
        return;
    }

    if (!pdt::write_spectrum_report(out, report) || !out) {
        QMessageBox::critical(this, "Export spectrum report", QString("Failed to write report file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export spectrum report", QString("Report exported to:\n%1").arg(filePath));
}

pdt::SpectrumReport WavAnalysisTab::buildSpectrumReport(
    const WavAnalysisEngine::AnalysisResult& result) const
{
    pdt::SpectrumReport report{};
    report.spectrum = result.spectrum;
    report.all_peaks = result.allPeaks;
    report.top_peaks = result.dominantPeaks;

    report.meta.input_path = m_session.filePath.toStdString();
    report.meta.sample_rate = m_session.wavData.has_value() ? static_cast<double>(m_session.wavData->sample_rate) : 0.0;
    report.meta.channels = m_session.wavData.has_value() ? static_cast<std::size_t>(m_session.wavData->channels) : 0;
    report.meta.total_samples = m_session.wavData.has_value() ? m_session.wavData->samples.size() : 0;

    const auto& settings = result.usedSettings;
    report.meta.from = settings.from;
    report.meta.windowSize = result.rawSegment.size();
    report.meta.window = settings.useWindow ? toString(settings.window).toStdString() : "none";
    report.meta.algorithm = toString(settings.algorithm).toStdString();
    report.meta.threshold = settings.threshold;
    report.meta.peak_mode = toString(settings.peakMode).toStdString();
    report.meta.top = settings.topPeaks;

    return report;
}

QString WavAnalysisTab::defaultExportPath(const QString& suffix) const
{
    const QFileInfo sourceInfo(m_session.filePath);
    return sourceInfo.dir().filePath(sourceInfo.completeBaseName() + suffix);
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

void WavAnalysisTab::renderSignalPlot(const WavAnalysisEngine::AnalysisResult& result)
{
    if (m_signalChartWidget == nullptr) {
        return;
    }

    const QFileInfo fileInfo(m_session.filePath);
    m_signalChartWidget->updatePlot(
        result.rawSegment,
        m_statsUsedFromValueLabel->text(),
        QString("Signal plot - %1").arg(fileInfo.fileName())
        );
}

void WavAnalysisTab::renderSpectrumPlot(const WavAnalysisEngine::AnalysisResult& result)
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
