#include "pdv/csv_analysis_tab.h"

#include <pdt/core/anomaly.h>

#include <set>

#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>

namespace pdv {

CsvAnalysisTab::CsvAnalysisTab(const SessionData& session, QWidget* parent)
    : AnalysisTab(session, parent)
{
    m_csvSamplesModel = new CsvSamplesTableModel(this);
    createUi();
}

void CsvAnalysisTab::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    splitter->addWidget(createDataPanel(splitter));
    splitter->addWidget(createControlsPanel(splitter));

    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addWidget(createStatisticsPanel(rightPanel));
    rightLayout->addWidget(createAlertsPanel(rightPanel));

    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 3);

    rootLayout->addWidget(splitter);

    rightPanel->setFixedWidth(400);

    rootLayout->setSizeConstraint(QLayout::SetMinimumSize);

    populateSensorOptions();
    initializeDateControls();
    connectControls();
    recomputeAnalysis();
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

    dataGroup->setMinimumWidth(380);

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

    m_zThresholdSpinBox = new QDoubleSpinBox(controlsGroup);
    m_zThresholdSpinBox->setRange(0.1, 10.0);
    m_zThresholdSpinBox->setDecimals(2);
    m_zThresholdSpinBox->setSingleStep(0.1);
    m_zThresholdSpinBox->setValue(1.5);

    m_topNSpinBox = new QSpinBox(controlsGroup);
    m_topNSpinBox->setRange(1, 100);
    m_topNSpinBox->setValue(20);

    m_autoUpdateCheckBox = new QCheckBox("Auto update", controlsGroup);
    m_autoUpdateCheckBox->setChecked(true);

    m_recomputeButton = new QPushButton("Recompute", controlsGroup);
    m_recomputeButton->setEnabled(false);

    m_showSkippedRowsCheckBox = new QCheckBox("Show skipped?", controlsGroup);
    m_showSkippedRowsCheckBox->setChecked(false);

    controlsLayout->addRow("Sensor filter:", m_useSensorCheckBox);
    controlsLayout->addRow("Sensor:", m_sensorComboBox);
    controlsLayout->addRow("From filter:", m_useFromCheckBox);
    controlsLayout->addRow("From:", fromWidget);
    controlsLayout->addRow("To filter:", m_useToCheckBox);
    controlsLayout->addRow("To:", toWidget);
    controlsLayout->addRow("Z-threshold:", m_zThresholdSpinBox);
    controlsLayout->addRow("Top anomalies:", m_topNSpinBox);
    controlsLayout->addRow("", m_recomputeButton);
    controlsLayout->addRow("", m_autoUpdateCheckBox);
    controlsLayout->addRow("", m_showSkippedRowsCheckBox);

    controlsGroup->setFixedWidth(320);

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
    m_statsZThresholdValueLabel = new QLabel("-", statsGroup);

    statsLayout->addRow("File type:", m_statsFileTypeValueLabel);
    statsLayout->addRow("Skipped:", m_statsSkippedValueLabel);
    statsLayout->addRow("Total loaded:", m_statsTotalValueLabel);
    statsLayout->addRow("Filtered:", m_statsFilteredValueLabel);

    auto* filterGroup = new QGroupBox(statsGroup);
    auto* filterLayout = new QFormLayout(filterGroup);
    filterLayout->setContentsMargins(0, 0, 0, 0);

    filterLayout->addRow("Sensor:", m_statsSensorValueLabel);
    filterLayout->addRow("Z-threshold:", m_statsZThresholdValueLabel);
    filterLayout->addRow("From:", m_statsFromValueLabel);
    filterLayout->addRow("To:", m_statsToValueLabel);

    statsLayout->addRow(filterGroup);

    auto* signalGroup = new QGroupBox(statsGroup);
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
    statsLayout->addRow("Detected anomalies:", m_statsDetectedAnomaliesValueLabel);

    statsLayout->setLabelAlignment(Qt::AlignLeft);
    statsLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    return statsGroup;
}

