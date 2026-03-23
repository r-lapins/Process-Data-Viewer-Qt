#include "pdv/csv_analysis_tab.h"
#include "pdv/csv_plot_widget.h"

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

namespace pdv {
namespace {

pdt::AnomalyMethod toPdtAnomalyMethod(CsvAnalysisEngine::AnomalyMethod method)
{
    using GUI = CsvAnalysisEngine::AnomalyMethod;
    using PDT = pdt::AnomalyMethod;

    switch (method) {
    case GUI::ZScore:
        return PDT::ZScore;
    case GUI::IQR:
        return PDT::IQR;
    case GUI::MAD:
        return PDT::MAD;
    }

    return PDT::ZScore;
}

QString anomalyMethodToString(CsvAnalysisEngine::AnomalyMethod method)
{
    using enum CsvAnalysisEngine::AnomalyMethod;

    switch (method) {
    case ZScore:
        return "Z-score";
    case IQR:
        return "IQR";
    case MAD:
        return "MAD";
    }

    return "Unknown";
}

QString anomalyScoreLabel(CsvAnalysisEngine::AnomalyMethod method)
{
    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (method) {
    case ZScore:
        return "z";
    case IQR:
        return "iqr";
    case MAD:
        return "mad";
    }

    return "score";
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

    // ===== TOP
    auto* topWidget = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    auto* dataPanel = createDataPanel(topWidget);
    auto* controlsPanel = createControlsPanel(topWidget);
    auto* statsPanel = createStatisticsPanel(topWidget);
    auto* actionsPanel = createActionsPanel(topWidget);
    auto* alertsPanel = createAlertsPanel(topWidget);

    auto* leftColumnWidget = new QWidget(topWidget);
    auto* leftColumnLayout = new QVBoxLayout(leftColumnWidget);
    leftColumnLayout->setContentsMargins(0, 0, 0, 0);
    leftColumnLayout->setSpacing(10);

    auto* leftColumnTopWidget = new QWidget(leftColumnWidget);
    auto* leftColumnTopLayout = new QHBoxLayout(leftColumnTopWidget);
    leftColumnTopLayout->setContentsMargins(0, 0, 0, 0);
    leftColumnTopLayout->setSpacing(10);

    leftColumnTopLayout->addWidget(controlsPanel, 0, Qt::AlignTop);
    leftColumnTopLayout->addWidget(statsPanel, 0, Qt::AlignTop);

    leftColumnLayout->addWidget(leftColumnTopWidget, 0, Qt::AlignTop);
    leftColumnLayout->addWidget(actionsPanel, 0, Qt::AlignTop | Qt::AlignHCenter);
    leftColumnLayout->addStretch();

    topLayout->addWidget(leftColumnWidget, 0, Qt::AlignTop);
    topLayout->addWidget(dataPanel, 1);
    topLayout->addWidget(alertsPanel, 1);

    // ===== BOTTOM
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

    m_dataPlaceholderLabel = new QLabel("No CSV data", dataGroup);
    m_dataPlaceholderLabel->setWordWrap(true);

    m_samplesTableView = new QTableView(dataGroup);
    m_samplesTableView->setModel(m_csvSamplesModel);
    m_samplesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_samplesTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_samplesTableView->setAlternatingRowColors(true);
    m_samplesTableView->setSortingEnabled(false);
    m_samplesTableView->horizontalHeader()->setStretchLastSection(true);
    m_samplesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    dataLayout->addWidget(m_dataPlaceholderLabel);
    dataLayout->addWidget(m_samplesTableView);

    m_samplesTableView->hide();
    m_samplesTableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    dataGroup->setMinimumWidth(400);
    dataGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return dataGroup;
}

QWidget* CsvAnalysisTab::createControlsPanel(QWidget* parent)
{
    auto* controlsGroup = new QGroupBox("Controls", parent);
    auto* controlsLayout = new QFormLayout(controlsGroup);

    m_useSensorCheckBox = new QCheckBox("Enable", controlsGroup);
    m_sensorComboBox = new QComboBox(controlsGroup);
    m_sensorComboBox->setEnabled(false);

    m_useFromCheckBox = new QCheckBox("Enable", controlsGroup);
    m_fromDateEdit = new QDateEdit(controlsGroup);
    m_fromDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setEnabled(false);

    m_fromTimeEdit = new QTimeEdit(controlsGroup);
    m_fromTimeEdit->setDisplayFormat("HH:mm:ss");
    m_fromTimeEdit->setEnabled(false);

    auto* fromWidget = new QWidget(controlsGroup);
    auto* fromLayout = new QHBoxLayout(fromWidget);
    fromLayout->setContentsMargins(0, 0, 0, 0);
    fromLayout->addWidget(m_fromDateEdit);
    fromLayout->addWidget(m_fromTimeEdit);

    m_useToCheckBox = new QCheckBox("Enable", controlsGroup);
    m_toDateEdit = new QDateEdit(controlsGroup);
    m_toDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setEnabled(false);

    m_toTimeEdit = new QTimeEdit(controlsGroup);
    m_toTimeEdit->setDisplayFormat("HH:mm:ss");    
    m_toTimeEdit->setEnabled(false);

    auto* toWidget = new QWidget(controlsGroup);
    auto* toLayout = new QHBoxLayout(toWidget);
    toLayout->setContentsMargins(0, 0, 0, 0);
    toLayout->addWidget(m_toDateEdit);
    toLayout->addWidget(m_toTimeEdit);

    m_anomalyMethodComboBox = new QComboBox(controlsGroup);
    m_anomalyMethodComboBox->addItem("Z-score");
    m_anomalyMethodComboBox->addItem("IQR");
    m_anomalyMethodComboBox->addItem("MAD");

    m_anomalyThresholdSpinBox = new QDoubleSpinBox(controlsGroup);
    m_anomalyThresholdSpinBox->setDecimals(2);
    m_anomalyThresholdLabel = new QLabel("Threshold:", controlsGroup);

    m_topNSpinBox = new QSpinBox(controlsGroup);
    m_topNSpinBox->setRange(1, 100);
    m_topNSpinBox->setValue(20);

    controlsLayout->addRow("Sensor filter:", m_useSensorCheckBox);
    controlsLayout->addRow("Sensor:", m_sensorComboBox);
    controlsLayout->addRow("From filter:", m_useFromCheckBox);
    controlsLayout->addRow("From:", fromWidget);
    controlsLayout->addRow("To filter:", m_useToCheckBox);
    controlsLayout->addRow("To:", toWidget);

    controlsLayout->addRow("Anomaly method:", m_anomalyMethodComboBox);
    controlsLayout->addRow(m_anomalyThresholdLabel, m_anomalyThresholdSpinBox);

    controlsLayout->addRow("Top anomalies:", m_topNSpinBox);

    controlsGroup->setFixedWidth(320);
    controlsGroup->setFixedHeight(350);

    return controlsGroup;
}

QWidget* CsvAnalysisTab::createStatisticsPanel(QWidget* parent)
{
    auto* statsGroup = new QGroupBox("Statistics", parent);
    auto* statsLayout = new QFormLayout(statsGroup);

    m_statsFileTypeValueLabel = new QLabel("-", statsGroup);
    m_statsSkippedValueLabel = new QLabel("-", statsGroup);
    m_statsTotalValueLabel = new QLabel("-", statsGroup);
    m_statsFilteredValueLabel = new QLabel("-", statsGroup);
    m_statsSensorValueLabel = new QLabel("-", statsGroup);
    m_statsFromValueLabel = new QLabel("-", statsGroup);
    m_statsToValueLabel = new QLabel("-", statsGroup);
    m_statsDetectedAnomaliesValueLabel = new QLabel("-", statsGroup);
    m_statsMinValueLabel = new QLabel("-", statsGroup);
    m_statsMaxValueLabel = new QLabel("-", statsGroup);
    m_statsMeanValueLabel = new QLabel("-", statsGroup);
    m_statsStddevValueLabel = new QLabel("-", statsGroup);
    m_statsAnomalyMethodValueLabel = new QLabel("-", statsGroup);
    m_statsAnomalyThresholdLabel = new QLabel("Threshold:", statsGroup);
    m_statsAnomalyThresholdValueLabel = new QLabel("-", statsGroup);

    statsLayout->addRow("File type:", m_statsFileTypeValueLabel);
    statsLayout->addRow("Skipped:", m_statsSkippedValueLabel);
    statsLayout->addRow("Total loaded:", m_statsTotalValueLabel);
    statsLayout->addRow("Filtered:", m_statsFilteredValueLabel);
    statsLayout->addRow("Detected anomalies:", m_statsDetectedAnomaliesValueLabel);

    auto* filterGroup = new QGroupBox(statsGroup);
    auto* filterLayout = new QFormLayout(filterGroup);
    filterLayout->setContentsMargins(5, 5, 5, 5);

    filterLayout->addRow("Sensor:", m_statsSensorValueLabel);
    filterLayout->addRow("Method:", m_statsAnomalyMethodValueLabel);
    filterLayout->addRow(m_statsAnomalyThresholdLabel, m_statsAnomalyThresholdValueLabel);
    filterLayout->addRow("From:", m_statsFromValueLabel);
    filterLayout->addRow("To:", m_statsToValueLabel);

    statsLayout->addRow(filterGroup);

    auto* signalGroup = new QGroupBox(statsGroup);
    auto* signalLayout = new QHBoxLayout(signalGroup);
    signalLayout->setContentsMargins(5, 5, 5, 5);

    auto* signalLeftWidget = new QWidget(signalGroup);
    auto* signalLeftLayout = new QFormLayout(signalLeftWidget);
    signalLeftLayout->setContentsMargins(5, 5, 5, 5);

    auto* signalRightWidget = new QWidget(signalGroup);
    auto* signalRightLayout = new QFormLayout(signalRightWidget);
    signalRightLayout->setContentsMargins(5, 5, 5, 5);

    signalLeftLayout->addRow("Min:", m_statsMinValueLabel);
    signalLeftLayout->addRow("Max:", m_statsMaxValueLabel);

    signalRightLayout->addRow("Mean:", m_statsMeanValueLabel);
    signalRightLayout->addRow("Stddev:", m_statsStddevValueLabel);

    signalLayout->addWidget(signalLeftWidget);
    signalLayout->addWidget(signalRightWidget);

    statsLayout->addRow(signalGroup);

    statsLayout->setLabelAlignment(Qt::AlignLeft);
    statsLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    statsGroup->setFixedWidth(280);
    statsGroup->setFixedHeight(350);
    statsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    return statsGroup;
}

QWidget* CsvAnalysisTab::createAlertsPanel(QWidget* parent)
{
    auto* alertsGroup = new QGroupBox("Alerts", parent);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    alertsGroup->setFixedWidth(500);

    return alertsGroup;
}

QWidget *CsvAnalysisTab::createActionsPanel(QWidget *parent)
{
    auto* actionsGroup = new QGroupBox("Actions", parent);
    auto* actionsLayout = new QGridLayout(actionsGroup);

    m_recomputeButton = new QPushButton("Recompute", actionsGroup);
    m_recomputeButton->setEnabled(false);

    m_exportJsonButton = new QPushButton("Export JSON", actionsGroup);

    m_showPlotButton = new QPushButton("Plot", actionsGroup);
    m_showPlotButton->setCheckable(true);
    m_showPlotButton->setChecked(false);
    m_showPlotButton->setStyleSheet(style);

    m_exportPerSensorCheckBox = new QCheckBox("Per-sensor export", actionsGroup);
    m_exportPerSensorCheckBox->setChecked(false);

    m_showSkippedRowsCheckBox = new QCheckBox("Show skipped rows", actionsGroup);
    m_showSkippedRowsCheckBox->setChecked(false);

    m_autoUpdateCheckBox = new QCheckBox("Auto update", actionsGroup);
    m_autoUpdateCheckBox->setChecked(true);

    actionsLayout->addWidget(m_recomputeButton, 0, 0);
    actionsLayout->addWidget(m_showPlotButton, 0, 1);
    actionsLayout->addWidget(m_exportJsonButton, 0, 2);

    actionsLayout->addWidget(m_autoUpdateCheckBox, 1, 0);
    actionsLayout->addWidget(m_showSkippedRowsCheckBox, 1, 1);
    actionsLayout->addWidget(m_exportPerSensorCheckBox, 1, 2);

    actionsGroup->setFixedWidth(600);
    actionsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    return actionsGroup;
}

void CsvAnalysisTab::resetStatisticsPanel()
{
    m_statsTotalValueLabel->setText("-");
    m_statsFileTypeValueLabel->setText("-");
    m_statsSkippedValueLabel->setText("-");
    m_statsFilteredValueLabel->setText("-");
    m_statsSensorValueLabel->setText("-");
    m_statsFromValueLabel->setText("-");
    m_statsToValueLabel->setText("-");
    m_statsMinValueLabel->setText("-");
    m_statsMaxValueLabel->setText("-");
    m_statsMeanValueLabel->setText("-");
    m_statsStddevValueLabel->setText("-");
    m_statsAnomalyMethodValueLabel->setText("-");
    m_statsAnomalyThresholdValueLabel->setText("-");
    m_statsDetectedAnomaliesValueLabel->setText("-");
}

void CsvAnalysisTab::updateStatisticsPanel(const CsvAnalysisEngine::AnalysisResult& result)
{
    resetStatisticsPanel();

    if (!m_session.dataSet.has_value() || m_session.dataSet->empty()) {
        return;
    }

    const auto& settings = result.usedSettings;

    m_statsFileTypeValueLabel->setText("CSV");
    m_statsSkippedValueLabel->setText(QString::number(static_cast<qulonglong>(m_session.skipped)));
    m_statsTotalValueLabel->setText(QString::number(static_cast<qulonglong>(m_session.dataSet->size())));
    m_statsFilteredValueLabel->setText(QString::number(static_cast<qulonglong>(result.filteredDataSet.size())));

    m_statsSensorValueLabel->setText( settings.useSensor ? QString::fromStdString(settings.sensor) : "All" );

    if (settings.useFrom && settings.from.has_value()) {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(settings.from->time_since_epoch()).count();
        m_statsFromValueLabel->setText(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(secs)).toString("yyyy-MM-dd HH:mm:ss"));
    }
    else {
        m_statsFromValueLabel->setText("-");
    }

