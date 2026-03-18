#pragma once

#include <QMainWindow>
#include <QFutureWatcher>

#include <optional>

class QAction;
class QChartView;
class QLabel;
class QLineSeries;
class QListWidget;
class QTableView;
class QValueAxis;

#include "pdv/file_loader_service.h"
#include "pdv/csv_samples_table_model.h"
#include "pdv/session_data.h"

namespace pdv {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private:
    void createMenu();
    void createCentralWorkspace();
    void openFile();    
    void loadFileAsync(const QString& filePath);
    void handleLoadFinished();
    void setLoadingUiState(bool loading);
    void updateWindowTitle();
    void clearLoadedData();
    void displaySessionData();    

    void createToolbar();
    void openFileFromDataFolder();

    void resetStatisticsPanel();
    void updateStatisticsPanel();

    void resetAlertsPanel();
    void updateAlertsPanel();

    void resetSignalPlot();
    void updateSignalPlot();

    FileLoaderService m_fileLoaderService;
    std::optional<SessionData> m_currentSession;

    QAction* m_openAction = nullptr;
    QAction* m_quickOpenAction = nullptr;
    QFutureWatcher<LoadResult>* m_loadWatcher = nullptr;
    bool m_isLoading = false;

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