QWidget* CsvAnalysisTab::createAlertsPanel(QWidget* parent)
{
    auto* alertsGroup = new QGroupBox("Alerts", parent);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    return alertsGroup;
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
    m_statsZThresholdValueLabel->setText("-");
    m_statsDetectedAnomaliesValueLabel->setText("-");
}

void CsvAnalysisTab::updateStatisticsPanel(const pdt::DataSet& filtered)
{
    resetStatisticsPanel();

    if (!m_session.dataSet.has_value()) {
        return;
    }

    const auto settings = currentSettings();
    const auto stats = filtered.stats();
    const auto summary = pdt::detect_zscore_global(filtered, settings.zThreshold, settings.topN);

    m_statsFileTypeValueLabel->setText("CSV");

    m_statsSkippedValueLabel->setText(
        QString::number(static_cast<qulonglong>(m_session.skipped))
        );

    m_statsTotalValueLabel->setText(
        QString::number(static_cast<qulonglong>(m_session.dataSet->size()))
        );
    m_statsFilteredValueLabel->setText(
        QString::number(static_cast<qulonglong>(filtered.size()))
        );

    if (!m_session.dataSet.has_value() || m_session.dataSet->empty()) {
        m_statsSensorValueLabel->setText("-");
    } else if (!settings.useSensor) {
        m_statsSensorValueLabel->setText("All");
    } else {
        m_statsSensorValueLabel->setText(settings.sensor);
    }

    if (settings.useFrom && settings.from.has_value()) {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                              settings.from->time_since_epoch()
                              ).count();

        m_statsFromValueLabel->setText(
            QDateTime::fromSecsSinceEpoch(static_cast<qint64>(secs)).toString("yyyy-MM-dd HH:mm:ss")
            );
    } else {
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
        QString::number(static_cast<qulonglong>(summary.count))
        );

    m_statsMinValueLabel->setText(QString::number(stats.min, 'f', 1));
    m_statsMaxValueLabel->setText(QString::number(stats.max, 'f', 1));
    m_statsMeanValueLabel->setText(QString::number(stats.mean, 'f', 2));
    m_statsStddevValueLabel->setText(QString::number(stats.stddev, 'f', 2));
    m_statsZThresholdValueLabel->setText(QString::number(settings.zThreshold, 'f', 2));
}

void CsvAnalysisTab::resetAlertsPanel()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

void CsvAnalysisTab::updateAlertsPanel(const pdt::DataSet& filtered)
{
    resetAlertsPanel();

    if (!m_session.dataSet.has_value()) {
        return;
    }

    if (hasInvalidTimeRange()) {
        m_alertsListWidget->clear();
        m_alertsListWidget->addItem("Invalid time range: From is later than To");
        return;
    }

    const auto settings = currentSettings();
    const auto summary = pdt::detect_zscore_global(filtered, settings.zThreshold, settings.topN);

    m_alertsListWidget->clear();

    if (filtered.empty()) {
        m_samplesTableView->hide();
        m_dataPlaceholderLabel->setText("No rows match current filters");
        m_dataPlaceholderLabel->show();
        m_alertsListWidget->addItem("No anomalies detected");

    } else if (summary.top.empty()) {
        m_alertsListWidget->addItem("No anomalies detected");

    } else {
        for (const auto& a : summary.top) {
            const auto ts = std::chrono::system_clock::to_time_t(a.timestamp);
            const QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts));
            const QString dateText = dt.date().toString("yyyy-MM-dd");
            const QString timeText = dt.time().toString("HH:mm:ss");

            m_alertsListWidget->addItem(
                QString("%1  %2  |  %3  |  value = %4  |  z = %5")
                    .arg(dateText,
                         timeText,
                         QString::fromStdString(a.sensor),
                         QString::number(a.value, 'f', 1),
                         QString::number(a.zscore, 'f', 2))
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
    connect(m_autoUpdateCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_recomputeButton->setEnabled(!checked);
    });

    connect(m_recomputeButton, &QPushButton::clicked,
            this, &CsvAnalysisTab::recomputeAnalysis);

    auto trigger = [this]() {
        if (m_autoUpdateCheckBox->isChecked()) {
            recomputeAnalysis();
        }
    };

    connect(m_useSensorCheckBox, &QCheckBox::toggled, this, trigger);
    connect(m_sensorComboBox, &QComboBox::currentIndexChanged, this, trigger);
    connect(m_useFromCheckBox, &QCheckBox::toggled, this, trigger);
    connect(m_fromDateEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_fromTimeEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_useToCheckBox, &QCheckBox::toggled, this, trigger);
    connect(m_toDateEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_toTimeEdit, &QDateTimeEdit::dateTimeChanged, this, trigger);
    connect(m_zThresholdSpinBox, &QDoubleSpinBox::valueChanged, this, trigger);
    connect(m_topNSpinBox, &QSpinBox::valueChanged, this, trigger);
    connect(m_showSkippedRowsCheckBox, &QCheckBox::toggled, this, trigger);

    connect(m_useSensorCheckBox, &QCheckBox::toggled, m_sensorComboBox, &QWidget::setEnabled);
    connect(m_useFromCheckBox, &QCheckBox::toggled, m_fromDateEdit, &QWidget::setEnabled);
    connect(m_useFromCheckBox, &QCheckBox::toggled, m_fromTimeEdit, &QWidget::setEnabled);
    connect(m_useToCheckBox, &QCheckBox::toggled, m_toDateEdit, &QWidget::setEnabled);
    connect(m_useToCheckBox, &QCheckBox::toggled, m_toTimeEdit, &QWidget::setEnabled);    
}

