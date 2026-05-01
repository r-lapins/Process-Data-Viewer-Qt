#include "pdv/rtlsdr/rtlsdr_analysis_controls_widget.h"

#include "pdt/compute/make_fft_backend.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>

namespace pdv {
namespace {

const char* kToggleButtonStyle = R"(
    QPushButton {
        padding: 4px 10px;
    }
    QPushButton:checked {
        background-color: #2E7D32;
        color: white;
        border: 1px solid #1B5E20;
    }
)";

QString deviceLabel(const pdt::RtlSdrDeviceInfo& info)
{
    return QString("#%1 %2 %3 [%4]")
        .arg(info.index)
        .arg(QString::fromStdString(info.vendor))
        .arg(QString::fromStdString(info.product))
        .arg(QString::fromStdString(info.serial));
}

} // namespace

RtlSdrAnalysisControlsWidget::RtlSdrAnalysisControlsWidget(QWidget* parent)
    : QWidget(parent)
{
    createUi();
    connectControls();
    refreshDevices();
}

void RtlSdrAnalysisControlsWidget::createUi()
{
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* controlsGroup = new QGroupBox("Device", this);
    auto* controlsLayout = new QFormLayout(controlsGroup);

    m_deviceComboBox = new QComboBox(controlsGroup);
    m_refreshDevicesButton = new QPushButton("Refresh", controlsGroup);

    auto* deviceWidget = new QWidget(controlsGroup);
    auto* deviceLayout = new QHBoxLayout(deviceWidget);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->addWidget(m_deviceComboBox, 1);
    deviceLayout->addWidget(m_refreshDevicesButton);

    m_frequencyPresetComboBox = new QComboBox(controlsGroup);
    m_frequencyPresetComboBox->addItem("Custom", 0);
    m_frequencyPresetComboBox->addItem("FM broadcast 100.0 MHz", 100000000);
    m_frequencyPresetComboBox->addItem("Airband 121.5 MHz", 121500000);
    m_frequencyPresetComboBox->addItem("NOAA 162.4 MHz", 162400000);
    m_frequencyPresetComboBox->addItem("433.92 MHz ISM", 433920000);
    m_frequencyPresetComboBox->addItem("ADS-B 1090 MHz", 1090000000);

    m_frequencySpinBox = new QSpinBox(controlsGroup);
    m_frequencySpinBox->setRange(24000000, 1766000000);
    m_frequencySpinBox->setSingleStep(100000);
    m_frequencySpinBox->setValue(100000000);
    m_frequencySpinBox->setSuffix(" Hz");
    m_frequencySpinBox->setLocale(QLocale(QLocale::Polish, QLocale::Poland));
    m_frequencySpinBox->setGroupSeparatorShown(true);
    m_frequencySpinBox->setKeyboardTracking(false);

    m_sampleRateSpinBox = new QSpinBox(controlsGroup);
    m_sampleRateSpinBox->setRange(225001, 3200000);
    m_sampleRateSpinBox->setSingleStep(240000);
    m_sampleRateSpinBox->setValue(1024000);
    m_sampleRateSpinBox->setSuffix(" Hz");
    m_sampleRateSpinBox->setLocale(QLocale(QLocale::Polish, QLocale::Poland));
    m_sampleRateSpinBox->setGroupSeparatorShown(true);
    m_sampleRateSpinBox->setKeyboardTracking(false);

    m_autoGainCheckBox = new QCheckBox("Auto", controlsGroup);
    m_autoGainCheckBox->setChecked(true);

    m_gainSpinBox = new QSpinBox(controlsGroup);
    m_gainSpinBox->setRange(0, 500);
    m_gainSpinBox->setSingleStep(10);
    m_gainSpinBox->setValue(0);
    m_gainSpinBox->setSuffix(" tenth dB");
    m_gainSpinBox->setEnabled(false);
    m_gainSpinBox->setKeyboardTracking(false);

    auto* gainWidget = new QWidget(controlsGroup);
    auto* gainLayout = new QHBoxLayout(gainWidget);
    gainLayout->setContentsMargins(0, 0, 0, 0);
    gainLayout->addWidget(m_autoGainCheckBox);
    gainLayout->addWidget(m_gainSpinBox, 1);

    m_blockBytesComboBox = new QComboBox(controlsGroup);
    for (int value = 4096; value <= 262144; value *= 2) {
        m_blockBytesComboBox->addItem(QString("%1 bytes").arg(value), value);
    }
    m_blockBytesComboBox->setCurrentIndex(m_blockBytesComboBox->findData(16384));

    m_refreshIntervalSpinBox = new QSpinBox(controlsGroup);
    m_refreshIntervalSpinBox->setRange(50, 2000);
    m_refreshIntervalSpinBox->setSingleStep(50);
    m_refreshIntervalSpinBox->setValue(200);
    m_refreshIntervalSpinBox->setSuffix(" ms");
    m_refreshIntervalSpinBox->setKeyboardTracking(false);

    m_windowComboBox = new QComboBox(controlsGroup);
    m_windowComboBox->addItem("Hann", static_cast<int>(pdt::WindowType::Hann));
    m_windowComboBox->addItem("Hamming", static_cast<int>(pdt::WindowType::Hamming));
    m_windowComboBox->addItem("None", static_cast<int>(pdt::WindowType::None));

    m_algorithmComboBox = new QComboBox(controlsGroup);
    m_algorithmComboBox->addItem("FFT", static_cast<int>(pdt::SpectrumAlgorithm::Fft));
    if (pdt::is_algorithm_available(pdt::SpectrumAlgorithm::cuFft)) {
        m_algorithmComboBox->addItem("cuFFT", static_cast<int>(pdt::SpectrumAlgorithm::cuFft));
    }

    m_peakModeComboBox = new QComboBox(controlsGroup);
    m_peakModeComboBox->addItem("Local maxima", static_cast<int>(pdt::PeakDetectionMode::LocalMaxima));
    m_peakModeComboBox->addItem("Threshold only", static_cast<int>(pdt::PeakDetectionMode::ThresholdOnly));

    m_thresholdSpinBox = new QDoubleSpinBox(controlsGroup);
    m_thresholdSpinBox->setRange(0.0, 1.0);
    m_thresholdSpinBox->setSingleStep(0.05);
    m_thresholdSpinBox->setDecimals(2);
    m_thresholdSpinBox->setValue(0.20);
    m_thresholdSpinBox->setKeyboardTracking(false);

    m_topPeaksSpinBox = new QSpinBox(controlsGroup);
    m_topPeaksSpinBox->setRange(1, 100);
    m_topPeaksSpinBox->setValue(15);
    m_topPeaksSpinBox->setKeyboardTracking(false);

    m_startButton = new QPushButton("Start", controlsGroup);
    m_stopButton = new QPushButton("Stop", controlsGroup);
    m_stopButton->setEnabled(false);

    auto* streamWidget = new QWidget(controlsGroup);
    auto* streamLayout = new QHBoxLayout(streamWidget);
    streamLayout->setContentsMargins(0, 0, 0, 0);
    streamLayout->addWidget(m_startButton);
    streamLayout->addWidget(m_stopButton);

    controlsLayout->addRow("Device:", deviceWidget);
    controlsLayout->addRow("Preset:", m_frequencyPresetComboBox);
    controlsLayout->addRow("Frequency:", m_frequencySpinBox);
    controlsLayout->addRow("Sample rate:", m_sampleRateSpinBox);
    controlsLayout->addRow("Gain:", gainWidget);
    controlsLayout->addRow("Block size:", m_blockBytesComboBox);
    controlsLayout->addRow("Refresh:", m_refreshIntervalSpinBox);
    controlsLayout->addRow(" ", streamWidget);

    auto* sidePanel = new QWidget(this);
    auto* sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(10);

    auto* analysisGroup = new QGroupBox("Analysis", sidePanel);
    auto* analysisLayout = new QFormLayout(analysisGroup);
    analysisLayout->setContentsMargins(0, 0, 0, 0);
    analysisLayout->setLabelAlignment(Qt::AlignLeft);
    analysisLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    analysisLayout->addRow("Algorithm:", m_algorithmComboBox);
    analysisLayout->addRow("Window:", m_windowComboBox);
    analysisLayout->addRow("Peak mode:", m_peakModeComboBox);
    analysisLayout->addRow("Threshold:", m_thresholdSpinBox);
    analysisLayout->addRow("Top peaks:", m_topPeaksSpinBox);

    auto* actionsGroup = new QGroupBox("Actions", sidePanel);
    auto* actionsLayout = new QFormLayout(actionsGroup);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setLabelAlignment(Qt::AlignLeft);
    actionsLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_showSignalButton = new QPushButton("IQ", actionsGroup);
    m_showSpectrumButton = new QPushButton("Spectrum", actionsGroup);
    m_exportSignalPlotButton = new QPushButton("IQ", actionsGroup);
    m_exportSpectrumPlotButton = new QPushButton("Spectrum", actionsGroup);
    m_exportSpectrumCsvButton = new QPushButton("CSV", actionsGroup);

    m_showSignalButton->setCheckable(true);
    m_showSpectrumButton->setCheckable(true);
    m_showSignalButton->setChecked(true);
    m_showSpectrumButton->setChecked(true);
    m_showSignalButton->setStyleSheet(kToggleButtonStyle);
    m_showSpectrumButton->setStyleSheet(kToggleButtonStyle);

    auto* plotsShowWidget = new QWidget(actionsGroup);
    auto* plotsShowLayout = new QHBoxLayout(plotsShowWidget);
    plotsShowLayout->setContentsMargins(0, 0, 0, 0);
    plotsShowLayout->addWidget(m_showSignalButton);
    plotsShowLayout->addWidget(m_showSpectrumButton);

    auto* plotsExportWidget = new QWidget(actionsGroup);
    auto* plotsExportLayout = new QHBoxLayout(plotsExportWidget);
    plotsExportLayout->setContentsMargins(0, 0, 0, 0);
    plotsExportLayout->addWidget(m_exportSignalPlotButton);
    plotsExportLayout->addWidget(m_exportSpectrumPlotButton);

    actionsLayout->addRow(new QLabel("Show plot:", actionsGroup));
    actionsLayout->addRow(" ", plotsShowWidget);
    actionsLayout->addRow(new QLabel("Export plot to PNG:", actionsGroup));
    actionsLayout->addRow(" ", plotsExportWidget);
    actionsLayout->addRow("Output:", m_exportSpectrumCsvButton);

    sideLayout->addWidget(analysisGroup, 0, Qt::AlignTop);
    sideLayout->addWidget(actionsGroup, 0, Qt::AlignTop);
    sideLayout->addStretch();

    rootLayout->addWidget(controlsGroup, 0, Qt::AlignTop);
    rootLayout->addWidget(sidePanel, 0, Qt::AlignTop);
    rootLayout->addStretch();
    rootLayout->addSpacing(10);
}

