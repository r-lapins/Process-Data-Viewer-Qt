#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/csv_samples_table_model.h"
#include "pdv/csv_analysis_engine.h"

#include <optional>

class QLabel;
class QTableView;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QTimeEdit;
class QGroupBox;

namespace pdv {

class CsvPlotWidget;
class CsvResultsPanel;

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

    struct FilterWidgets {
        QCheckBox* useSensorCheckBox = nullptr;
        QComboBox* sensorComboBox = nullptr;
        QCheckBox* useFromCheckBox = nullptr;
        QDateEdit* fromDateEdit = nullptr;
        QTimeEdit* fromTimeEdit = nullptr;
        QCheckBox* useToCheckBox = nullptr;
        QDateEdit* toDateEdit = nullptr;
        QTimeEdit* toTimeEdit = nullptr;
    };

    struct AnalysisWidgets {
        QComboBox* anomalyMethodComboBox = nullptr;
        QLabel* anomalyThresholdLabel = nullptr;
        QDoubleSpinBox* anomalyThresholdSpinBox = nullptr;
        QSpinBox* topNSpinBox = nullptr;
    };

    struct ActionWidgets {
        QPushButton* recomputeButton = nullptr;
        QPushButton* showPlotButton = nullptr;
        QPushButton* exportJsonButton = nullptr;
        QCheckBox* autoUpdateCheckBox = nullptr;
        QCheckBox* showSkippedRowsCheckBox = nullptr;
        QCheckBox* exportPerSensorCheckBox = nullptr;
    };

    void createUi();
    QWidget* createDataPanel(QWidget* parent);
    QWidget* createControlsPanel(QWidget* parent);
    QWidget* createActionsPanel(QWidget* parent);
    QGroupBox* createPlotPanel(QWidget* parent);

    void connectControls();
    void displaySessionData(const pdt::DataSet& filtered);
    void recomputeAnalysis();
    void exportJsonReport();

    void populateSensorOptions();
    void initializeDateControls();

    void updatePlotVisibility();
    void updatePlotPanel(const CsvAnalysisEngine::AnalysisResult& result);
    void updateDataView(const CsvAnalysisEngine::AnalysisResult& result);

    [[nodiscard]] CsvAnalysisEngine::AnalysisSettings currentSettings() const;

    void updateAnomalyThresholdControls();
    void applyAnomalyMethodUi(CsvAnalysisEngine::AnomalyMethod method);
    [[nodiscard]] CsvAnalysisEngine::AnomalyMethod currentAnomalyMethod() const;

    // Remembers the threshold last used for each anomaly method,
    // so switching methods in the UI restores the previous value.
    CsvAnalysisEngine::AnomalyMethod m_currentMethodUi = CsvAnalysisEngine::AnomalyMethod::ZScore;
    double m_lastZScoreThreshold = 3.0;
    double m_lastIqrThreshold = 1.5;
    double m_lastMadThreshold = 3.5;

    FilterWidgets m_filters;
    AnalysisWidgets m_analysis;
    ActionWidgets m_actions;
    DataWidgets m_data;

    QWidget* m_plotContainer = nullptr;
    CsvPlotWidget* m_csvPlotWidget = nullptr;
    CsvResultsPanel* m_resultsPanel = nullptr;
    CsvSamplesTableModel* m_csvSamplesModel = nullptr;

    std::optional<CsvAnalysisEngine::AnalysisResult> m_lastResult;
};

} // namespace pdv
