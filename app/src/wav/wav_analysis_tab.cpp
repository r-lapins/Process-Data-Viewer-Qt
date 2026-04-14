#include "pdv/wav/wav_analysis_tab.h"
#include "pdv/wav/wav_analysis_plot_widget.h"
#include "pdv/wav/wav_analysis_controller.h"
#include "pdv/wav/wav_analysis_controls_widget.h"
#include "pdv/wav/wav_analysis_results_panel.h"

#include "pdt/io/wav/wav_output.h"

#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>

#include <fstream>

namespace pdv {
namespace {

QString toDisplayString(pdt::SpectrumAlgorithm algorithm)
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

QString toDisplayString(pdt::WindowType window)
{
    using enum pdt::WindowType;

    switch (window) {
    case Hann:      return "Hann";
    case Hamming:   return "Hamming";
    case None:      return "None";
        break;
    }

    return "-";
}

QString toDisplayString(pdt::PeakDetectionMode mode)
{
    using enum pdt::PeakDetectionMode;

    switch (mode) {
    case ThresholdOnly: return "Threshold-Only";
    case LocalMaxima:   return "Local-Maxima";
    }

    return "-";
}

QString defaultExportPath(const QString& filePath, const QString& suffix)
{
    const QFileInfo sourceInfo(filePath);
    return sourceInfo.dir().filePath(sourceInfo.completeBaseName() + suffix);
}

pdt::SpectrumReport buildSpectrumReport(const SessionData& session, const pdt::WavAnalysisResult& result) {
    pdt::SpectrumReport report{};
    report.analysis = result.analysis;

    report.meta.input_path    = session.filePath.toStdString();
    report.meta.sample_rate   = session.wavData.has_value() ? static_cast<double>(session.wavData->sample_rate) : 0.0;
    report.meta.channels      = session.wavData.has_value() ? static_cast<std::size_t>(session.wavData->channels) : 0;
    report.meta.total_samples = session.wavData.has_value() ? session.wavData->samples.size() : 0;

    const auto& settings = result.used_settings;
    report.meta.from          = settings.from;
    report.meta.windowSize    = result.raw_segment.size();
    report.meta.window        = settings.window;
    report.meta.algorithm     = result.analysis.algorithm;
    report.meta.threshold     = settings.threshold;
    report.meta.peak_mode     = settings.peak_mode;
    report.meta.top           = settings.top_peaks;

    return report;
}

} // namespace

WavAnalysisTab::WavAnalysisTab(const SessionData& session, QWidget* parent) : AnalysisTab(session, parent)
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

    m_resultsPanel = new WavAnalysisResultsPanel(this);
    m_resultsPanel->setFixedWidth(725);

    topLayout->addSpacing(20);
    topLayout->addWidget(m_controlsWidget, 0, Qt::AlignTop);
    topLayout->addWidget(m_resultsPanel, 0, Qt::AlignTop);
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

void WavAnalysisTab::connectControls()
{
    connect(m_controlsWidget, &WavAnalysisControlsWidget::analysisRequested, this, &WavAnalysisTab::recomputeAnalysis);

    connect(m_controlsWidget, &WavAnalysisControlsWidget::signalPlotToggled, this, [this]() { updatePlotVisibility(); });
    connect(m_controlsWidget, &WavAnalysisControlsWidget::spectrumPlotToggled, this, [this]() { updatePlotVisibility(); });

    connect(m_controller, &WavAnalysisController::resultChanged, this, [this](const pdt::WavAnalysisResult& result) { renderAnalysis(result); });

    connect(m_controller, &WavAnalysisController::busyChanged,
            this, [this](bool busy) {
                m_controlsWidget->setBusy(busy);
                emit analysisStatusChanged(busy, busy ? "Analyzing WAV data..." : "Ready");
            });

    connect(m_controller, &WavAnalysisController::analysisFailed,
            this, [this](const QString& message) {
                QMessageBox::critical(this, "WAV analysis failed", message);
                emit analysisStatusChanged(false, "Analysis failed");
            });

    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSignalPlotRequested, this, &WavAnalysisTab::exportSignalPlotPng);
    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSpectrumPlotRequested, this, &WavAnalysisTab::exportSpectrumPlotPng);

    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSpectrumCsvRequested, this, &WavAnalysisTab::exportSpectrumCsv);
    connect(m_controlsWidget, &WavAnalysisControlsWidget::exportSpectrumReportRequested, this, &WavAnalysisTab::exportSpectrumReport);
}