void RtlSdrAnalysisControlsWidget::connectControls()
{
    const auto emitSettingsChanged = [this]() { emit settingsChanged(); };

    connect(m_refreshDevicesButton, &QPushButton::clicked, this, &RtlSdrAnalysisControlsWidget::refreshDevices);
    connect(m_startButton, &QPushButton::clicked, this, &RtlSdrAnalysisControlsWidget::startRequested);
    connect(m_stopButton, &QPushButton::clicked, this, &RtlSdrAnalysisControlsWidget::stopRequested);
    connect(m_autoGainCheckBox, &QCheckBox::toggled, m_gainSpinBox, &QWidget::setDisabled);
    connect(m_autoGainCheckBox, &QCheckBox::toggled, this, [emitSettingsChanged](bool) { emitSettingsChanged(); });
    connect(m_frequencyPresetComboBox, &QComboBox::currentIndexChanged, this, &RtlSdrAnalysisControlsWidget::applyFrequencyPreset);

    connect(m_deviceComboBox, &QComboBox::currentIndexChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_frequencySpinBox, &QSpinBox::valueChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_sampleRateSpinBox, &QSpinBox::valueChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_gainSpinBox, &QSpinBox::valueChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_blockBytesComboBox, &QComboBox::currentIndexChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_refreshIntervalSpinBox, &QSpinBox::valueChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_algorithmComboBox, &QComboBox::currentIndexChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_windowComboBox, &QComboBox::currentIndexChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_peakModeComboBox, &QComboBox::currentIndexChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });
    connect(m_thresholdSpinBox, &QDoubleSpinBox::valueChanged, this, [emitSettingsChanged](double) { emitSettingsChanged(); });
    connect(m_topPeaksSpinBox, &QSpinBox::valueChanged, this, [emitSettingsChanged](int) { emitSettingsChanged(); });

    connect(m_showSignalButton, &QPushButton::toggled, m_exportSignalPlotButton, &QWidget::setEnabled);
    connect(m_showSpectrumButton, &QPushButton::toggled, m_exportSpectrumPlotButton, &QWidget::setEnabled);
    connect(m_showSignalButton, &QPushButton::toggled, this, &RtlSdrAnalysisControlsWidget::signalPlotToggled);
    connect(m_showSpectrumButton, &QPushButton::toggled, this, &RtlSdrAnalysisControlsWidget::spectrumPlotToggled);

    connect(m_exportSignalPlotButton, &QPushButton::clicked, this, &RtlSdrAnalysisControlsWidget::exportSignalPlotRequested);
    connect(m_exportSpectrumPlotButton, &QPushButton::clicked, this, &RtlSdrAnalysisControlsWidget::exportSpectrumPlotRequested);
    connect(m_exportSpectrumCsvButton, &QPushButton::clicked, this, &RtlSdrAnalysisControlsWidget::exportSpectrumCsvRequested);
}

