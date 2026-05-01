#pragma once

#include <QWidget>

#include "pdv/rtlsdr/rtlsdr_analysis_controller.h"

class QLabel;
class QListWidget;

namespace pdv {

class RtlSdrAnalysisResultsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit RtlSdrAnalysisResultsPanel(QWidget* parent = nullptr);

    void clear();
    void setResults(const RtlSdrAnalysisResult& result);
    void setStatusText(const QString& text);

private:
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createPeaksPanel(QWidget* parent);

    void clearStatistics();
    void renderStatistics(const RtlSdrAnalysisResult& result);
    void renderPeaks(const RtlSdrAnalysisResult& result);

    [[nodiscard]] QString toString(pdt::SpectrumAlgorithm algorithm) const;
    [[nodiscard]] QString toString(pdt::WindowType window) const;
    [[nodiscard]] QString toString(pdt::PeakDetectionMode mode) const;

    QLabel* m_statusValueLabel = nullptr;
    QLabel* m_deviceValueLabel = nullptr;
    QLabel* m_centerFrequencyValueLabel = nullptr;
    QLabel* m_sampleRateValueLabel = nullptr;
    QLabel* m_gainValueLabel = nullptr;
    QLabel* m_biasTeeValueLabel = nullptr;
    QLabel* m_frameValueLabel = nullptr;
    QLabel* m_samplesValueLabel = nullptr;
    QLabel* m_windowValueLabel = nullptr;
    QLabel* m_algorithmValueLabel = nullptr;
    QLabel* m_peakModeValueLabel = nullptr;
    QLabel* m_detectedPeaksValueLabel = nullptr;
    QLabel* m_thresholdValueLabel = nullptr;
    QLabel* m_totalTimeValueLabel = nullptr;
    QListWidget* m_peaksListWidget = nullptr;
};

} // namespace pdv
