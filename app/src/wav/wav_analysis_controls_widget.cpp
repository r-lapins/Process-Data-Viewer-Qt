#include "pdv/wav/wav_analysis_controls_widget.h"
#include "pdv/core/session_data.h"

#include <algorithm>
#include <limits>
#include <vector>

#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace pdv {
namespace {

constexpr auto kRecomputeLabel = "Recompute";
constexpr auto kBusyLabel = "Wait for it.";

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

const char* kToggleButtonStyle2 = R"(
    QPushButton {
        background-color: #FF9800;
        color: black;
        font-weight: bold;
    }
)";

} // namespace

WavAnalysisControlsWidget::WavAnalysisControlsWidget(const SessionData& session, QWidget* parent)
    : QWidget(parent)
    , m_session(&session)
{
    createUi(session);
    connectControls();
}

void WavAnalysisControlsWidget::createUi(const SessionData& session)
{
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Controls
    auto* controlsGroup = new QGroupBox("Controls", this);
    auto* controlsLayout = new QFormLayout(controlsGroup);

    m_fromSpinBox = new QSpinBox(controlsGroup);

    m_windowSizeSpinBox = new QSpinBox(controlsGroup);
    m_windowSizeComboBox = new QComboBox(controlsGroup);
    m_windowSizeInputStack = new QStackedWidget(controlsGroup);

    m_windowComboBox = new QComboBox(controlsGroup);
    m_algorithmComboBox = new QComboBox(controlsGroup);
    m_thresholdSpinBox = new QDoubleSpinBox(controlsGroup);
    m_peakModeComboBox = new QComboBox(controlsGroup);
    m_topPeaksSpinBox = new QSpinBox(controlsGroup);

    int sampleCount = 0;
    if (session.wavData.has_value()) {
        sampleCount = static_cast<int>(session.wavData->samples.size());
    }

    m_fromSpinBox->setRange(0, std::max(0, sampleCount > 0 ? sampleCount - 1 : 0));
    m_fromSpinBox->setValue(0);

    m_windowSizeSpinBox->setRange(1, std::max(1, sampleCount));
    m_windowSizeSpinBox->setValue(std::min(1024, std::max(1, sampleCount)));

    m_windowSizeInputStack->addWidget(m_windowSizeSpinBox);
    m_windowSizeInputStack->addWidget(m_windowSizeComboBox);
    m_windowSizeInputStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_windowComboBox->addItem("Hann", static_cast<int>(pdt::WindowType::Hann));
    m_windowComboBox->addItem("Hamming", static_cast<int>(pdt::WindowType::Hamming));
    m_windowComboBox->addItem("None", -1);

    m_algorithmComboBox->addItem("FFT", static_cast<int>(WavAnalysisEngine::SpectrumAlgorithm::Fft));
    m_algorithmComboBox->addItem("DFT", static_cast<int>(WavAnalysisEngine::SpectrumAlgorithm::Dft));

    m_thresholdSpinBox->setRange(0.0, 1.0);
    m_thresholdSpinBox->setSingleStep(0.05);
    m_thresholdSpinBox->setDecimals(2);
    m_thresholdSpinBox->setValue(0.20);

    m_peakModeComboBox->addItem("Local maxima", static_cast<int>(pdt::PeakDetectionMode::LocalMaxima));
    m_peakModeComboBox->addItem("Threshold only", static_cast<int>(pdt::PeakDetectionMode::ThresholdOnly));

    m_topPeaksSpinBox->setRange(1, 100);
    m_topPeaksSpinBox->setValue(20);

    controlsLayout->addRow("Algorithm:", m_algorithmComboBox);
    controlsLayout->addRow("Window:", m_windowComboBox);
    controlsLayout->addRow("Peak mode:", m_peakModeComboBox);
    controlsLayout->addRow("Threshold:", m_thresholdSpinBox);
    controlsLayout->addRow("Top peaks:", m_topPeaksSpinBox);
    controlsLayout->addRow("Window size:", m_windowSizeInputStack);
    controlsLayout->addRow("From sample:", m_fromSpinBox);

    // Actions
    auto* actionsGroup = new QGroupBox(this);
    auto* actionsLayout = new QGridLayout(actionsGroup);
    actionsLayout->setContentsMargins(0, 0, 0, 0);

    m_autoUpdateCheckBox = new QCheckBox("Auto update", actionsGroup);
    m_recomputeButton = new QPushButton("Recompute", actionsGroup);

    m_autoUpdateCheckBox->setChecked(true);
    m_recomputeButton->setEnabled(!m_autoUpdateCheckBox->isChecked());

    // Signal / Spectrum
    m_showSignalButton = new QPushButton("Signal", actionsGroup);
    m_showSpectrumButton = new QPushButton("Spectrum", actionsGroup);

    m_exportSignalPlotButton = new QPushButton("Signal", actionsGroup);
    m_exportSpectrumPlotButton = new QPushButton("Spectrum", actionsGroup);

    m_showSignalButton->setCheckable(true);
    m_showSignalButton->setChecked(false);
    m_showSignalButton->setStyleSheet(kToggleButtonStyle);

    m_showSpectrumButton->setCheckable(true);
    m_showSpectrumButton->setChecked(false);
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

    // export csv / report
    m_exportSpectrumCsvButton = new QPushButton("CSV", actionsGroup);
    m_exportSpectrumReportButton = new QPushButton("Report", actionsGroup);

    auto* outputWidget = new QWidget(actionsGroup);
    auto* outputLayout = new QHBoxLayout(outputWidget);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->addWidget(m_exportSpectrumReportButton);
    outputLayout->addWidget(m_exportSpectrumCsvButton);

    auto* lab_1 = new QLabel("Show plot:", actionsGroup);
    auto* lab_2 = new QLabel("Export plot to PNG:", actionsGroup);
    auto* lab_3 = new QLabel("Output:", actionsGroup);

    // composing
    actionsLayout->addWidget(m_recomputeButton, 0, 0);
    actionsLayout->addWidget(m_autoUpdateCheckBox, 1, 0, Qt::AlignRight);
    actionsLayout->addWidget(lab_1, 2, 0, 1, 2);
    actionsLayout->addWidget(plotsShowWidget, 3, 0, 1, 2);
    actionsLayout->addWidget(lab_2, 4, 0, 1, 2);
    actionsLayout->addWidget(plotsExportWidget, 5, 0, 1, 2);
    actionsLayout->addWidget(lab_3, 6, 0, 1, 2);
    actionsLayout->addWidget(outputWidget, 7, 0, 1, 2);

    rootLayout->addWidget(controlsGroup, 0, Qt::AlignTop);
    rootLayout->addWidget(actionsGroup, 0, Qt::AlignBottom);
    rootLayout->addStretch();
    rootLayout->addSpacing(10);

    rebuildFftWindowSizeCombo(session);
    updateWindowSizeInputMode();
    updateFromSpinRange(session);
    updateFromSpinStep();
}