    if (settings.useTo && settings.to.has_value()) {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                              settings.to->time_since_epoch()
                              ).count();

        m_statsToValueLabel->setText(
            QDateTime::fromSecsSinceEpoch(static_cast<qint64>(secs)).toString("yyyy-MM-dd HH:mm:ss")
            );
    } else {
        m_statsToValueLabel->setText("-");
    }

    m_statsDetectedAnomaliesValueLabel->setText(
        QString::number(static_cast<qulonglong>(result.anomalySummary.count))
        );

    m_statsMinValueLabel->setText(QString::number(result.minValue, 'f', 2));
    m_statsMaxValueLabel->setText(QString::number(result.maxValue, 'f', 2));
    m_statsMeanValueLabel->setText(QString::number(result.meanValue, 'f', 2));
    m_statsStddevValueLabel->setText(QString::number(result.stddevValue, 'f', 2));
    m_statsAnomalyMethodValueLabel->setText(anomalyMethodToString(settings.anomalyMethod));
    m_statsAnomalyThresholdLabel->setText(anomalyThresholdLabelText(settings.anomalyMethod));
    m_statsAnomalyThresholdValueLabel->setText(QString::number(settings.anomalyThreshold, 'f', 2));

    m_statsAnomalyThresholdLabel->setToolTip(anomalyThresholdTooltip(settings.anomalyMethod));
    m_statsAnomalyThresholdValueLabel->setToolTip(anomalyThresholdTooltip(settings.anomalyMethod));
}

