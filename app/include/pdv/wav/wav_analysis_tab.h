#pragma once

#include "pdv/core/analysis_tab.h"

#include <pdt/pipeline/wav_analysis_session.h>
#include <pdt/pipeline/wav_analysis_service.h>

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
    void renderSignalPlot(const pdt::WavAnalysisResult& result);
    void renderSpectrumPlot(const pdt::WavAnalysisResult& result);
    void renderAnalysis(const pdt::WavAnalysisResult& result);

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
