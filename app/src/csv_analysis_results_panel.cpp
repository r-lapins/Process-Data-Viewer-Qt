#include "pdv/csv_analysis_results_panel.h"

#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QTimeZone>

namespace pdv {
namespace {

QString anomalyMethodToString(CsvAnalysisEngine::AnomalyMethod method)
{
    using enum CsvAnalysisEngine::AnomalyMethod;

    switch (method) {
    case ZScore: return "Z-score";
    case IQR:    return "IQR";
    case MAD:    return "MAD";
    }

    return "Unknown";
}

QString anomalyScoreLabel(CsvAnalysisEngine::AnomalyMethod method)
{
    using enum CsvAnalysisEngine::AnomalyMethod;

    switch (method) {
    case ZScore: return "z";
    case IQR:    return "iqr";
    case MAD:    return "mad";
    }

    return "score";
}

QString anomalyThresholdLabelText(CsvAnalysisEngine::AnomalyMethod method)
{
    using enum CsvAnalysisEngine::AnomalyMethod;

    switch (method) {
    case ZScore: return "Z-threshold:";
    case IQR:    return "IQR multiplier:";
    case MAD:    return "MAD threshold:";
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

CsvAnalysisResultsPanel::CsvAnalysisResultsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(10);

    auto* statsPanel = createStatisticsPanel(this);
    auto* alertsPanel = createAlertsPanel(this);

    rootLayout->addWidget(statsPanel, 0, Qt::AlignTop);
    rootLayout->addWidget(alertsPanel, 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    clear();
}

void CsvAnalysisResultsPanel::clear()
{
    clearStatistics();
    clearAlerts();
}

void CsvAnalysisResultsPanel::setResults(const SessionData &session, const CsvAnalysisEngine::AnalysisResult &result, bool showSkippedRows)
{
    renderStatistics(session, result);
    renderAlerts(session, result, showSkippedRows);
}

QWidget* CsvAnalysisResultsPanel::createStatisticsPanel(QWidget* parent)
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

    statsGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    statsGroup->setMinimumWidth(270);

    return statsGroup;
}

QWidget* CsvAnalysisResultsPanel::createAlertsPanel(QWidget* parent)
{
    auto* alertsGroup = new QGroupBox("Alerts", parent);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    alertsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return alertsGroup;
}

void CsvAnalysisResultsPanel::clearStatistics()
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

void CsvAnalysisResultsPanel::renderStatistics(const SessionData& session, const CsvAnalysisEngine::AnalysisResult& result)
{
    clearStatistics();

    if (!session.dataSet.has_value() || session.dataSet->empty()) {
        return;
    }

    const auto& settings = result.usedSettings;

    m_statsFileTypeValueLabel->setText("CSV");
    m_statsSkippedValueLabel->setText(QString::number(static_cast<qulonglong>(session.skipped)));
    m_statsTotalValueLabel->setText(QString::number(static_cast<qulonglong>(session.dataSet->size())));
    m_statsFilteredValueLabel->setText(QString::number(static_cast<qulonglong>(result.filteredDataSet.size())));

    m_statsSensorValueLabel->setText( settings.useSensor ? QString::fromStdString(settings.sensor) : "All" );

    if (settings.useFrom && settings.from.has_value()) {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(settings.from->time_since_epoch()).count();
        m_statsFromValueLabel->setText(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(secs), QTimeZone::UTC).toString("yyyy-MM-dd HH:mm:ss"));
    }
    else {
        m_statsFromValueLabel->setText("-");
    }

    if (settings.useTo && settings.to.has_value()) {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(settings.to->time_since_epoch()).count();

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

void CsvAnalysisResultsPanel::clearAlerts()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

void CsvAnalysisResultsPanel::renderAlerts(const SessionData& session, const CsvAnalysisEngine::AnalysisResult& result, bool showSkippedRows)
{
    clearAlerts();

    if (!session.dataSet.has_value()) {
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
            const QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts), QTimeZone::UTC);
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

    if (!session.skippedRows.empty() && showSkippedRows) {
        m_alertsListWidget->addItem("------------ Skipped rows ------------");

        for (const auto& row : session.skippedRows) {
            m_alertsListWidget->addItem(
                QString("line %1: %2")
                    .arg(row.line_number)
                    .arg(QString::fromStdString(row.text))
                );
        }
    }
}


} // namespace pdv