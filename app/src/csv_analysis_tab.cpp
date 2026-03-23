#include "pdv/csv_analysis_tab.h"
#include "pdv/csv_plot_widget.h"
#include "pdv/csv_results_panel.h"

#include <pdt/core/report.h>

#include <fstream>
#include <numeric>

#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFormLayout>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

namespace pdv {
namespace {

pdt::AnomalyMethod toPdtAnomalyMethod(CsvAnalysisEngine::AnomalyMethod method)
{
    using GUI = CsvAnalysisEngine::AnomalyMethod;
    using PDT = pdt::AnomalyMethod;

    switch (method) {
    case GUI::ZScore:   return PDT::ZScore;
    case GUI::IQR:      return PDT::IQR;
    case GUI::MAD:      return PDT::MAD;
    }

    return PDT::ZScore;
}

} // namespace

CsvAnalysisTab::CsvAnalysisTab(const SessionData& session, QWidget* parent)
    : AnalysisTab(session, parent)
{
    m_csvSamplesModel = new CsvSamplesTableModel(this);

    createUi();
    connectControls();

    recomputeAnalysis();
    updatePlotVisibility();
}

void CsvAnalysisTab::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);
    rootLayout->setSpacing(10);

    auto* topWidget = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    auto* dataPanel = createDataPanel(topWidget);

    m_controlsWidget = new CsvAnalysisControlsWidget(m_session, topWidget);
    m_controlsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    dataPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dataPanel->setMinimumWidth(400);

    m_resultsPanel = new CsvResultsPanel(topWidget);
    m_resultsPanel->setFixedWidth(700);

    auto* leftColumnWidget = new QWidget(topWidget);
    auto* leftColumnLayout = new QVBoxLayout(leftColumnWidget);
    leftColumnLayout->setContentsMargins(0, 0, 0, 0);

    leftColumnLayout->addWidget(m_controlsWidget, 0, Qt::AlignTop);
    leftColumnLayout->addStretch();

    topLayout->addWidget(leftColumnWidget, 0);
    topLayout->addWidget(m_resultsPanel, 0);
    topLayout->addWidget(dataPanel, 1);

    m_plotContainer = createPlotPanel(this);
    m_plotContainer->setVisible(false);

    rootLayout->addWidget(topWidget, 1);
    rootLayout->addWidget(m_plotContainer, 0);

    rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
}

QWidget* CsvAnalysisTab::createDataPanel(QWidget* parent)
{
    auto* dataGroup = new QGroupBox("Data", parent);
    auto* dataLayout = new QVBoxLayout(dataGroup);

    m_data.placeholderLabel = new QLabel("No CSV data", dataGroup);
    m_data.placeholderLabel->setWordWrap(true);

    m_data.tableView = new QTableView(dataGroup);
    m_data.tableView->setModel(m_csvSamplesModel);
    m_data.tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_data.tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_data.tableView->setAlternatingRowColors(true);
    m_data.tableView->setSortingEnabled(false);
    m_data.tableView->horizontalHeader()->setStretchLastSection(true);
    m_data.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    dataLayout->addWidget(m_data.placeholderLabel);
    dataLayout->addWidget(m_data.tableView);

    m_data.tableView->hide();
    m_data.tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return dataGroup;
}

QGroupBox* CsvAnalysisTab::createPlotPanel(QWidget* parent)
{
    auto* plotGroup = new QGroupBox(parent);
    auto* plotLayout = new QVBoxLayout(plotGroup);

    m_csvPlotWidget = new CsvPlotWidget(plotGroup);
    plotLayout->addWidget(m_csvPlotWidget);

    plotGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    return plotGroup;
}

void CsvAnalysisTab::connectControls()
{
    connect(m_controlsWidget, &CsvAnalysisControlsWidget::analysisRequested, this, &CsvAnalysisTab::recomputeAnalysis);
    connect(m_controlsWidget, &CsvAnalysisControlsWidget::exportJsonRequested, this, &CsvAnalysisTab::exportJsonReport);

    connect(m_controlsWidget, &CsvAnalysisControlsWidget::showPlotChanged,
            this, [this](bool) {
                updatePlotVisibility();
                if (m_lastResult.has_value()) { updatePlotPanel(*m_lastResult); }
            });

    connect(m_controlsWidget, &CsvAnalysisControlsWidget::showSkippedRowsChanged,
            this, [this](bool) {
                if (m_lastResult.has_value() && m_resultsPanel != nullptr) {
                    m_resultsPanel->setResults(
                        m_session,
                        *m_lastResult,
                        m_controlsWidget->showSkippedRowsEnabled()
                        );
                }
            });
}

void CsvAnalysisTab::recomputeAnalysis()
{
    CsvAnalysisEngine::AnalysisResult result{};

    if (m_session.dataSet.has_value()) {
        result = CsvAnalysisEngine::analyze(*m_session.dataSet, m_controlsWidget->settings());
    }

    m_lastResult = result;

    updateDataView(result);
    updatePlotPanel(result);

    const bool showSkippedRows = (m_controlsWidget != nullptr && m_controlsWidget->showSkippedRowsEnabled());
    if (m_resultsPanel != nullptr) { m_resultsPanel->setResults(m_session, result, showSkippedRows); }
}

