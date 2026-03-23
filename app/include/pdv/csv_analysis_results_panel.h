#pragma once

#include "pdv/csv_analysis_engine.h"
#include "pdv/session_data.h"

#include <QWidget>

class QLabel;
class QListWidget;

namespace pdv {

class CsvAnalysisResultsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CsvAnalysisResultsPanel(QWidget* parent = nullptr);

    void clear();
    void setResults(const SessionData& session, const CsvAnalysisEngine::AnalysisResult& result, bool showSkippedRows);

private:
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);

    void clearStatistics();
    void renderStatistics(const SessionData& session, const CsvAnalysisEngine::AnalysisResult& result);

    void clearAlerts();
    void renderAlerts(const SessionData& session, const CsvAnalysisEngine::AnalysisResult& result, bool showSkippedRows);

    QLabel* m_statsFileTypeValueLabel = nullptr;
    QLabel* m_statsMinValueLabel = nullptr;
    QLabel* m_statsMaxValueLabel = nullptr;
    QLabel* m_statsMeanValueLabel = nullptr;
    QLabel* m_statsStddevValueLabel = nullptr;

    QLabel* m_statsSkippedValueLabel = nullptr;
    QLabel* m_statsTotalValueLabel = nullptr;
    QLabel* m_statsFilteredValueLabel = nullptr;
    QLabel* m_statsSensorValueLabel = nullptr;
    QLabel* m_statsFromValueLabel = nullptr;
    QLabel* m_statsToValueLabel = nullptr;
    QLabel* m_statsDetectedAnomaliesValueLabel = nullptr;
    QLabel* m_statsAnomalyMethodValueLabel = nullptr;
    QLabel* m_statsAnomalyThresholdLabel = nullptr;
    QLabel* m_statsAnomalyThresholdValueLabel = nullptr;

    QListWidget* m_alertsListWidget = nullptr;
};

} // namespace pdv