#pragma once

#include <QWidget>

#include "pdv/wav_analysis_engine.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;
class QStackedWidget;

namespace pdv {

struct SessionData;

class WavAnalysisControlsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WavAnalysisControlsWidget(const SessionData& session, QWidget* parent = nullptr);

    [[nodiscard]] WavAnalysisEngine::AnalysisSettings settings() const;
    [[nodiscard]] bool isAutoUpdateEnabled() const noexcept;
    [[nodiscard]] bool isSignalPlotEnabled() const noexcept;
    [[nodiscard]] bool isSpectrumPlotEnabled() const noexcept;

    void setBusy(bool busy);

signals:
    void analysisRequested();
    void signalPlotToggled(bool checked);
    void spectrumPlotToggled(bool checked);
    void exportSignalPlotRequested();
    void exportSpectrumPlotRequested();

private:
    void createUi(const SessionData& session);
    void connectControls();

    [[nodiscard]] std::size_t selectedWindowSize() const;
    [[nodiscard]] WavAnalysisEngine::SpectrumAlgorithm selectedAlgorithm() const noexcept;
    [[nodiscard]] bool useWindow() const noexcept;

    void triggerAutoAnalysis();
    void updateWindowSizeInputMode();
    void rebuildFftWindowSizeCombo(const SessionData& session);
    void updateFromSpinRange(const SessionData& session);
    void updateFromSpinStep();

    QSpinBox* m_fromSpinBox = nullptr;
    QSpinBox* m_windowSizeSpinBox = nullptr;
    QSpinBox* m_topPeaksSpinBox = nullptr;

    QComboBox* m_windowComboBox = nullptr;
    QComboBox* m_algorithmComboBox = nullptr;
    QComboBox* m_peakModeComboBox = nullptr;
    QComboBox* m_windowSizeComboBox = nullptr;

    QCheckBox* m_autoUpdateCheckBox = nullptr;
    QPushButton* m_recomputeButton = nullptr;
    QDoubleSpinBox* m_thresholdSpinBox = nullptr;
    QStackedWidget* m_windowSizeInputStack = nullptr;

    QPushButton* m_showSignalButton = nullptr;
    QPushButton* m_showSpectrumButton = nullptr;
    QPushButton* m_exportSignalPlotButton = nullptr;
    QPushButton* m_exportSpectrumPlotButton = nullptr;

    const SessionData* m_session = nullptr;
};

} // namespace pdv