void CsvAnalysisTab::resetAlertsPanel()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

void CsvAnalysisTab::updateAlertsPanel(const CsvAnalysisEngine::AnalysisResult& result)
{
    resetAlertsPanel();

    if (!m_session.dataSet.has_value()) {
        return;
    }

    if (result.invalidTimeRange) {
        m_alertsListWidget->clear();
        m_alertsListWidget->addItem("Invalid time range: From is later than To");
        return;
    }

    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("Detected anomalies:");

    if (result.filteredDataSet.empty() || result.anomalySummary.top.empty()) {
        m_alertsListWidget->addItem("No anomalies detected");
    }
    else {
        const QString scoreLabel = anomalyScoreLabel(result.usedSettings.anomalyMethod);

        for (std::size_t anomalyPos = 0; anomalyPos < result.anomalySummary.top.size(); ++anomalyPos) {
            const auto& a = result.anomalySummary.top[anomalyPos];

            const auto ts = std::chrono::system_clock::to_time_t(a.timestamp);
            const QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts));
            const QString dateText = dt.date().toString("yyyy-MM-dd");
            const QString timeText = dt.time().toString("HH:mm:ss");

            QString indexText = "?";

            if (anomalyPos < result.anomalyIndices.size()) {
                // 1-based row index, matching the row numbering visible in the table
                indexText = QString::number(static_cast<qulonglong>(result.anomalyIndices[anomalyPos] + 1));
            }

            m_alertsListWidget->addItem(
                QString("#%1  |  %2  %3  |  %4  |  value = %5  |  %6 = %7")
                    .arg(indexText,
                         dateText,
                         timeText,
                         QString::fromStdString(a.sensor),
                         QString::number(a.value, 'f', 1),
                         scoreLabel,
                         QString::number(a.score, 'f', 2))
                );
        }
    }

    if (!m_session.skippedRows.empty() && m_showSkippedRowsCheckBox->isChecked()) {
        m_alertsListWidget->addItem("------------ Skipped rows ------------");

        for (const auto& row : m_session.skippedRows) {
            m_alertsListWidget->addItem(
                QString("line %1: %2")
                    .arg(row.line_number)
                    .arg(QString::fromStdString(row.text))
                );
        }
    }
}

