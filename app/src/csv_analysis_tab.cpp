#include "pdv/csv_analysis_tab.h"
#include "pdv/csv_plot_widget.h"
#include "pdv/csv_results_panel.h"

#include <pdt/core/report.h>

#include <set>
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
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QSignalBlocker>
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

QString anomalyThresholdLabelText(CsvAnalysisEngine::AnomalyMethod method)
{
    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (method) {
    case ZScore:    return "Z-threshold:";
    case IQR:       return "IQR multiplier:";
    case MAD:       return "MAD threshold:";
    }

    return "Threshold:";
}

QString anomalyThresholdTooltip(CsvAnalysisEngine::AnomalyMethod method)
{
    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (method) {
    case ZScore:    return "Absolute z-score threshold used to classify anomalies.";
    case IQR:       return "Multiplier applied to the interquartile range: lower = Q1 - k·IQR, upper = Q3 + k·IQR.";
    case MAD:       return "Absolute score threshold computed from deviation relative to the median and MAD.";
    }

    return {};
}

} // namespace

CsvAnalysisTab::CsvAnalysisTab(const SessionData& session, QWidget* parent)
    : AnalysisTab(session, parent)
{
    m_csvSamplesModel = new CsvSamplesTableModel(this);

    createUi();

    populateSensorOptions();
    initializeDateControls();
    connectControls();

    m_currentMethodUi = currentAnomalyMethod();
    applyAnomalyMethodUi(m_currentMethodUi);

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

    auto* controlsPanel = createControlsPanel(topWidget);
    auto* dataPanel = createDataPanel(topWidget);
    auto* actionsPanel = createActionsPanel(topWidget);

    controlsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    actionsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dataPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dataPanel->setMinimumWidth(400);

    m_resultsPanel = new CsvResultsPanel(topWidget);
    m_resultsPanel->setFixedWidth(700);

    auto* leftColumnWidget = new QWidget(topWidget);
    auto* leftColumnLayout = new QVBoxLayout(leftColumnWidget);
    leftColumnLayout->setContentsMargins(0, 0, 0, 0);

    leftColumnLayout->addWidget(controlsPanel, 0, Qt::AlignTop);
    leftColumnLayout->addWidget(actionsPanel, 0, Qt::AlignTop);
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

QWidget* CsvAnalysisTab::createControlsPanel(QWidget* parent)
{
    auto* controlsGroup = new QGroupBox("Controls", parent);
    auto* controlsLayout = new QFormLayout(controlsGroup);

    m_filters.useSensorCheckBox = new QCheckBox("Enable", controlsGroup);
    m_filters.sensorComboBox = new QComboBox(controlsGroup);
    m_filters.sensorComboBox->setEnabled(false);

    m_filters.useFromCheckBox = new QCheckBox("Enable", controlsGroup);
    m_filters.fromDateEdit = new QDateEdit(controlsGroup);
    m_filters.fromDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_filters.fromDateEdit->setCalendarPopup(true);
    m_filters.fromDateEdit->setEnabled(false);

    m_filters.fromTimeEdit = new QTimeEdit(controlsGroup);
    m_filters.fromTimeEdit->setDisplayFormat("HH:mm:ss");
    m_filters.fromTimeEdit->setEnabled(false);

    auto* fromWidget = new QWidget(controlsGroup);
    auto* fromLayout = new QHBoxLayout(fromWidget);
    fromLayout->setContentsMargins(0, 0, 0, 0);
    fromLayout->addWidget(m_filters.fromDateEdit);
    fromLayout->addWidget(m_filters.fromTimeEdit);

    m_filters.useToCheckBox = new QCheckBox("Enable", controlsGroup);
    m_filters.toDateEdit = new QDateEdit(controlsGroup);
    m_filters.toDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_filters.toDateEdit->setCalendarPopup(true);
    m_filters.toDateEdit->setEnabled(false);

    m_filters.toTimeEdit = new QTimeEdit(controlsGroup);
    m_filters.toTimeEdit->setDisplayFormat("HH:mm:ss");
    m_filters.toTimeEdit->setEnabled(false);

    auto* toWidget = new QWidget(controlsGroup);
    auto* toLayout = new QHBoxLayout(toWidget);
    toLayout->setContentsMargins(0, 0, 0, 0);
    toLayout->addWidget(m_filters.toDateEdit);
    toLayout->addWidget(m_filters.toTimeEdit);

    m_analysis.anomalyMethodComboBox = new QComboBox(controlsGroup);
    m_analysis.anomalyMethodComboBox->addItem("Z-score");
    m_analysis.anomalyMethodComboBox->addItem("IQR");
    m_analysis.anomalyMethodComboBox->addItem("MAD");

    m_analysis.anomalyThresholdSpinBox = new QDoubleSpinBox(controlsGroup);
    m_analysis.anomalyThresholdSpinBox->setDecimals(2);
    m_analysis.anomalyThresholdLabel = new QLabel("Threshold:", controlsGroup);

    m_analysis.topNSpinBox = new QSpinBox(controlsGroup);
    m_analysis.topNSpinBox->setRange(1, 100);
    m_analysis.topNSpinBox->setValue(20);

    controlsLayout->addRow("Sensor filter:", m_filters.useSensorCheckBox);
    controlsLayout->addRow("Sensor:", m_filters.sensorComboBox);
    controlsLayout->addRow("From filter:", m_filters.useFromCheckBox);
    controlsLayout->addRow("From:", fromWidget);
    controlsLayout->addRow("To filter:", m_filters.useToCheckBox);
    controlsLayout->addRow("To:", toWidget);
    controlsLayout->addRow("Anomaly method:", m_analysis.anomalyMethodComboBox);
    controlsLayout->addRow(m_analysis.anomalyThresholdLabel, m_analysis.anomalyThresholdSpinBox);
    controlsLayout->addRow("Top anomalies:", m_analysis.topNSpinBox);

    return controlsGroup;
}

QWidget *CsvAnalysisTab::createActionsPanel(QWidget *parent)
{
    auto* actionsGroup = new QGroupBox(parent);
    auto* actionsLayout = new QGridLayout(actionsGroup);

    m_actions.recomputeButton = new QPushButton("Recompute", actionsGroup);
    m_actions.recomputeButton->setEnabled(false);

    m_actions.exportJsonButton = new QPushButton("Export JSON", actionsGroup);

    m_actions.showPlotButton = new QPushButton("Plot", actionsGroup);
    m_actions.showPlotButton->setCheckable(true);
    m_actions.showPlotButton->setChecked(false);
    m_actions.showPlotButton->setStyleSheet(style);

    m_actions.exportPerSensorCheckBox = new QCheckBox("Per-sensor", actionsGroup);
    m_actions.exportPerSensorCheckBox->setChecked(false);

    m_actions.showSkippedRowsCheckBox = new QCheckBox("Show skipped", actionsGroup);
    m_actions.showSkippedRowsCheckBox->setChecked(false);

    m_actions.autoUpdateCheckBox = new QCheckBox("Auto update", actionsGroup);
    m_actions.autoUpdateCheckBox->setChecked(true);

    actionsLayout->addWidget(m_actions.recomputeButton, 0, 0);
    actionsLayout->addWidget(m_actions.showPlotButton, 0, 1);
    actionsLayout->addWidget(m_actions.exportJsonButton, 0, 2);

    actionsLayout->addWidget(m_actions.autoUpdateCheckBox, 1, 0);
    actionsLayout->addWidget(m_actions.showSkippedRowsCheckBox, 1, 1);
    actionsLayout->addWidget(m_actions.exportPerSensorCheckBox, 1, 2);

    return actionsGroup;
}

void CsvAnalysisTab::populateSensorOptions()
{
    if (m_filters.sensorComboBox == nullptr) {
        return;
    }

    m_filters.sensorComboBox->clear();

    if (!m_session.dataSet.has_value()) {
        m_filters.sensorComboBox->setEnabled(false);
        return;
    }

    std::set<QString> sensors;
    for (const auto& sample : m_session.dataSet->samples()) {
        sensors.insert(QString::fromStdString(sample.sensor));
    }

    for (const auto& sensor : sensors) {
        m_filters.sensorComboBox->addItem(sensor);
    }
}

void CsvAnalysisTab::initializeDateControls()
{
    if (!m_session.dataSet.has_value()) {
        return;
    }

    const auto samples = m_session.dataSet->samples();
    if (samples.empty()) {
        return;
    }

    auto minTs = samples.front().timestamp;
    auto maxTs = samples.front().timestamp;

    for (const auto& s : samples) {
        if (s.timestamp < minTs) minTs = s.timestamp;
        if (s.timestamp > maxTs) maxTs = s.timestamp;
    }

    const auto minSecs = std::chrono::duration_cast<std::chrono::seconds>(
                             minTs.time_since_epoch()).count();
    const auto maxSecs = std::chrono::duration_cast<std::chrono::seconds>(
                             maxTs.time_since_epoch()).count();

    const QDateTime minDt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(minSecs));
    const QDateTime maxDt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(maxSecs));

    m_filters.fromDateEdit->setMinimumDate(minDt.date());
    m_filters.fromDateEdit->setMaximumDate(maxDt.date());
    m_filters.toDateEdit->setMinimumDate(minDt.date());
    m_filters.toDateEdit->setMaximumDate(maxDt.date());

    m_filters.fromDateEdit->setDate(minDt.date());
    m_filters.toDateEdit->setDate(maxDt.date());

    m_filters.fromTimeEdit->setTime(QTime(0, 0, 0));
    m_filters.toTimeEdit->setTime(QTime(23, 59, 59));
}

