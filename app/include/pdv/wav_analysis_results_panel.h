#pragma once

#include <QWidget>

#include "pdv/session_data.h"
#include "pdv/wav_analysis_engine.h"
#include <pdt/signal/spectrum_output.h>

class QLabel;
class QListWidget;

namespace pdv {

class WavAnalysisResultsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit WavAnalysisResultsPanel(QWidget* parent = nullptr);

    void clear();
    void setResults(const SessionData& session, const WavAnalysisEngine::AnalysisResult& result);

private:
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);

    void clearStatistics();
    void renderStatistics(const SessionData& session, const WavAnalysisEngine::AnalysisResult& result);

    void clearAlerts();
    void renderAlerts(const SessionData& session, const WavAnalysisEngine::AnalysisResult& result);

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
};

} // namespace pdv