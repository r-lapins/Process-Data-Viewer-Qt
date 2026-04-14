#pragma once

#include <QWidget>

#include "pdv/core/session_data.h"

#include <pdt/pipeline/wav_analysis_service.h>

class QLabel;
class QListWidget;

namespace pdv {

class WavAnalysisResultsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit WavAnalysisResultsPanel(QWidget* parent = nullptr);

    void clear();
    void setResults(const SessionData& session, const pdt::WavAnalysisResult& result);

private:
    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);

    void clearStatistics();
    void renderStatistics(const SessionData& session, const pdt::WavAnalysisResult& result);

    void clearAlerts();
    void renderAlerts(const SessionData& session, const pdt::WavAnalysisResult& result);

    QString toString(pdt::SpectrumAlgorithm algorithm) const;
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
    QLabel* m_statsTotalTimeValueLabel = nullptr;

    QListWidget* m_alertsListWidget = nullptr;
};

} // namespace pdv