void CsvAnalysisTab::populateSensorOptions()
{
    if (m_sensorComboBox == nullptr) {
        return;
    }

    m_sensorComboBox->clear();

    if (!m_session.dataSet.has_value()) {
        m_sensorComboBox->setEnabled(false);
        return;
    }

    std::set<QString> sensors;
    for (const auto& sample : m_session.dataSet->samples()) {
        sensors.insert(QString::fromStdString(sample.sensor));
    }

    for (const auto& sensor : sensors) {
        m_sensorComboBox->addItem(sensor);
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

    m_fromDateEdit->setMinimumDate(minDt.date());
    m_fromDateEdit->setMaximumDate(maxDt.date());
    m_toDateEdit->setMinimumDate(minDt.date());
    m_toDateEdit->setMaximumDate(maxDt.date());

    m_fromDateEdit->setDate(minDt.date());
    m_toDateEdit->setDate(maxDt.date());

    m_fromTimeEdit->setTime(QTime(0, 0, 0));
    m_toTimeEdit->setTime(QTime(23, 59, 59));
}

void CsvAnalysisTab::connectControls()
{
    connect(m_recomputeButton, &QPushButton::clicked, this, &CsvAnalysisTab::recomputeAnalysis);
    connect(m_exportJsonButton, &QPushButton::clicked, this, &CsvAnalysisTab::exportJsonReport);

    connect(m_autoUpdateCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_recomputeButton->setEnabled(!checked);
    });

    connect(m_showPlotButton, &QPushButton::toggled, this, [this]() {
        updatePlotVisibility();
        if (m_lastResult.has_value()) { updatePlotPanel(*m_lastResult); }
    });

    connect(m_showSkippedRowsCheckBox, &QCheckBox::toggled, this, [this]() {
        if (m_lastResult.has_value()) { updateAlertsPanel(*m_lastResult); }
    });

    auto trigger = [this]() {
        if (m_autoUpdateCheckBox->isChecked()) { recomputeAnalysis(); }
    };

    connect(m_anomalyMethodComboBox, &QComboBox::currentIndexChanged, this, [this, trigger]() {
        updateAnomalyThresholdControls();
        trigger();
    });

    connect(m_exportPerSensorCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) { return; }

        if (m_useSensorCheckBox != nullptr && m_useSensorCheckBox->isChecked()) {
            m_useSensorCheckBox->setChecked(false);

            QMessageBox::information(this, "Per-sensor export",
                "Sensor filter has been disabled because per-sensor export is selected."
                );
        }
    });

    connect(m_useSensorCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            return;
        }

        if (m_exportPerSensorCheckBox != nullptr && m_exportPerSensorCheckBox->isChecked()) {
            m_exportPerSensorCheckBox->setChecked(false);
        }
    });

    connect(m_sensorComboBox, &QComboBox::currentIndexChanged, this, trigger);
    connect(m_fromDateEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_fromTimeEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_toDateEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_toTimeEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_topNSpinBox, &QSpinBox::valueChanged, this, trigger);
    connect(m_useSensorCheckBox, &QCheckBox::toggled, this, trigger);
    connect(m_useFromCheckBox, &QCheckBox::toggled, this, trigger);
    connect(m_useToCheckBox, &QCheckBox::toggled, this, trigger);

    connect(m_anomalyThresholdSpinBox, &QDoubleSpinBox::valueChanged, this, trigger);

    connect(m_useSensorCheckBox, &QCheckBox::toggled, m_sensorComboBox, &QWidget::setEnabled);
    connect(m_useFromCheckBox, &QCheckBox::toggled, m_fromDateEdit, &QWidget::setEnabled);
    connect(m_useFromCheckBox, &QCheckBox::toggled, m_fromTimeEdit, &QWidget::setEnabled);
    connect(m_useToCheckBox, &QCheckBox::toggled, m_toDateEdit, &QWidget::setEnabled);
    connect(m_useToCheckBox, &QCheckBox::toggled, m_toTimeEdit, &QWidget::setEnabled);
}

