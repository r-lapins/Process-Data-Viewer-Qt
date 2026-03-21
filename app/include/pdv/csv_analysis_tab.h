#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/csv_samples_table_model.h"
#include "pdv/csv_analysis_engine.h"

#include <optional>

class QLabel;
class QListWidget;
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

class CsvAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit CsvAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    void createUi();
    QWidget* createDataPanel(QWidget* parent);
    QWidget* createControlsPanel(QWidget* parent);
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);
    QGroupBox* createPlotPanel(QWidget* parent);

    void connectControls();

    void displaySessionData(const pdt::DataSet& filtered);
    void recomputeAnalysis();

    void resetStatisticsPanel();
    void updateStatisticsPanel(const CsvAnalysisEngine::AnalysisResult& result);

    void resetAlertsPanel();
    void updateAlertsPanel(const CsvAnalysisEngine::AnalysisResult& result);

    void populateSensorOptions();
    void initializeDateControls();

    void updatePlotVisibility();
    void updatePlotPanel(const CsvAnalysisEngine::AnalysisResult& result);

    void updateDataView(const CsvAnalysisEngine::AnalysisResult& result);

    [[nodiscard]] CsvAnalysisEngine::AnalysisSettings currentSettings() const;

    QLabel* m_dataPlaceholderLabel = nullptr;
    QTableView* m_samplesTableView = nullptr;

    QComboBox* m_sensorComboBox = nullptr;
    QDateEdit* m_fromDateEdit = nullptr;
    QTimeEdit* m_fromTimeEdit = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QTimeEdit* m_toTimeEdit = nullptr;
    QCheckBox* m_useSensorCheckBox = nullptr;
    QCheckBox* m_useFromCheckBox = nullptr;
    QCheckBox* m_useToCheckBox = nullptr;
    QCheckBox* m_autoUpdateCheckBox = nullptr;
    QCheckBox* m_showSkippedRowsCheckBox = nullptr;
    QDoubleSpinBox* m_zThresholdSpinBox = nullptr;
    QSpinBox* m_topNSpinBox = nullptr;
    QPushButton* m_recomputeButton = nullptr;
    QPushButton* m_showPlotButton = nullptr;

    QWidget* m_plotContainer = nullptr;
    CsvPlotWidget* m_csvPlotWidget = nullptr;

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
    QLabel* m_statsZThresholdValueLabel = nullptr;

    QListWidget* m_alertsListWidget = nullptr;

    CsvSamplesTableModel* m_csvSamplesModel = nullptr;

    std::optional<CsvAnalysisEngine::AnalysisResult> m_lastResult;
};

} // namespace pdv
