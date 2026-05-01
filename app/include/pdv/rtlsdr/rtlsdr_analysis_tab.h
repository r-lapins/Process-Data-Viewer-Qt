#pragma once

#include "pdv/core/analysis_tab.h"
#include "pdv/rtlsdr/rtlsdr_analysis_controller.h"

#include <optional>

class QWidget;

namespace pdv {

class RtlSdrAnalysisControlsWidget;
class RtlSdrAnalysisResultsPanel;
class SignalChartWidget;
class SpectrumChartWidget;

class RtlSdrAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit RtlSdrAnalysisTab(const SessionData& session, QWidget* parent = nullptr);
    ~RtlSdrAnalysisTab() override;

private:
    void createUi();
    void connectControls();
    void updatePlotVisibility();
    void renderAnalysis(const RtlSdrAnalysisResult& result);
    void renderSignalPlot(const RtlSdrAnalysisResult& result);
    void renderSpectrumPlot(const RtlSdrAnalysisResult& result);
    void exportSignalPlotPng();
    void exportSpectrumPlotPng();
    void exportSpectrumCsv();

    RtlSdrAnalysisController* m_controller = nullptr;
    RtlSdrAnalysisControlsWidget* m_controlsWidget = nullptr;
    RtlSdrAnalysisResultsPanel* m_resultsPanel = nullptr;
    SignalChartWidget* m_signalChartWidget = nullptr;
    SpectrumChartWidget* m_spectrumChartWidget = nullptr;
    QWidget* m_signalPlotContainer = nullptr;
    QWidget* m_spectrumPlotContainer = nullptr;
    std::optional<RtlSdrAnalysisResult> m_lastResult;
};

} // namespace pdv
