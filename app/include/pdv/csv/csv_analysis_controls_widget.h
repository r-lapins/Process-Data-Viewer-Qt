#pragma once

#include <QWidget>

#include "pdv/csv/csv_analysis_engine.h"
#include "pdv/core/session_data.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTimeEdit;

namespace pdv {

class CsvAnalysisControlsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CsvAnalysisControlsWidget(const SessionData& session, QWidget* parent = nullptr);

    [[nodiscard]] CsvAnalysisEngine::AnalysisSettings settings() const;

    [[nodiscard]] bool isAutoUpdateEnabled() const noexcept;
    [[nodiscard]] bool isPlotEnabled() const noexcept;
    [[nodiscard]] bool isShowSkippedRowsEnabled() const noexcept;
    [[nodiscard]] bool isExportPerSensorEnabled() const noexcept;

    void setBusy(bool busy);

signals:
    void analysisRequested();
    void exportJsonRequested();
    void showPlotChanged(bool checked);
    void showSkippedRowsChanged(bool checked);
    void exportPlotRequested();

private:
    void createUi();
    void connectControls();

    void initializeFromSession(const SessionData& session);
    void populateSensorOptions(const SessionData& session);
    void initializeDateControls(const SessionData& session);

    void triggerAnalysisIfAutoUpdate();

    void updateAnomalyThresholdControls();
    void applyAnomalyMethodUi(pdt::AnomalyMethod method);
    [[nodiscard]] pdt::AnomalyMethod currentAnomalyMethod() const;

    QCheckBox* m_useSensorCheckBox = nullptr;
    QComboBox* m_sensorComboBox = nullptr;

    QCheckBox* m_useFromCheckBox = nullptr;
    QDateEdit* m_fromDateEdit = nullptr;
    QTimeEdit* m_fromTimeEdit = nullptr;

    QCheckBox* m_useToCheckBox = nullptr;
    QDateEdit* m_toDateEdit = nullptr;
    QTimeEdit* m_toTimeEdit = nullptr;

    QComboBox* m_anomalyMethodComboBox = nullptr;
    QLabel* m_anomalyThresholdLabel = nullptr;
    QDoubleSpinBox* m_anomalyThresholdSpinBox = nullptr;
    QSpinBox* m_topNSpinBox = nullptr;

    QPushButton* m_recomputeButton = nullptr;
    QPushButton* m_showPlotButton = nullptr;
    QPushButton* m_exportJsonButton = nullptr;
    QPushButton* m_exportPlotButton = nullptr;

    QCheckBox* m_autoUpdateCheckBox = nullptr;
    QCheckBox* m_showSkippedRowsCheckBox = nullptr;
    QCheckBox* m_exportPerSensorCheckBox = nullptr;

    // Remembers the threshold last used for each anomaly method,
    // so switching methods in the UI restores the previous value.
    pdt::AnomalyMethod m_currentMethodUi = pdt::AnomalyMethod::ZScore;
    double m_lastZScoreThreshold = 3.0;
    double m_lastIqrThreshold = 1.5;
    double m_lastMadThreshold = 3.5;
};

} // namespace pdv