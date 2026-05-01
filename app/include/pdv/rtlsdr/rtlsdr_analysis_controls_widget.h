#pragma once

#include <QWidget>

#include "pdv/rtlsdr/rtlsdr_analysis_controller.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;

namespace pdv {

class RtlSdrAnalysisControlsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RtlSdrAnalysisControlsWidget(QWidget* parent = nullptr);

    [[nodiscard]] RtlSdrAnalysisSettings settings() const;
    [[nodiscard]] bool isSignalPlotEnabled() const noexcept;
    [[nodiscard]] bool isSpectrumPlotEnabled() const noexcept;

    void setRunning(bool running);
    void refreshDevices();

signals:
    void startRequested();
    void stopRequested();
    void settingsChanged();
    void signalPlotToggled(bool checked);
    void spectrumPlotToggled(bool checked);
    void exportSignalPlotRequested();
    void exportSpectrumPlotRequested();
    void exportSpectrumCsvRequested();

private:
    void createUi();
    void connectControls();
    void applyFrequencyPreset(int index);

    [[nodiscard]] pdt::SpectrumAlgorithm selectedAlgorithm() const noexcept;

    QComboBox* m_deviceComboBox = nullptr;
    QComboBox* m_frequencyPresetComboBox = nullptr;
    QSpinBox* m_frequencySpinBox = nullptr;
    QSpinBox* m_sampleRateSpinBox = nullptr;
    QSpinBox* m_gainSpinBox = nullptr;
    QComboBox* m_blockBytesComboBox = nullptr;
    QSpinBox* m_refreshIntervalSpinBox = nullptr;
    QCheckBox* m_autoGainCheckBox = nullptr;

    QComboBox* m_windowComboBox = nullptr;
    QComboBox* m_algorithmComboBox = nullptr;
    QComboBox* m_peakModeComboBox = nullptr;
    QDoubleSpinBox* m_thresholdSpinBox = nullptr;
    QSpinBox* m_topPeaksSpinBox = nullptr;

    QPushButton* m_refreshDevicesButton = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QPushButton* m_showSignalButton = nullptr;
    QPushButton* m_showSpectrumButton = nullptr;
    QPushButton* m_exportSignalPlotButton = nullptr;
    QPushButton* m_exportSpectrumPlotButton = nullptr;
    QPushButton* m_exportSpectrumCsvButton = nullptr;
    bool m_hasDevices = false;
};

} // namespace pdv
