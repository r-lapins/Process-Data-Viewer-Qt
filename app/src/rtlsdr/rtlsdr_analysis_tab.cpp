#include "pdv/rtlsdr/rtlsdr_analysis_tab.h"

#include "pdv/rtlsdr/rtlsdr_analysis_controls_widget.h"
#include "pdv/rtlsdr/rtlsdr_analysis_results_panel.h"
#include "pdv/wav/wav_analysis_plot_widget.h"

#include "pdt/io/wav/wav_output.h"

#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <fstream>

namespace pdv {
namespace {

QString defaultExportPath(const QString& suffix)
{
    return QDir::home().filePath("rtlsdr" + suffix);
}

} // namespace

RtlSdrAnalysisTab::RtlSdrAnalysisTab(const SessionData& session, QWidget* parent)
    : AnalysisTab(session, parent)
{
    m_controller = new RtlSdrAnalysisController(this);

    createUi();
    connectControls();
    updatePlotVisibility();
}

RtlSdrAnalysisTab::~RtlSdrAnalysisTab()
{
    if (m_controller != nullptr) {
        m_controller->stop();
    }
}

void RtlSdrAnalysisTab::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 0);

    auto* topWidget = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    m_controlsWidget = new RtlSdrAnalysisControlsWidget(topWidget);
    m_controlsWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    m_resultsPanel = new RtlSdrAnalysisResultsPanel(this);
    m_resultsPanel->setFixedWidth(760);

    topLayout->addSpacing(20);
    topLayout->addWidget(m_controlsWidget, 0, Qt::AlignTop);
    topLayout->addWidget(m_resultsPanel, 0, Qt::AlignTop);
    topLayout->addStretch();

    topWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* plotGroup = new QGroupBox(this);
    auto* bottomLayout = new QVBoxLayout(plotGroup);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    m_signalChartWidget = new SignalChartWidget(plotGroup);
    m_spectrumChartWidget = new SpectrumChartWidget(plotGroup);
    m_signalPlotContainer = m_signalChartWidget;
    m_spectrumPlotContainer = m_spectrumChartWidget;

    m_signalPlotContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_spectrumPlotContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    bottomLayout->addWidget(m_signalPlotContainer);
    bottomLayout->addWidget(m_spectrumPlotContainer);
    bottomLayout->addStretch(0);

    rootLayout->addWidget(topWidget, 0);
    rootLayout->addWidget(plotGroup, 1);
    rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
}

void RtlSdrAnalysisTab::connectControls()
{
    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::startRequested,
            this, [this]() { m_controller->start(m_controlsWidget->settings()); });
    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::stopRequested,
            m_controller, &RtlSdrAnalysisController::stop);
    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::settingsChanged,
            this, [this]() { m_controller->updateSettings(m_controlsWidget->settings()); });

    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::signalPlotToggled,
            this, [this]() { updatePlotVisibility(); });
    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::spectrumPlotToggled,
            this, [this]() { updatePlotVisibility(); });

    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::exportSignalPlotRequested,
            this, &RtlSdrAnalysisTab::exportSignalPlotPng);
    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::exportSpectrumPlotRequested,
            this, &RtlSdrAnalysisTab::exportSpectrumPlotPng);
    connect(m_controlsWidget, &RtlSdrAnalysisControlsWidget::exportSpectrumCsvRequested,
            this, &RtlSdrAnalysisTab::exportSpectrumCsv);

    connect(m_controller, &RtlSdrAnalysisController::resultChanged,
            this, &RtlSdrAnalysisTab::renderAnalysis);

    connect(m_controller, &RtlSdrAnalysisController::runningChanged,
            this, [this](bool running) {
                m_controlsWidget->setRunning(running);
                m_resultsPanel->setStatusText(running ? "Running" : "Idle");
                emit analysisStatusChanged(running, running ? "RTL-SDR streaming..." : "Ready");
            });

    connect(m_controller, &RtlSdrAnalysisController::streamFailed,
            this, [this](const QString& message) {
                m_controlsWidget->setRunning(false);
                m_resultsPanel->setStatusText("Error");
                QMessageBox::critical(this, "RTL-SDR stream failed", message);
                emit analysisStatusChanged(false, "RTL-SDR stream failed");
            });
}

void RtlSdrAnalysisTab::updatePlotVisibility()
{
    const bool signalVisible = (m_controlsWidget != nullptr && m_controlsWidget->isSignalPlotEnabled());
    const bool spectrumVisible = (m_controlsWidget != nullptr && m_controlsWidget->isSpectrumPlotEnabled());

    m_signalPlotContainer->setVisible(signalVisible);
    m_spectrumPlotContainer->setVisible(spectrumVisible);
    emit preferredSizeChanged();
}

void RtlSdrAnalysisTab::renderAnalysis(const RtlSdrAnalysisResult& result)
{
    m_lastResult = result;
    m_resultsPanel->setResults(result);
    renderSignalPlot(result);
    renderSpectrumPlot(result);
}

void RtlSdrAnalysisTab::renderSignalPlot(const RtlSdrAnalysisResult& result)
{
    m_signalChartWidget->updateIqPlot(result.frame.samples, "IQ plot - RTL-SDR");
}

void RtlSdrAnalysisTab::renderSpectrumPlot(const RtlSdrAnalysisResult& result)
{
    m_spectrumChartWidget->updatePlot(result.analysis.spectrum, "Spectrum plot - RTL-SDR");
}

void RtlSdrAnalysisTab::exportSignalPlotPng()
{
    const QString filePath = QFileDialog::getSaveFileName(this, "Export IQ PNG", defaultExportPath("_iq.png"), "PNG files (*.png)");
    if (filePath.isEmpty()) { return; }

    const QPixmap pixmap = m_signalChartWidget->grab();
    if (!pixmap.save(filePath, "PNG")) {
        QMessageBox::critical(this, "Export IQ PNG", QString("Failed to save PNG file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export IQ PNG", QString("IQ plot exported to:\n%1").arg(filePath));
}

void RtlSdrAnalysisTab::exportSpectrumPlotPng()
{
    const QString filePath = QFileDialog::getSaveFileName(this, "Export spectrum PNG", defaultExportPath("_spectrum.png"), "PNG files (*.png)");
    if (filePath.isEmpty()) { return; }

    const QPixmap pixmap = m_spectrumChartWidget->grab();
    if (!pixmap.save(filePath, "PNG")) {
        QMessageBox::critical(this, "Export spectrum PNG", QString("Failed to save PNG file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export spectrum PNG", QString("Spectrum plot exported to:\n%1").arg(filePath));
}

void RtlSdrAnalysisTab::exportSpectrumCsv()
{
    if (!m_lastResult.has_value()) {
        QMessageBox::warning(this, "Export spectrum CSV", "No live spectrum is available yet.");
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, "Export spectrum CSV", defaultExportPath("_spectrum.csv"), "CSV files (*.csv)");
    if (filePath.isEmpty()) { return; }

    std::ofstream out(filePath.toStdString());
    if (!out.is_open() || !pdt::write_spectrum_csv(out, m_lastResult->analysis.spectrum)) {
        QMessageBox::critical(this, "Export spectrum CSV", QString("Failed to save CSV file:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export spectrum CSV", QString("Spectrum CSV exported to:\n%1").arg(filePath));
}

} // namespace pdv
