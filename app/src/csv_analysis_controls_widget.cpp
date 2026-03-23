#include "pdv/csv_analysis_controls_widget.h"

#include <set>
#include <chrono>

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace pdv {
namespace {

const char* kToggleButtonStyle = R"(
    QPushButton {
        padding: 4px 10px;
    }
    QPushButton:checked {
        background-color: #2E7D32;
        color: white;
        border: 1px solid #1B5E20;
    }
)";

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

CsvAnalysisControlsWidget::CsvAnalysisControlsWidget(const SessionData& session, QWidget* parent)
    : QWidget(parent)
{
    createUi();
    initializeFromSession(session);
    connectControls();

    m_currentMethodUi = currentAnomalyMethod();
    applyAnomalyMethodUi(m_currentMethodUi);

    m_recomputeButton->setEnabled(!m_autoUpdateCheckBox->isChecked());
}

void CsvAnalysisControlsWidget::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(10);

    // Controls
    auto* controlsGroup = new QGroupBox("Controls", this);
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
    // fromLayout->setSpacing(6);
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
    // toLayout->setSpacing(6);
    toLayout->addWidget(m_toDateEdit);
    toLayout->addWidget(m_toTimeEdit);

    m_anomalyMethodComboBox = new QComboBox(controlsGroup);
    m_anomalyMethodComboBox->addItem("Z-score");
    m_anomalyMethodComboBox->addItem("IQR");
    m_anomalyMethodComboBox->addItem("MAD");

    m_anomalyThresholdLabel = new QLabel("Threshold:", controlsGroup);

    m_anomalyThresholdSpinBox = new QDoubleSpinBox(controlsGroup);
    m_anomalyThresholdSpinBox->setDecimals(2);

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

    // Actions
    auto* actionsGroup = new QGroupBox(this);
    auto* actionsLayout = new QGridLayout(actionsGroup);

    m_recomputeButton = new QPushButton("Recompute", actionsGroup);
    // m_recomputeButton->setEnabled(false);
    m_exportJsonButton = new QPushButton("Export JSON", actionsGroup);

    m_showPlotButton = new QPushButton("Plot", actionsGroup);
    m_showPlotButton->setCheckable(true);
    m_showPlotButton->setChecked(false);
    m_showPlotButton->setStyleSheet(kToggleButtonStyle);

    m_exportPerSensorCheckBox = new QCheckBox("Per-sensor", actionsGroup);
    m_exportPerSensorCheckBox->setChecked(false);

    m_showSkippedRowsCheckBox = new QCheckBox("Show skipped", actionsGroup);
    m_showSkippedRowsCheckBox->setChecked(false);

    m_autoUpdateCheckBox = new QCheckBox("Auto update", actionsGroup);
    m_autoUpdateCheckBox->setChecked(true);

    actionsLayout->addWidget(m_recomputeButton, 0, 0);
    actionsLayout->addWidget(m_showPlotButton, 0, 1);
    actionsLayout->addWidget(m_exportJsonButton, 0, 2);

    actionsLayout->addWidget(m_autoUpdateCheckBox, 1, 0);
    actionsLayout->addWidget(m_showSkippedRowsCheckBox, 1, 1);
    actionsLayout->addWidget(m_exportPerSensorCheckBox, 1, 2);

    // Root
    rootLayout->addWidget(controlsGroup, 0, Qt::AlignTop);
    rootLayout->addWidget(actionsGroup, 0, Qt::AlignTop);
    rootLayout->addStretch();
}

void CsvAnalysisControlsWidget::connectControls()
{
    connect(m_recomputeButton, &QPushButton::clicked, this, &CsvAnalysisControlsWidget::analysisRequested);
    connect(m_exportJsonButton, &QPushButton::clicked, this, &CsvAnalysisControlsWidget::exportJsonRequested);
    connect(m_showPlotButton, &QPushButton::toggled, this, &CsvAnalysisControlsWidget::showPlotChanged);
    connect(m_showSkippedRowsCheckBox, &QCheckBox::toggled, this, &CsvAnalysisControlsWidget::showSkippedRowsChanged);

    connect(m_autoUpdateCheckBox, &QCheckBox::toggled, this, [this](bool checked) { m_recomputeButton->setEnabled(!checked); });

    connect(m_useSensorCheckBox, &QCheckBox::toggled, m_sensorComboBox, &QWidget::setEnabled);
    connect(m_useFromCheckBox, &QCheckBox::toggled, m_fromDateEdit, &QWidget::setEnabled);
    connect(m_useFromCheckBox, &QCheckBox::toggled, m_fromTimeEdit, &QWidget::setEnabled);
    connect(m_useToCheckBox, &QCheckBox::toggled, m_toDateEdit, &QWidget::setEnabled);
    connect(m_useToCheckBox, &QCheckBox::toggled, m_toTimeEdit, &QWidget::setEnabled);

    connect(m_anomalyMethodComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        updateAnomalyThresholdControls();
        triggerAnalysisIfAutoUpdate();
    });

    connect(m_sensorComboBox, &QComboBox::currentIndexChanged, this, [this](int) { triggerAnalysisIfAutoUpdate(); });
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, [this](const QDate&) { triggerAnalysisIfAutoUpdate(); });
    connect(m_fromTimeEdit, &QTimeEdit::timeChanged, this, [this](const QTime&) { triggerAnalysisIfAutoUpdate(); });
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, [this](const QDate&) { triggerAnalysisIfAutoUpdate(); });
    connect(m_toTimeEdit, &QTimeEdit::timeChanged, this, [this](const QTime&) { triggerAnalysisIfAutoUpdate(); });
    connect(m_topNSpinBox, &QSpinBox::valueChanged, this, [this](int) { triggerAnalysisIfAutoUpdate(); });
    connect(m_anomalyThresholdSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double) { triggerAnalysisIfAutoUpdate(); });

    connect(m_useSensorCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && m_exportPerSensorCheckBox->isChecked()) {
            m_exportPerSensorCheckBox->setChecked(false);
        }
        triggerAnalysisIfAutoUpdate();
    });

    connect(m_useFromCheckBox, &QCheckBox::toggled, this, [this](bool) { triggerAnalysisIfAutoUpdate(); });
    connect(m_useToCheckBox, &QCheckBox::toggled, this, [this](bool) { triggerAnalysisIfAutoUpdate(); });

    connect(m_exportPerSensorCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) { return; }

        if (m_useSensorCheckBox->isChecked()) {
            m_useSensorCheckBox->setChecked(false);

            QMessageBox::information(this, "Per-sensor export",
                "Sensor filter has been disabled because per-sensor export is selected."
                );
        }
    });
}