RtlSdrAnalysisSettings RtlSdrAnalysisControlsWidget::settings() const
{
    RtlSdrAnalysisSettings result{};
    result.device.device_index = static_cast<std::uint32_t>(m_deviceComboBox->currentData().toUInt());
    result.device.center_frequency = static_cast<std::uint32_t>(m_frequencySpinBox->value());
    result.device.sample_rate = static_cast<std::uint32_t>(m_sampleRateSpinBox->value());
    result.device.tuner_gain_tenth_db = m_autoGainCheckBox->isChecked() ? 0 : m_gainSpinBox->value();
    result.device.bias_tee = false;
    result.blockBytes = static_cast<std::size_t>(m_blockBytesComboBox->currentData().toUInt());
    result.refreshIntervalMs = m_refreshIntervalSpinBox->value();
    result.algorithm = selectedAlgorithm();
    result.window = static_cast<pdt::WindowType>(m_windowComboBox->currentData().toInt());
    result.peakMode = static_cast<pdt::PeakDetectionMode>(m_peakModeComboBox->currentData().toInt());
    result.threshold = m_thresholdSpinBox->value();
    result.maxPeaks = static_cast<std::size_t>(m_topPeaksSpinBox->value());
    return result;
}

bool RtlSdrAnalysisControlsWidget::isSignalPlotEnabled() const noexcept
{
    return m_showSignalButton->isChecked();
}

