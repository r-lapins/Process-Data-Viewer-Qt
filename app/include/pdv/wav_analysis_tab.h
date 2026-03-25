#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/wav_analysis_engine.h"

#include <pdt/signal/window.h>
#include <pdt/signal/peak_detection.h>

class QLabel;
class QListWidget;
class QWidget;

namespace pdv {

class SignalChartWidget;
class SpectrumChartWidget;
class WavAnalysisController;
class WavAnalysisControlsWidget;

class WavAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit WavAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    // ui creation
    void createUi();
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);
    QWidget* createSignalPlot(QWidget* parent);
    QWidget* createSpectrumPlot(QWidget* parent);

    // orchestration
    void connectControls();
    void recomputeAnalysis();

    // ui updates
    void renderStatistics(const WavAnalysisEngine::AnalysisResult& result);
    void renderAlerts(const WavAnalysisEngine::AnalysisResult& result);
    void renderSignalPlot(const WavAnalysisEngine::AnalysisResult& result);
    void renderSpectrumPlot(const WavAnalysisEngine::AnalysisResult& result);
    void renderAnalysis(const WavAnalysisEngine::AnalysisResult& result);

    // resets/helpers
    void clearStatistics();
    void clearAlerts();

    void updatePlotVisibility();
    void exportSignalPlotPng();
    void exportSpectrumPlotPng();

    QString toString(WavAnalysisEngine::SpectrumAlgorithm algorithm) const;
    QString toString(pdt::WindowType window) const;
    QString toString(pdt::PeakDetectionMode mode) const;

    QLabel* m_statsFileTypeValueLabel = nullptr;
    QLabel* m_statsSampleRateValueLabel = nullptr;
    QLabel* m_statsChannelsValueLabel = nullptr;
    QLabel* m_statsTotalSamplesValueLabel = nullptr;
    QLabel* m_statsUsedFromValueLabel = nullptr;
    QLabel* m_statsWindowSizeValueLabel = nullptr;
    QLabel* m_statsWindowValueLabel = nullptr;
    QLabel* m_statsAlgorithmValueLabel = nullptr;
    QLabel* m_statsThresholdValueLabel = nullptr;
    QLabel* m_statsPeakModeValueLabel = nullptr;
    QLabel* m_statsDetectedPeaksValueLabel = nullptr;
    QLabel* m_statsMinValueLabel = nullptr;
    QLabel* m_statsMaxValueLabel = nullptr;
    QLabel* m_statsMeanValueLabel = nullptr;
    QLabel* m_statsStddevValueLabel = nullptr;

    QListWidget* m_alertsListWidget = nullptr;

    SignalChartWidget* m_signalChartWidget = nullptr;
    SpectrumChartWidget* m_spectrumChartWidget = nullptr;

    QWidget* m_signalPlotContainer = nullptr;
    QWidget* m_spectrumPlotContainer = nullptr;

    WavAnalysisController* m_controller = nullptr;
    WavAnalysisControlsWidget* m_controlsWidget = nullptr;
};

} // namespace pdv