void CsvAnalysisTab::displaySessionData(const pdt::DataSet& filtered)
{
    if (m_csvSamplesModel == nullptr || m_samplesTableView == nullptr || m_dataPlaceholderLabel == nullptr) {
        return;
    }

    m_csvSamplesModel->setDataSet(filtered);
    m_samplesTableView->show();
    m_dataPlaceholderLabel->hide();

    m_samplesTableView->resizeColumnsToContents();

    m_samplesTableView->setColumnWidth(0, m_samplesTableView->columnWidth(0) + 20);
    m_samplesTableView->setColumnWidth(1, m_samplesTableView->columnWidth(1) + 20);
}

void CsvAnalysisTab::recomputeAnalysis()
{
    CsvAnalysisEngine::AnalysisResult result{};

    if (m_session.dataSet.has_value()) {
        result = CsvAnalysisEngine::analyze(*m_session.dataSet, currentSettings());
    }

    m_lastResult = result;

    updateDataView(result);
    updateStatisticsPanel(result);
    updateAlertsPanel(result);
    updatePlotPanel(result);
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
    const bool exportPerSensor = (m_exportPerSensorCheckBox != nullptr && m_exportPerSensorCheckBox->isChecked());

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

    s.sensor = m_sensorComboBox->currentText().trimmed().toStdString();
    s.useSensor = m_useSensorCheckBox->isChecked() && !s.sensor.empty();

    s.useFrom = m_useFromCheckBox->isChecked();
    s.useTo = m_useToCheckBox->isChecked();

    if (s.useFrom) {
        const QDateTime dt(m_fromDateEdit->date(), m_fromTimeEdit->time());
        s.from = std::chrono::sys_seconds{std::chrono::seconds{dt.toSecsSinceEpoch()}};
    }

    if (s.useTo) {
        const QDateTime dt(m_toDateEdit->date(), m_toTimeEdit->time());
        s.to = std::chrono::sys_seconds{std::chrono::seconds{dt.toSecsSinceEpoch()}};
    }

    s.anomalyMethod = currentAnomalyMethod();
    s.anomalyThreshold = m_anomalyThresholdSpinBox->value();
    s.topN = static_cast<std::size_t>(m_topNSpinBox->value());

    return s;
}