void CsvAnalysisTab::exportJsonReport()
{
    if (!m_session.dataSet.has_value()) {
        QMessageBox::warning(this, "Export JSON", "No CSV data available for export.");
        return;
    }

    if (!m_lastResult.has_value()) {
        QMessageBox::warning(this, "Export JSON", "No analysis result available for export.");
        return;
    }

    const auto& result = *m_lastResult;

    if (result.invalidTimeRange) {
        QMessageBox::warning(this, "Export JSON", "Cannot export report for an invalid time range.");
        return;
    }

    const auto settings = result.usedSettings;
    const bool exportPerSensor = (m_controlsWidget != nullptr && m_controlsWidget->exportPerSensorEnabled());

    const QFileInfo sourceInfo(m_session.filePath);
    const QString suffix = exportPerSensor ? "_per_sensor_report.json" : "_report.json";
    const QString defaultName = sourceInfo.dir().filePath(sourceInfo.completeBaseName() + suffix);

    const QString filePath = QFileDialog::getSaveFileName(this, "Export JSON report", defaultName, "JSON files (*.json);;All files (*)");

    if (filePath.isEmpty()) {
        return;
    }

    std::ofstream out(filePath.toStdString());
    if (!out) {
        QMessageBox::critical(this, "Export JSON", QString("Failed to open output file:\n%1").arg(filePath));
        return;
    }

    pdt::ReportContext ctx{};
    ctx.parsed_ok = m_session.parsedOk;
    ctx.skipped = m_session.skipped;
    ctx.total = m_session.dataSet->size();
    ctx.filtered = result.filteredDataSet.size();

    if (!exportPerSensor && settings.useSensor && !settings.sensor.empty()) { ctx.sensor = settings.sensor; }
    if (settings.useFrom && settings.from.has_value()) { ctx.from = settings.from; }
    if (settings.useTo && settings.to.has_value()) { ctx.to = settings.to; }

    ctx.anomaly_threshold = settings.anomalyThreshold;
    ctx.anomaly_method = toPdtAnomalyMethod(settings.anomalyMethod);
    ctx.top_n = settings.topN;

    if (exportPerSensor) {
        const auto perSensorStats = result.filteredDataSet.stats_by_sensor();
        const auto perSensorAnomalies = pdt::detect_anomalies_per_sensor(
            result.filteredDataSet,
            toPdtAnomalyMethod(settings.anomalyMethod),
            settings.anomalyThreshold,
            settings.topN
            );

        pdt::write_json_report(out, ctx, perSensorStats, perSensorAnomalies);
    } else {
        const auto stats = result.filteredDataSet.stats();
        pdt::write_json_report(out, ctx, stats, result.anomalySummary);
    }

    if (!out) {
        QMessageBox::critical(this, "Export JSON", QString("Failed while writing JSON report:\n%1").arg(filePath));
        return;
    }

    QMessageBox::information(this, "Export JSON", QString("JSON report exported to:\n%1").arg(filePath));
}

void CsvAnalysisTab::updatePlotVisibility()
{
    const bool visible = (m_controlsWidget != nullptr && m_controlsWidget->showPlotEnabled());

    if (m_plotContainer != nullptr) {
        m_plotContainer->setVisible(visible);
    }

    updateGeometry();
    emit preferredSizeChanged();
}

void CsvAnalysisTab::updatePlotPanel(const CsvAnalysisEngine::AnalysisResult& result)
{
    if (m_csvPlotWidget == nullptr) {
        return;
    }

    if (m_controlsWidget == nullptr || !m_controlsWidget->showPlotEnabled()) {
        return;
    }

    if (result.invalidTimeRange || result.filteredDataSet.empty()) {
        m_csvPlotWidget->resetPlot();
        return;
    }

    const auto samples = result.filteredDataSet.samples();
    if (samples.empty()) {
        m_csvPlotWidget->resetPlot();
        return;
    }

    // X axis: sequential sample indices (1, 2, ..., N)
    std::vector<double> xValues(samples.size());
    std::iota(xValues.begin(), xValues.end(), 1.0);

    // Y axis: measurement values extracted from samples
    std::vector<double> yValues(samples.size());
    std::ranges::transform(samples, yValues.begin(),
                           [](const auto& s) { return s.value; });

    // X positions of detected anomalies (indices -> 1-based sample number)
    std::vector<double> markerXValues(result.anomalyIndices.size());
    std::ranges::transform(result.anomalyIndices, markerXValues.begin(),
                           [](std::size_t idx) { return static_cast<double>(idx + 1); });

    // Plot anomaly markers at the mean level so they remain visible
    // without needing a separate series with original sample lookup.
    const QFileInfo fileInfo(m_session.filePath);
    m_csvPlotWidget->updatePlotWithMarkers(
        xValues,
        yValues,
        markerXValues,
        result.meanValue,
        QString("CSV plot - %1").arg(fileInfo.fileName())
        );
}

void CsvAnalysisTab::updateDataView(const CsvAnalysisEngine::AnalysisResult& result)
{
    if (m_csvSamplesModel == nullptr ||
        m_data.placeholderLabel == nullptr ||
        m_data.tableView == nullptr) {
        return;
    }

    if (result.invalidTimeRange) {
        m_csvSamplesModel->clear();
        m_data.placeholderLabel->setText("Invalid time range");
        m_data.placeholderLabel->show();
        m_data.tableView->hide();
        return;
    }

    if (result.filteredDataSet.empty()) {
        m_csvSamplesModel->clear();
        m_data.placeholderLabel->setText("No rows match current filters");
        m_data.placeholderLabel->show();
        m_data.tableView->hide();
        return;
    }

    m_csvSamplesModel->setDataSet(result.filteredDataSet);
    m_data.tableView->show();
    m_data.placeholderLabel->hide();

    m_data.tableView->resizeColumnsToContents();

    m_data.tableView->setColumnWidth(0, m_data.tableView->columnWidth(0) + 20);
    m_data.tableView->setColumnWidth(1, m_data.tableView->columnWidth(1) + 20);
}

} // namespace pdv
