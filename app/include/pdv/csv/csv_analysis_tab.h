#pragma once

#include "pdv/core/analysis_tab.h"
#include "pdv/csv/csv_samples_table_model.h"
#include "pdv/csv/csv_analysis_engine.h"
#include "pdv/csv/csv_analysis_controls_widget.h"
#include "pdv/csv/csv_analysis_controller.h"

class QLabel;
class QTableView;
class QGroupBox;
class QWidget;

namespace pdv {

class CsvAnalysisPlotWidget;
class CsvAnalysisResultsPanel;
class CsvAnalysisController;

class CsvAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit CsvAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    struct DataWidgets {
        QLabel* placeholderLabel = nullptr;
        QTableView* tableView = nullptr;
    };

    void createUi();
    QWidget* createDataPanel(QWidget* parent);
    QGroupBox* createPlotPanel(QWidget* parent);

    void connectControls();
    void recomputeAnalysis();
    void exportJsonReport();
    void exportPlotPng();

    void updatePlotVisibility();
    void renderPlot(const CsvAnalysisEngine::AnalysisResult& result);
    void renderData(const CsvAnalysisEngine::AnalysisResult& result);
    void renderResults(const CsvAnalysisEngine::AnalysisResult& result);

    DataWidgets m_data;
    QWidget* m_plotContainer = nullptr;
    CsvAnalysisPlotWidget* m_csvAnalysisPlotWidget = nullptr;
    CsvAnalysisResultsPanel* m_resultsPanel = nullptr;
    CsvAnalysisControlsWidget* m_controlsWidget = nullptr;
    CsvAnalysisController* m_controller = nullptr;
    CsvSamplesTableModel* m_csvSamplesModel = nullptr;
};

} // namespace pdv
