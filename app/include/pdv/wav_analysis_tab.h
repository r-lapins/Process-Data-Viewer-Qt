#pragma once

#include "pdv/analysis_tab.h"
#include "pdv/wav_analysis_engine.h"

#include <pdt/signal/window.h>
#include <pdt/signal/peak_detection.h>
#include <pdt/signal/spectrum_output.h>

class QLabel;
class QListWidget;
class QWidget;

namespace pdv {

class SignalChartWidget;
class SpectrumChartWidget;
class WavAnalysisController;
class WavAnalysisControlsWidget;
class WavAnalysisResultsPanel;

class WavAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit WavAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    // ui creation
    void createUi();
    QWidget* createSignalPlot(QWidget* parent);
    QWidget* createSpectrumPlot(QWidget* parent);

    // orchestration
    void connectControls();
    void recomputeAnalysis();

    // ui updates
    void updatePlotVisibility();
    void renderSignalPlot(const WavAnalysisEngine::AnalysisResult& result);
    void renderSpectrumPlot(const WavAnalysisEngine::AnalysisResult& result);
    void renderAnalysis(const WavAnalysisEngine::AnalysisResult& result);

    void exportSignalPlotPng();
    void exportSpectrumPlotPng();
    void exportSpectrumCsv();
    void exportSpectrumReport();

    SignalChartWidget* m_signalChartWidget = nullptr;
    SpectrumChartWidget* m_spectrumChartWidget = nullptr;

    QWidget* m_signalPlotContainer = nullptr;
    QWidget* m_spectrumPlotContainer = nullptr;

    WavAnalysisController* m_controller = nullptr;
    WavAnalysisControlsWidget* m_controlsWidget = nullptr;
    WavAnalysisResultsPanel* m_resultsPanel = nullptr;
};

} // namespace pdv