void CsvAnalysisTab::connectControls()
{
    connect(m_actions.recomputeButton, &QPushButton::clicked, this, &CsvAnalysisTab::recomputeAnalysis);
    connect(m_actions.exportJsonButton, &QPushButton::clicked, this, &CsvAnalysisTab::exportJsonReport);

    connect(m_actions.autoUpdateCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_actions.recomputeButton->setEnabled(!checked);
    });

    connect(m_actions.showPlotButton, &QPushButton::toggled, this, [this]() {
        updatePlotVisibility();
        if (m_lastResult.has_value()) { updatePlotPanel(*m_lastResult); }
    });

    connect(m_actions.showSkippedRowsCheckBox, &QCheckBox::toggled, this, [this]() {
        if (m_lastResult.has_value()) {
            m_resultsPanel->setResults(m_session, *m_lastResult, m_actions.showSkippedRowsCheckBox->isChecked());
        }
    });

    auto trigger = [this]() {
        if (m_actions.autoUpdateCheckBox->isChecked()) { recomputeAnalysis(); }
    };

    connect(m_analysis.anomalyMethodComboBox, &QComboBox::currentIndexChanged, this, [this, trigger]() {
        updateAnomalyThresholdControls();
        trigger();
    });

    connect(m_actions.exportPerSensorCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) { return; }

        if (m_filters.useSensorCheckBox != nullptr && m_filters.useSensorCheckBox->isChecked()) {
            m_filters.useSensorCheckBox->setChecked(false);

            QMessageBox::information(this, "Per-sensor export",
                "Sensor filter has been disabled because per-sensor export is selected."
                );
        }
    });

    connect(m_filters.useSensorCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            return;
        }

        if (m_actions.exportPerSensorCheckBox != nullptr && m_actions.exportPerSensorCheckBox->isChecked()) {
            m_actions.exportPerSensorCheckBox->setChecked(false);
        }
    });

    connect(m_filters.sensorComboBox, &QComboBox::currentIndexChanged, this, trigger);
    connect(m_filters.fromDateEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_filters.fromTimeEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_filters.toDateEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_filters.toTimeEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_analysis.topNSpinBox, &QSpinBox::valueChanged, this, trigger);
    connect(m_filters.useSensorCheckBox, &QCheckBox::toggled, this, trigger);
    connect(m_filters.useFromCheckBox, &QCheckBox::toggled, this, trigger);
    connect(m_filters.useToCheckBox, &QCheckBox::toggled, this, trigger);

    connect(m_analysis.anomalyThresholdSpinBox, &QDoubleSpinBox::valueChanged, this, trigger);

    connect(m_filters.useSensorCheckBox, &QCheckBox::toggled, m_filters.sensorComboBox, &QWidget::setEnabled);
    connect(m_filters.useFromCheckBox, &QCheckBox::toggled, m_filters.fromDateEdit, &QWidget::setEnabled);
    connect(m_filters.useFromCheckBox, &QCheckBox::toggled, m_filters.fromTimeEdit, &QWidget::setEnabled);
    connect(m_filters.useToCheckBox, &QCheckBox::toggled, m_filters.toDateEdit, &QWidget::setEnabled);
    connect(m_filters.useToCheckBox, &QCheckBox::toggled, m_filters.toTimeEdit, &QWidget::setEnabled);
}