void CsvAnalysisTab::updateAnomalyThresholdControls()
{
    if (m_anomalyThresholdSpinBox == nullptr) { return; }

    // Preserve the threshold value used by the method that is currently active
    // before switching the UI to the newly selected method.
    const double currentValue = m_anomalyThresholdSpinBox->value();

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
    if (m_anomalyThresholdSpinBox == nullptr || m_anomalyThresholdLabel == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_anomalyThresholdSpinBox);
    // Prevent valueChanged from firing while the control is reconfigured.

    m_anomalyThresholdLabel->setText(anomalyThresholdLabelText(method));    
    m_anomalyThresholdLabel->setToolTip(anomalyThresholdTooltip(method));
    m_anomalyThresholdSpinBox->setToolTip(anomalyThresholdTooltip(method));

    m_anomalyThresholdSpinBox->setDecimals(2);

    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (method) {
    case ZScore:
        m_anomalyThresholdSpinBox->setRange(0.1, 10.0);
        m_anomalyThresholdSpinBox->setSingleStep(0.1);
        m_anomalyThresholdSpinBox->setValue(m_lastZScoreThreshold);
        break;

    case IQR:
        m_anomalyThresholdSpinBox->setRange(0.1, 10.0);
        m_anomalyThresholdSpinBox->setSingleStep(0.1);
        m_anomalyThresholdSpinBox->setValue(m_lastIqrThreshold);
        break;

    case MAD:
        m_anomalyThresholdSpinBox->setRange(0.1, 20.0);
        m_anomalyThresholdSpinBox->setSingleStep(0.1);
        m_anomalyThresholdSpinBox->setValue(m_lastMadThreshold);
        break;
    }
}

