#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/wav_analysis_engine.h"
#include "pdv/wav_analysis_controls_widget.h"

class QLabel;
class QListWidget;

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
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);
    QWidget* createSignalPlot(QWidget* parent);
    QWidget* createSpectrumPlot(QWidget* parent);

    // orchestration
    void connectControls();
    void recomputeAnalysis();

    // ui updates
    void renderStatistics(const AnalysisResult& result);
    void renderAlerts(const AnalysisResult& result);
    void renderSignalPlot(const AnalysisResult& result);
    void renderSpectrumPlot(const AnalysisResult& result);
    void renderAnalysis(const AnalysisResult& result);

    // resets/helpers
    void clearStatistics();
    void clearAlerts();

    void updatePlotVisibility();

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

    QListWidget* m_alertsListWidget = nullptr;

    SignalChartWidget* m_signalChartWidget = nullptr;
    SpectrumChartWidget* m_spectrumChartWidget = nullptr;

    QWidget* m_signalPlotContainer = nullptr;
    QWidget* m_spectrumPlotContainer = nullptr;

    WavAnalysisControlsWidget* m_controlsWidget = nullptr;
};

} // namespace pdv