void CsvAnalysisTab::displaySessionData(const pdt::DataSet& filtered)
{
    if (m_csvSamplesModel == nullptr || m_data.tableView == nullptr || m_data.placeholderLabel == nullptr) {
        return;
    }

    m_csvSamplesModel->setDataSet(filtered);
    m_data.tableView->show();
    m_data.placeholderLabel->hide();

    m_data.tableView->resizeColumnsToContents();

    m_data.tableView->setColumnWidth(0, m_data.tableView->columnWidth(0) + 20);
    m_data.tableView->setColumnWidth(1, m_data.tableView->columnWidth(1) + 20);
}

void CsvAnalysisTab::recomputeAnalysis()
{
    CsvAnalysisEngine::AnalysisResult result{};

    if (m_session.dataSet.has_value()) {
        result = CsvAnalysisEngine::analyze(*m_session.dataSet, currentSettings());
    }

    m_lastResult = result;

    updateDataView(result);
    updatePlotPanel(result);

    bool showSkippedRows = m_actions.showSkippedRowsCheckBox != nullptr && m_actions.showSkippedRowsCheckBox->isChecked();
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
    const bool exportPerSensor = (m_actions.exportPerSensorCheckBox != nullptr && m_actions.exportPerSensorCheckBox->isChecked());

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

CsvAnalysisEngine::AnalysisSettings CsvAnalysisTab::currentSettings() const
{
    CsvAnalysisEngine::AnalysisSettings s{};

    s.sensor = m_filters.sensorComboBox->currentText().trimmed().toStdString();
    s.useSensor = m_filters.useSensorCheckBox->isChecked() && !s.sensor.empty();

    s.useFrom = m_filters.useFromCheckBox->isChecked();
    s.useTo = m_filters.useToCheckBox->isChecked();

    if (s.useFrom) {
        const QDateTime dt(m_filters.fromDateEdit->date(), m_filters.fromTimeEdit->time());
        s.from = std::chrono::sys_seconds{std::chrono::seconds{dt.toSecsSinceEpoch()}};
    }

    if (s.useTo) {
        const QDateTime dt(m_filters.toDateEdit->date(), m_filters.toTimeEdit->time());
        s.to = std::chrono::sys_seconds{std::chrono::seconds{dt.toSecsSinceEpoch()}};
    }

    s.anomalyMethod = currentAnomalyMethod();
    s.anomalyThreshold = m_analysis.anomalyThresholdSpinBox->value();
    s.topN = static_cast<std::size_t>(m_analysis.topNSpinBox->value());

    return s;
}

void CsvAnalysisTab::updateAnomalyThresholdControls()
{
    if (m_analysis.anomalyThresholdSpinBox == nullptr) { return; }

    // Preserve the threshold value used by the method that is currently active
    // before switching the UI to the newly selected method.
    const double currentValue = m_analysis.anomalyThresholdSpinBox->value();

    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (m_currentMethodUi) {
    case ZScore:
        m_lastZScoreThreshold = currentValue;
        break;
    case IQR:
        m_lastIqrThreshold = currentValue;
        break;
    case MAD:
        m_lastMadThreshold = currentValue;
        break;
    }

    // Apply UI settings and restore the remembered threshold
    // for the newly selected anomaly method.
    m_currentMethodUi = currentAnomalyMethod();
    applyAnomalyMethodUi(m_currentMethodUi);
}

void CsvAnalysisTab::applyAnomalyMethodUi(CsvAnalysisEngine::AnomalyMethod method)
{
    if (m_analysis.anomalyThresholdSpinBox == nullptr || m_analysis.anomalyThresholdLabel == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_analysis.anomalyThresholdSpinBox);
    // Prevent valueChanged from firing while the control is reconfigured.

    m_analysis.anomalyThresholdLabel->setText(anomalyThresholdLabelText(method));
    m_analysis.anomalyThresholdLabel->setToolTip(anomalyThresholdTooltip(method));
    m_analysis.anomalyThresholdSpinBox->setToolTip(anomalyThresholdTooltip(method));

    m_analysis.anomalyThresholdSpinBox->setDecimals(2);

    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (method) {
    case ZScore:
        m_analysis.anomalyThresholdSpinBox->setRange(0.1, 10.0);
        m_analysis.anomalyThresholdSpinBox->setSingleStep(0.1);
        m_analysis.anomalyThresholdSpinBox->setValue(m_lastZScoreThreshold);
        break;

    case IQR:
        m_analysis.anomalyThresholdSpinBox->setRange(0.1, 10.0);
        m_analysis.anomalyThresholdSpinBox->setSingleStep(0.1);
        m_analysis.anomalyThresholdSpinBox->setValue(m_lastIqrThreshold);
        break;

    case MAD:
        m_analysis.anomalyThresholdSpinBox->setRange(0.1, 20.0);
        m_analysis.anomalyThresholdSpinBox->setSingleStep(0.1);
        m_analysis.anomalyThresholdSpinBox->setValue(m_lastMadThreshold);
        break;
    }
}

CsvAnalysisEngine::AnomalyMethod CsvAnalysisTab::currentAnomalyMethod() const
{
    using enum CsvAnalysisEngine::AnomalyMethod;

    if (m_analysis.anomalyMethodComboBox == nullptr) {
        return ZScore;
    }

    switch (m_analysis.anomalyMethodComboBox->currentIndex()) {
    case 0:
        return ZScore;
    case 1:
        return IQR;
    case 2:
        return MAD;
    default:
        return ZScore;
    }
}

void CsvAnalysisTab::updatePlotVisibility()
{
    const bool visible = (m_actions.showPlotButton != nullptr && m_actions.showPlotButton->isChecked());

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

    if (m_actions.showPlotButton == nullptr || !m_actions.showPlotButton->isChecked()) {
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

    displaySessionData(result.filteredDataSet);
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

} // namespace pdv
