#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QTableView;
class QChartView;
class QLineSeries;
class QValueAxis;

#include "pdv/csv_samples_table_model.h"
#include "pdv/session_data.h"

namespace pdv {

class AnalysisTab : public QWidget
{
    Q_OBJECT

public:
    explicit AnalysisTab(const SessionData& session, QWidget* parent = nullptr);

    QString tabTitle() const;

private:
    void createUi();
    void displaySessionData();

    void resetStatisticsPanel();
    void updateStatisticsPanel();

    void resetAlertsPanel();
    void updateAlertsPanel();

    void resetSignalPlot();
    void updateSignalPlot();

    SessionData m_session;

    QLabel* m_dataPlaceholderLabel = nullptr;
    QTableView* m_samplesTableView = nullptr;

    QChartView* m_signalChartView = nullptr;
    QLineSeries* m_signalSeries = nullptr;
    QValueAxis* m_signalAxisX = nullptr;
    QValueAxis* m_signalAxisY = nullptr;

    QLabel* m_statsFileTypeValueLabel = nullptr;
    QLabel* m_statsSampleRateValueLabel = nullptr;
    QLabel* m_statsChannelsValueLabel = nullptr;
    QLabel* m_statsCountValueLabel = nullptr;
    QLabel* m_statsMinValueLabel = nullptr;
    QLabel* m_statsMaxValueLabel = nullptr;
    QLabel* m_statsMeanValueLabel = nullptr;
    QLabel* m_statsStddevValueLabel = nullptr;

    QListWidget* m_alertsListWidget = nullptr;

    CsvSamplesTableModel* m_csvSamplesModel = nullptr;
};

} // namespace pdv
