#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/wav_analysis_engine.h"

class QLabel;
class QListWidget;
class QPushButton;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QStackedWidget;
class QSplitter;

namespace pdv {

class SignalChartWidget;
class SpectrumChartWidget;

using AnalysisResult = WavAnalysisEngine::AnalysisResult;
using AnalysisSettings = WavAnalysisEngine::AnalysisSettings;
using SpectrumAlgorithm = WavAnalysisEngine::SpectrumAlgorithm;

class WavAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit WavAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    // ui creation
    void createUi();
    QWidget* createControlsPanel(QWidget* parent);
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);
    QWidget* createSignalPlot(QWidget* parent);
    QWidget* createSpectrumPlot(QWidget* parent);

    // orchestration
    void connectControls();
    void recomputeAnalysis();
    void triggerAutoRecompute();

    // analysis
    std::size_t selectedBins() const;
    SpectrumAlgorithm selectedAlgorithm() const;
    AnalysisSettings currentSettings() const;
    bool useWindow() const;

    // ui updates
    void updateStatisticsPanel(const AnalysisResult& result);
    void updateAlertsPanel(const AnalysisResult& result);
    void updateSignalPlot(const AnalysisResult& result);
    void updateSpectrumPlot(const AnalysisResult& result);

    // resets/helpers
    void resetStatisticsPanel();
    void resetAlertsPanel();
    void updateBinsInputMode();
    void rebuildFftBinsCombo();
    void updateFromSpinRange();
    void updateFromSpinStep();

    void updatePlotVisibility();
    QWidget* createPlotVisibilityToolbar(QWidget* parent);

    QString toString(SpectrumAlgorithm algorithm) const;
    QString toString(pdt::WindowType window) const;
    QString toString(pdt::PeakDetectionMode mode) const;

    QLabel* m_statsFileTypeValueLabel = nullptr;
    QLabel* m_statsSampleRateValueLabel = nullptr;
    QLabel* m_statsChannelsValueLabel = nullptr;
    QLabel* m_statsTotalSamplesValueLabel = nullptr;
    QLabel* m_statsUsedFromValueLabel = nullptr;
    QLabel* m_statsUsedBinsValueLabel = nullptr;
    QLabel* m_statsWindowValueLabel = nullptr;
    QLabel* m_statsAlgorithmValueLabel = nullptr;
    QLabel* m_statsThresholdValueLabel = nullptr;
    QLabel* m_statsPeakModeValueLabel = nullptr;
    QLabel* m_statsDetectedPeaksValueLabel = nullptr;
    QLabel* m_statsMinValueLabel = nullptr;
    QLabel* m_statsMaxValueLabel = nullptr;
    QLabel* m_statsMeanValueLabel = nullptr;
    QLabel* m_statsStddevValueLabel = nullptr;

    QSpinBox* m_fromSpinBox = nullptr;
    QSpinBox* m_binsSpinBox = nullptr;
    QSpinBox* m_topPeaksSpinBox = nullptr;

    QComboBox* m_windowComboBox = nullptr;
    QComboBox* m_algorithmComboBox = nullptr;
    QComboBox* m_peakModeComboBox = nullptr;
    QComboBox* m_binsComboBox = nullptr;

    QCheckBox* m_autoUpdateCheckBox = nullptr;
    QPushButton* m_recomputeButton = nullptr;
    QListWidget* m_alertsListWidget = nullptr;
    QDoubleSpinBox* m_thresholdSpinBox = nullptr;
    QStackedWidget* m_binsInputStack = nullptr;

    SignalChartWidget* m_signalChartWidget = nullptr;
    SpectrumChartWidget* m_spectrumChartWidget = nullptr;

    QPushButton* m_showSignalButton = nullptr;
    QPushButton* m_showSpectrumButton = nullptr;

    QWidget* m_signalPlotContainer = nullptr;
    QWidget* m_spectrumPlotContainer = nullptr;
    QSplitter* m_plotsSplitter = nullptr;
};

} // namespace pdv
