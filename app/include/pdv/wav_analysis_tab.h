#pragma once

#include "pdv/analysis_tab.h"

#include <pdt/signal/window.h>
#include <pdt/signal/peak_detection.h>

#include <vector>

class QLabel;
class QListWidget;
class QPushButton;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QChartView;
class QLineSeries;
class QValueAxis;
class QCheckBox;
class QStackedWidget;

namespace pdv {

class WavAnalysisTab : public AnalysisTab
{
    Q_OBJECT

public:
    explicit WavAnalysisTab(const SessionData& session, QWidget* parent = nullptr);

private:
    enum class SpectrumAlgorithm {
        Dft,
        Fft
    };

    struct AnalysisSettings {
        SpectrumAlgorithm algorithm{SpectrumAlgorithm::Fft};
        bool useWindow{true};
        pdt::WindowType window{pdt::WindowType::Hann};
        pdt::PeakDetectionMode peakMode{pdt::PeakDetectionMode::LocalMaxima};
        double threshold{0.20};
        std::size_t topPeaks{20};
        std::size_t from{0};
        std::size_t bins{1024};
    };

    struct AnalysisResult {
        std::vector<double> rawSegment;
        std::vector<double> processedSegment;
        pdt::Spectrum spectrum;
        std::vector<pdt::Peak> allPeaks;
        std::vector<pdt::Peak> dominantPeaks;
        double minValue{0.0};
        double maxValue{0.0};
        double meanValue{0.0};
        double stddevValue{0.0};
    };

    void createUi();

    QWidget* createStatisticsPanel(QWidget* parent);
    QWidget* createControlsPanel(QWidget* parent);
    QWidget* createAlertsPanel(QWidget* parent);
    QChartView* createSignalPlot(QWidget* parent);

    void connectControls();

    void resetStatisticsPanel();
    void resetAlertsPanel();
    void resetSignalPlot();

    void updateStatisticsPanel(const AnalysisResult& result);
    void updateAlertsPanel(const AnalysisResult& result);
    void updateSignalPlot(const AnalysisResult& result);

    void recomputeAnalysis();

    bool useWindow() const;

    std::size_t selectedBins() const;
    void updateBinsInputMode();
    void rebuildFftBinsCombo();
    void updateFromSpinRange();

    void triggerAutoRecompute();
    void updateFromSpinStep();

    AnalysisSettings currentSettings() const;
    AnalysisResult analyzeCurrentSelection() const;

    std::vector<double> selectedSegment() const;
    SpectrumAlgorithm selectedAlgorithm() const;
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
    QComboBox* m_windowComboBox = nullptr;
    QComboBox* m_algorithmComboBox = nullptr;
    QDoubleSpinBox* m_thresholdSpinBox = nullptr;
    QComboBox* m_peakModeComboBox = nullptr;
    QSpinBox* m_topPeaksSpinBox = nullptr;
    QPushButton* m_recomputeButton = nullptr;

    QListWidget* m_alertsListWidget = nullptr;

    QChartView* m_signalChartView = nullptr;
    QLineSeries* m_signalSeries = nullptr;
    QValueAxis* m_signalAxisX = nullptr;
    QValueAxis* m_signalAxisY = nullptr;

    QCheckBox* m_autoUpdateCheckBox = nullptr;

    QComboBox* m_binsComboBox = nullptr;
    QStackedWidget* m_binsInputStack = nullptr;
};

} // namespace pdv