void WavAnalysisTab::recomputeAnalysis()
{
    m_controller->setSettings(m_controlsWidget->settings());
    m_controller->recompute();
}

void WavAnalysisTab::renderAnalysis(const pdt::WavAnalysisResult& result)
{
    m_resultsPanel->setResults(m_session, result);
    renderSignalPlot(result);
    renderSpectrumPlot(result);
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

void WavAnalysisTab::renderSignalPlot(const pdt::WavAnalysisResult& result)
{
    const QString fromInfo = QString::number(static_cast<qulonglong>(result.used_settings.from));
    const QFileInfo fileInfo(m_session.filePath);

    m_signalChartWidget->updatePlot(result.raw_segment, fromInfo, QString("Signal plot - %1").arg(fileInfo.fileName()));
}

void WavAnalysisTab::renderSpectrumPlot(const pdt::WavAnalysisResult& result)
{
    const QFileInfo fileInfo(m_session.filePath);
    m_spectrumChartWidget->updatePlot(result.analysis.spectrum, QString("Spectrum plot - %1").arg(fileInfo.fileName()));
}

void WavAnalysisTab::exportSignalPlotPng()
{
    if (m_signalChartWidget == nullptr) {
        QMessageBox::warning(this, "Export signal PNG", "Signal plot is not available.");
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, "Export signal PNG", defaultExportPath(m_session.filePath, "_signal.png"), "PNG files (*.png)");

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

    const QString filePath = QFileDialog::getSaveFileName(this, "Export spectrum PNG", defaultExportPath(m_session.filePath, "_spectrum.png"), "PNG files (*.png)");

    if (filePath.isEmpty()) { return; }

    const QPixmap pixmap = m_spectrumChartWidget->grab();
    if (!pixmap.save(filePath, "PNG")) {
        QMessageBox::critical(this, "Export spectrum PNG", QString("Failed to save PNG file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export spectrum PNG", QString("Spectrum plot exported to:\n%1").arg(filePath));
}

void WavAnalysisTab::updatePlotVisibility()
{
    // Show only plots explicitly enabled by the user
    const bool signalVisible = (m_controlsWidget != nullptr && m_controlsWidget->isSignalPlotEnabled());
    const bool spectrumVisible = (m_controlsWidget != nullptr && m_controlsWidget->isSpectrumPlotEnabled());

    m_signalPlotContainer->setVisible(signalVisible);
    m_signalPlotContainer->updateGeometry();

    m_spectrumPlotContainer->setVisible(spectrumVisible);
    m_spectrumPlotContainer->updateGeometry();

    updateGeometry();
    emit preferredSizeChanged();
}

void WavAnalysisTab::exportSpectrumCsv()
{
    if (m_controller == nullptr || !m_controller->hasResult()) {
        QMessageBox::warning(this, "Export spectrum CSV", "No spectrum data available for export.");
        return;
    }

    const auto& result = m_controller->result();

    const QString filePath = QFileDialog::getSaveFileName(
        this, "Export spectrum CSV", defaultExportPath(m_session.filePath, "_spectrum.csv"), "CSV files (*.csv)");

    if (filePath.isEmpty()) { return; }

    std::ofstream out(filePath.toStdString());
    if (!out) {
        QMessageBox::critical(this, "Export spectrum CSV", QString("Failed to open output file:\n%1").arg(filePath));
        return;
    }

    if (!pdt::write_spectrum_csv(out, result.analysis.spectrum) || !out) {
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
    const auto report = pdv::buildSpectrumReport(m_session, result);

    const QString filePath = QFileDialog::getSaveFileName(
        this, "Export spectrum report", defaultExportPath(m_session.filePath, "_spectrum_report.txt"), "Text files (*.txt);;All files (*)");

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

} // namespace pdv