void CsvAnalysisControlsWidget::initializeFromSession(const SessionData& session)
{
    populateSensorOptions(session);
    initializeDateControls(session);
}

void CsvAnalysisControlsWidget::populateSensorOptions(const SessionData& session)
{
    m_sensorComboBox->clear();

    if (!session.dataSet.has_value()) {
        m_useSensorCheckBox->setEnabled(false);
        m_sensorComboBox->setEnabled(false);
        return;
    }

    std::set<QString> sensors;
    for (const auto& sample : session.dataSet->samples()) {
        sensors.insert(QString::fromStdString(sample.sensor));
    }

    for (const auto& sensor : sensors) {
        m_sensorComboBox->addItem(sensor);
    }

    const bool hasSensors = (m_sensorComboBox->count() > 0);
    m_useSensorCheckBox->setEnabled(hasSensors);
    m_sensorComboBox->setEnabled(hasSensors && m_useSensorCheckBox->isChecked());
}

void CsvAnalysisControlsWidget::initializeDateControls(const SessionData& session)
{
    if (!session.dataSet.has_value() || session.dataSet->empty()) {
        m_useFromCheckBox->setEnabled(false);
        m_useToCheckBox->setEnabled(false);
        m_fromDateEdit->setEnabled(false);
        m_fromTimeEdit->setEnabled(false);
        m_toDateEdit->setEnabled(false);
        m_toTimeEdit->setEnabled(false);
        return;
    }

    const auto& samples = session.dataSet->samples();
    if (samples.empty()) {
        m_useFromCheckBox->setEnabled(false);
        m_useToCheckBox->setEnabled(false);
        return;
    }

    auto minTs = samples.front().timestamp;
    auto maxTs = samples.front().timestamp;

    for (const auto& sample : samples) {
        if (sample.timestamp < minTs) { minTs = sample.timestamp; }
        if (sample.timestamp > maxTs) { maxTs = sample.timestamp; }
    }

    const auto minSecs =
        std::chrono::duration_cast<std::chrono::seconds>(minTs.time_since_epoch()).count();
    const auto maxSecs =
        std::chrono::duration_cast<std::chrono::seconds>(maxTs.time_since_epoch()).count();

    const QDateTime minDt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(minSecs));
    const QDateTime maxDt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(maxSecs));

    m_useFromCheckBox->setEnabled(true);
    m_useToCheckBox->setEnabled(true);

    m_fromDateEdit->setMinimumDate(minDt.date());
    m_fromDateEdit->setMaximumDate(maxDt.date());
    m_toDateEdit->setMinimumDate(minDt.date());
    m_toDateEdit->setMaximumDate(maxDt.date());

    m_fromDateEdit->setDate(minDt.date());
    m_toDateEdit->setDate(maxDt.date());

    m_fromTimeEdit->setTime(QTime(0, 0, 0));
    m_toTimeEdit->setTime(QTime(23, 59, 59));

    m_fromDateEdit->setEnabled(m_useFromCheckBox->isChecked());
    m_fromTimeEdit->setEnabled(m_useFromCheckBox->isChecked());
    m_toDateEdit->setEnabled(m_useToCheckBox->isChecked());
    m_toTimeEdit->setEnabled(m_useToCheckBox->isChecked());
}

void CsvAnalysisControlsWidget::triggerAnalysisIfAutoUpdate()
{
    if (m_autoUpdateCheckBox->isChecked()) {
        emit analysisRequested();
    }
}

CsvAnalysisEngine::AnalysisSettings CsvAnalysisControlsWidget::settings() const
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

bool CsvAnalysisControlsWidget::autoUpdateEnabled() const noexcept
{
    return m_autoUpdateCheckBox->isChecked();
}

bool CsvAnalysisControlsWidget::showPlotEnabled() const noexcept
{
    return m_showPlotButton->isChecked();
}

bool CsvAnalysisControlsWidget::showSkippedRowsEnabled() const noexcept
{
    return m_showSkippedRowsCheckBox->isChecked();
}

bool CsvAnalysisControlsWidget::exportPerSensorEnabled() const noexcept
{
    return m_exportPerSensorCheckBox->isChecked();
}

void CsvAnalysisControlsWidget::updateAnomalyThresholdControls()
{
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

void CsvAnalysisControlsWidget::applyAnomalyMethodUi(CsvAnalysisEngine::AnomalyMethod method)
{
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

CsvAnalysisEngine::AnomalyMethod CsvAnalysisControlsWidget::currentAnomalyMethod() const
{
    using enum CsvAnalysisEngine::AnomalyMethod;

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

} // namespace pdv