CsvAnalysisEngine::AnomalyMethod CsvAnalysisTab::currentAnomalyMethod() const
{
    using enum CsvAnalysisEngine::AnomalyMethod;

    if (m_anomalyMethodComboBox == nullptr) {
        return ZScore;
    }

    switch (m_anomalyMethodComboBox->currentIndex()) {
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

QString CsvAnalysisTab::anomalyThresholdLabelText(CsvAnalysisEngine::AnomalyMethod method) const
{
    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (method) {
    case ZScore:
        return "Z-threshold:";
    case IQR:
        return "IQR multiplier:";
    case MAD:
        return "MAD threshold:";
    }

    return "Threshold:";
}

QString CsvAnalysisTab::anomalyThresholdTooltip(CsvAnalysisEngine::AnomalyMethod method) const
{
    using enum CsvAnalysisEngine::AnomalyMethod;
    switch (method) {
    case ZScore:
        return "Absolute z-score threshold used to classify anomalies.";
    case IQR:
        return "Multiplier applied to the interquartile range: lower = Q1 - k·IQR, upper = Q3 + k·IQR.";
    case MAD:
        return "Absolute score threshold computed from deviation relative to the median and MAD.";
    }

    return {};
}

void CsvAnalysisTab::updatePlotVisibility()
{
    const bool visible = (m_showPlotButton != nullptr && m_showPlotButton->isChecked());

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

    if (m_showPlotButton == nullptr || !m_showPlotButton->isChecked()) {
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
        m_dataPlaceholderLabel == nullptr ||
        m_samplesTableView == nullptr) {
        return;
    }

    if (result.invalidTimeRange) {
        m_csvSamplesModel->clear();
        m_dataPlaceholderLabel->setText("Invalid time range");
        m_dataPlaceholderLabel->show();
        m_samplesTableView->hide();
        return;
    }

    if (result.filteredDataSet.empty()) {
        m_csvSamplesModel->clear();
        m_dataPlaceholderLabel->setText("No rows match current filters");
        m_dataPlaceholderLabel->show();
        m_samplesTableView->hide();
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