void WavAnalysisControlsWidget::connectControls()
{
    connect(m_autoUpdateCheckBox, &QCheckBox::toggled, this, [this](bool checked) { m_recomputeButton->setEnabled(!checked); });

    connect(m_recomputeButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::analysisRequested);

    connect(m_algorithmComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        updateWindowSizeInputMode();
        if (m_session != nullptr) { updateFromSpinRange(*m_session); }

        updateFromSpinStep();
        triggerAutoAnalysis();
    });

    connect(m_peakModeComboBox, &QComboBox::currentIndexChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_windowComboBox, &QComboBox::currentIndexChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_thresholdSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double) { triggerAutoAnalysis(); });

    connect(m_windowSizeComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        if (m_session != nullptr) { updateFromSpinRange(*m_session); }

        updateFromSpinStep();
        triggerAutoAnalysis();
    });

    connect(m_topPeaksSpinBox, &QSpinBox::valueChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_fromSpinBox, &QSpinBox::valueChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_windowSizeSpinBox, &QSpinBox::valueChanged, this, [this](int) {
        if (m_session != nullptr) { updateFromSpinRange(*m_session); }
        updateFromSpinStep();
        triggerAutoAnalysis();
    });

    connect(m_showSignalButton, &QPushButton::toggled, this, &WavAnalysisControlsWidget::signalPlotToggled);
    connect(m_showSpectrumButton, &QPushButton::toggled, this, &WavAnalysisControlsWidget::spectrumPlotToggled);

    connect(m_exportSignalPlotButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::exportSignalPlotRequested);
    connect(m_exportSpectrumPlotButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::exportSpectrumPlotRequested);

    connect(m_exportSpectrumCsvButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::exportSpectrumCsvRequested);
    connect(m_exportSpectrumReportButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::exportSpectrumReportRequested);
}

WavAnalysisEngine::AnalysisSettings WavAnalysisControlsWidget::settings() const
{
    // Gather current UI state into analysis settings passed to the engine
    WavAnalysisEngine::AnalysisSettings s{};

    s.algorithm = selectedAlgorithm();
    s.useWindow = useWindow();

    if (s.useWindow) {
        s.window = static_cast<pdt::WindowType>(m_windowComboBox->currentData().toInt());
    }

    s.peakMode = static_cast<pdt::PeakDetectionMode>(m_peakModeComboBox->currentData().toInt());
    s.threshold = m_thresholdSpinBox->value();
    s.topPeaks = static_cast<std::size_t>(m_topPeaksSpinBox->value());
    s.from = static_cast<std::size_t>(m_fromSpinBox->value());
    s.windowSize = selectedWindowSize();

    return s;
}

bool WavAnalysisControlsWidget::isAutoUpdateEnabled() const noexcept
{
    return m_autoUpdateCheckBox->isChecked();
}

bool WavAnalysisControlsWidget::isSignalPlotEnabled() const noexcept
{
    return m_showSignalButton->isChecked();
}

bool WavAnalysisControlsWidget::isSpectrumPlotEnabled() const noexcept
{
    return m_showSpectrumButton->isChecked();
}