void CsvAnalysisTab::displaySessionData(const pdt::DataSet& filtered)
{
    if (!m_session.dataSet.has_value()) {
        return;
    }

    m_csvSamplesModel->setDataSet(filtered);
    m_samplesTableView->show();
    m_dataPlaceholderLabel->hide();

    m_samplesTableView->resizeColumnsToContents();

    const int col = 0;
    m_samplesTableView->setColumnWidth(col, m_samplesTableView->columnWidth(col) + 20);
}

void CsvAnalysisTab::recomputeAnalysis()
{
    if (hasInvalidTimeRange()) {
        if (m_csvSamplesModel != nullptr) {
            m_csvSamplesModel->clear();
        }

        if (m_dataPlaceholderLabel != nullptr) {
            m_dataPlaceholderLabel->setText("Invalid time range");
            m_dataPlaceholderLabel->show();
        }

        if (m_samplesTableView != nullptr) {
            m_samplesTableView->hide();
        }

        resetStatisticsPanel();
        updateAlertsPanel(pdt::DataSet{});
        return;
    }

    const auto filtered = m_session.dataSet->filter(currentFilterOptions());

    displaySessionData(filtered);
    updateStatisticsPanel(filtered);
    updateAlertsPanel(filtered);
}

pdt::FilterOptions CsvAnalysisTab::currentFilterOptions() const
{
    const auto settings = currentSettings();

    pdt::FilterOptions filter;

    if (settings.useSensor) {
        filter.sensor = settings.sensor.toStdString();
    }

    if (settings.useFrom) {
        filter.from = settings.from;
    }

    if (settings.useTo) {
        filter.to = settings.to;
    }

    return filter;
}

CsvAnalysisTab::AnalysisSettings CsvAnalysisTab::currentSettings() const
{
    AnalysisSettings s{};

    s.sensor = m_sensorComboBox->currentText().trimmed();
    s.useSensor = m_useSensorCheckBox->isChecked() && !s.sensor.isEmpty();

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

    s.zThreshold = m_zThresholdSpinBox->value();
    s.topN = static_cast<std::size_t>(m_topNSpinBox->value());

    return s;
}

bool CsvAnalysisTab::hasInvalidTimeRange() const
{
    const auto settings = currentSettings();
    return settings.useFrom && settings.useTo &&
           settings.from.has_value() && settings.to.has_value() &&
           *settings.from > *settings.to;
}

} // namespace pdv
