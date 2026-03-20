#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/csv_samples_table_model.h"

class QLabel;
class QListWidget;
class QTableView;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QDateTimeEdit;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QTimeEdit;

namespace pdv {

class CsvAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit CsvAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    struct AnalysisSettings {
        QString sensor;
        bool useSensor{false};
        bool useFrom{false};
        bool useTo{false};
        std::optional<std::chrono::sys_seconds> from;
        std::optional<std::chrono::sys_seconds> to;
        double zThreshold{3.0};
        std::size_t topN{20};
    };

    void createUi();
    QWidget* createDataPanel(QWidget* parent);
    QWidget* createControlsPanel(QWidget* parent);
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);

    void connectControls();

    void displaySessionData();
    void recomputeAnalysis();

    void resetStatisticsPanel();
    void updateStatisticsPanel();

    void resetAlertsPanel();
    void updateAlertsPanel();

    void populateSensorOptions();
    void initializeDateControls();

    bool hasInvalidTimeRange() const;

    AnalysisSettings currentSettings() const;
    pdt::FilterOptions currentFilterOptions() const;

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
    QDoubleSpinBox* m_zThresholdSpinBox = nullptr;
    QSpinBox* m_topNSpinBox = nullptr;
    QCheckBox* m_autoUpdateCheckBox = nullptr;
    QPushButton* m_recomputeButton = nullptr;

    QLabel* m_statsFileTypeValueLabel = nullptr;
    QLabel* m_statsCountValueLabel = nullptr;
    QLabel* m_statsMinValueLabel = nullptr;
    QLabel* m_statsMaxValueLabel = nullptr;
    QLabel* m_statsMeanValueLabel = nullptr;
    QLabel* m_statsStddevValueLabel = nullptr;

    QLabel* m_statsParsedOkValueLabel = nullptr;
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
};

} // namespace pdv