void WavAnalysisControlsWidget::setBusy(bool busy)
{
    if (busy) {
        m_recomputeButton->setText(kBusyLabel);
        m_recomputeButton->setEnabled(false);

        m_recomputeButton->setStyleSheet(kToggleButtonStyle2);
    } else {
        m_recomputeButton->setText(kRecomputeLabel);

        m_recomputeButton->setStyleSheet("");

        // ważne: respect auto-update
        const bool autoUpdate = m_autoUpdateCheckBox->isChecked();
        m_recomputeButton->setEnabled(!autoUpdate);
    }
}

std::size_t WavAnalysisControlsWidget::selectedWindowSize() const
{
    const auto selected = static_cast<WavAnalysisEngine::SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());

    if (selected == WavAnalysisEngine::SpectrumAlgorithm::Fft) {
        return static_cast<std::size_t>(m_windowSizeComboBox->currentData().toULongLong());
    }

    return static_cast<std::size_t>(m_windowSizeSpinBox->value());
}

WavAnalysisEngine::SpectrumAlgorithm WavAnalysisControlsWidget::selectedAlgorithm() const noexcept
{
    return static_cast<WavAnalysisEngine::SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());
}

bool WavAnalysisControlsWidget::useWindow() const noexcept
{
    return m_windowComboBox->currentData().toInt() != -1;
}

void WavAnalysisControlsWidget::triggerAutoAnalysis()
{
    if (m_autoUpdateCheckBox->isChecked()) {
        emit analysisRequested();
    }
}

void WavAnalysisControlsWidget::updateWindowSizeInputMode()
{
    const auto selected = static_cast<WavAnalysisEngine::SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());

    if (selected == WavAnalysisEngine::SpectrumAlgorithm::Fft) {
        m_windowSizeInputStack->setCurrentWidget(m_windowSizeComboBox);
    } else {
        m_windowSizeInputStack->setCurrentWidget(m_windowSizeSpinBox);
    }
}

void WavAnalysisControlsWidget::rebuildFftWindowSizeCombo(const SessionData& session)
{
    // It generates a list of valid FFT sizes (powers of 2)
    // and selects the best value based on the current UI state.
    const qulonglong previousValue = m_windowSizeComboBox->currentData().toULongLong();

    m_windowSizeComboBox->clear();

    if (!session.wavData.has_value()) { return; }

    const auto& samples = session.wavData->samples;
    if (samples.empty()) { return; }

    const std::size_t availableFromZero = samples.size();

    // Build FFT-only bin choices as powers of two up to available sample count
    std::vector<std::size_t> values;
    for (std::size_t value = 1; value <= availableFromZero; value *= 2) {
        values.push_back(value);
        if (value > (std::numeric_limits<std::size_t>::max() / 2)) {
            // Prevent overflow before the next doubling step
            break;
        }
    }

    for (std::size_t value : values) {
        m_windowSizeComboBox->addItem(
            QString::number(static_cast<qulonglong>(value)),
            static_cast<qulonglong>(value)
            );
    }

    if (m_windowSizeComboBox->count() == 0) {
        return;
    }

    qulonglong targetValue = previousValue;

    if (targetValue == 0) {
        targetValue = m_windowSizeSpinBox->value() > 0
                          ? static_cast<qulonglong>(m_windowSizeSpinBox->value())
                          : m_windowSizeComboBox->itemData(m_windowSizeComboBox->count() - 1).toULongLong();
    }

    // Pick the largest available value that does not exceed the target
    int bestIndex = 0;
    for (int i = 0; i < m_windowSizeComboBox->count(); ++i) {
        const qulonglong value = m_windowSizeComboBox->itemData(i).toULongLong();
        if (value <= targetValue) {
            bestIndex = i;
        } else {
            break;
        }
    }

    m_windowSizeComboBox->setCurrentIndex(bestIndex);
}

void WavAnalysisControlsWidget::updateFromSpinRange(const SessionData& session)
{
    if (!session.wavData.has_value()) {
        m_fromSpinBox->setRange(0, 0);
        return;
    }

    const auto& samples = session.wavData->samples;
    if (samples.empty()) {
        m_fromSpinBox->setRange(0, 0);
        return;
    }

    const std::size_t windowSize = selectedWindowSize();
    const std::size_t total = samples.size();

    // Keep the selected segment fully inside the available sample range
    const int maxFrom = (windowSize >= total) ? 0 : static_cast<int>(total - windowSize);

    m_fromSpinBox->setRange(0, maxFrom);

    if (m_fromSpinBox->value() > maxFrom) {
        m_fromSpinBox->setValue(maxFrom);
    }
}

void WavAnalysisControlsWidget::updateFromSpinStep()
{
    // Scale navigation step with segment size to make larger windows easier to browse
    const std::size_t windowSize = selectedWindowSize();
    const int step = std::max<int>(1, static_cast<int>(windowSize / 32));
    m_fromSpinBox->setSingleStep(step);
}

} // namespace pdv