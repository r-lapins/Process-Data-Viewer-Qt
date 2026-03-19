#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/csv_samples_table_model.h"

class QLabel;
class QListWidget;
class QTableView;

namespace pdv {

class CsvAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit CsvAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    void createUi();
    void displaySessionData();

    void resetStatisticsPanel();
    void updateStatisticsPanel();

    void resetAlertsPanel();
    void updateAlertsPanel();

    QLabel* m_dataPlaceholderLabel = nullptr;
    QTableView* m_samplesTableView = nullptr;

    QLabel* m_statsFileTypeValueLabel = nullptr;
    QLabel* m_statsCountValueLabel = nullptr;
    QLabel* m_statsMinValueLabel = nullptr;
    QLabel* m_statsMaxValueLabel = nullptr;
    QLabel* m_statsMeanValueLabel = nullptr;
    QLabel* m_statsStddevValueLabel = nullptr;

    QListWidget* m_alertsListWidget = nullptr;

    CsvSamplesTableModel* m_csvSamplesModel = nullptr;
};

} // namespace pdv