bool RtlSdrAnalysisControlsWidget::isSpectrumPlotEnabled() const noexcept
{
    return m_showSpectrumButton->isChecked();
}

void RtlSdrAnalysisControlsWidget::setRunning(bool running)
{
    m_startButton->setEnabled(!running && m_hasDevices);
    m_stopButton->setEnabled(running);
    m_refreshDevicesButton->setEnabled(!running);
    m_deviceComboBox->setEnabled(!running);
}

void RtlSdrAnalysisControlsWidget::refreshDevices()
{
    QSignalBlocker blocker(*m_deviceComboBox);
    m_deviceComboBox->clear();

    const auto devices = RtlSdrAnalysisController::enumerateDevices();
    for (const auto& device : devices) {
        m_deviceComboBox->addItem(deviceLabel(device), device.index);
    }

    if (m_deviceComboBox->count() == 0) {
        m_deviceComboBox->addItem("No RTL-SDR devices found", 0);
        m_hasDevices = false;
        m_startButton->setEnabled(false);
    } else {
        m_hasDevices = true;
        m_startButton->setEnabled(true);
    }
}

void RtlSdrAnalysisControlsWidget::applyFrequencyPreset(int index)
{
    const int value = m_frequencyPresetComboBox->itemData(index).toInt();
    if (value <= 0) { return; }

    m_frequencySpinBox->setValue(value);
}

pdt::SpectrumAlgorithm RtlSdrAnalysisControlsWidget::selectedAlgorithm() const noexcept
{
    return static_cast<pdt::SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());
}

} // namespace pdv
