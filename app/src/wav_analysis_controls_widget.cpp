#include "pdv/wav_analysis_controls_widget.h"
#include "pdv/session_data.h"

#include <algorithm>
#include <limits>
#include <vector>

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
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Controls
    auto* controlsGroup = new QGroupBox("Controls", this);
    auto* controlsLayout = new QFormLayout(controlsGroup);

    m_fromSpinBox = new QSpinBox(controlsGroup);

    m_binsSpinBox = new QSpinBox(controlsGroup);
    m_binsComboBox = new QComboBox(controlsGroup);
    m_binsInputStack = new QStackedWidget(controlsGroup);

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

    m_binsSpinBox->setRange(1, std::max(1, sampleCount));
    m_binsSpinBox->setValue(std::min(1024, std::max(1, sampleCount)));

    m_binsInputStack->addWidget(m_binsSpinBox);
    m_binsInputStack->addWidget(m_binsComboBox);
    m_binsInputStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

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
    controlsLayout->addRow("Bins:", m_binsInputStack);
    controlsLayout->addRow("From sample:", m_fromSpinBox);

    // Actions
    auto* actionsGroup = new QGroupBox(this);
    auto* actionsLayout = new QGridLayout(actionsGroup);
    actionsLayout->setContentsMargins(0, 0, 0, 0);

    m_autoUpdateCheckBox = new QCheckBox("Auto update", actionsGroup);
    m_recomputeButton = new QPushButton("Recompute", actionsGroup);

    m_autoUpdateCheckBox->setChecked(true);
    m_recomputeButton->setEnabled(!m_autoUpdateCheckBox->isChecked());

    m_showSignalButton = new QPushButton("Signal", actionsGroup);
    m_showSpectrumButton = new QPushButton("Spectrum", actionsGroup);

    m_exportSignalPlotButton = new QPushButton("Export PNG", actionsGroup);
    m_exportSpectrumPlotButton = new QPushButton("Export PNG", actionsGroup);

    m_showSignalButton->setCheckable(true);
    m_showSignalButton->setChecked(false);
    m_showSignalButton->setStyleSheet(kToggleButtonStyle);

    m_showSpectrumButton->setCheckable(true);
    m_showSpectrumButton->setChecked(false);
    m_showSpectrumButton->setStyleSheet(kToggleButtonStyle);

    auto* plotSignalWidget = new QWidget(actionsGroup);
    auto* plotSignalLayout = new QVBoxLayout(plotSignalWidget);
    plotSignalLayout->setContentsMargins(0, 0, 0, 0);
    plotSignalLayout->addWidget(m_showSignalButton);
    plotSignalLayout->addWidget(m_exportSignalPlotButton);
    plotSignalWidget->setStyleSheet("background-color: #2a2e30;");

    auto* plotSpectrumWidget = new QWidget(actionsGroup);
    auto* plotSpectrumLayout = new QVBoxLayout(plotSpectrumWidget);
    plotSpectrumLayout->setContentsMargins(0, 0, 0, 0);
    plotSpectrumLayout->addWidget(m_showSpectrumButton);
    plotSpectrumLayout->addWidget(m_exportSpectrumPlotButton);
    plotSpectrumWidget->setStyleSheet("background-color: #302e2a;");

    actionsLayout->addWidget(m_autoUpdateCheckBox, 0, 0, Qt::AlignRight);
    actionsLayout->addWidget(m_recomputeButton, 0, 1);
    actionsLayout->addWidget(plotSignalWidget, 1, 0, 2, 1);
    actionsLayout->addWidget(plotSpectrumWidget, 1, 1, 2, 1);

    rootLayout->addWidget(controlsGroup, 0, Qt::AlignTop);
    rootLayout->addWidget(actionsGroup, 0, Qt::AlignTop);
    rootLayout->addStretch();
    rootLayout->addSpacing(10);

    rebuildFftBinsCombo(session);
    updateBinsInputMode();
    updateFromSpinRange(session);
    updateFromSpinStep();
}

void WavAnalysisControlsWidget::connectControls()
{
    connect(m_autoUpdateCheckBox, &QCheckBox::toggled, this, [this](bool checked) { m_recomputeButton->setEnabled(!checked); });

    connect(m_recomputeButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::analysisRequested);

    connect(m_algorithmComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        updateBinsInputMode();
        if (m_session != nullptr) { updateFromSpinRange(*m_session); }

        updateFromSpinStep();
        triggerAutoAnalysis();
    });

    connect(m_peakModeComboBox, &QComboBox::currentIndexChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_windowComboBox, &QComboBox::currentIndexChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_thresholdSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double) { triggerAutoAnalysis(); });

    connect(m_binsComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        if (m_session != nullptr) { updateFromSpinRange(*m_session); }

        updateFromSpinStep();
        triggerAutoAnalysis();
    });

    connect(m_topPeaksSpinBox, &QSpinBox::valueChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_fromSpinBox, &QSpinBox::valueChanged, this, [this](int) { triggerAutoAnalysis(); });
    connect(m_binsSpinBox, &QSpinBox::valueChanged, this, [this](int) {
        if (m_session != nullptr) { updateFromSpinRange(*m_session); }
        updateFromSpinStep();
        triggerAutoAnalysis();
    });

    connect(m_showSignalButton, &QPushButton::toggled, this, &WavAnalysisControlsWidget::signalPlotToggled);
    connect(m_showSpectrumButton, &QPushButton::toggled, this, &WavAnalysisControlsWidget::spectrumPlotToggled);

    connect(m_exportSignalPlotButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::exportSignalPlotRequested);
    connect(m_exportSpectrumPlotButton, &QPushButton::clicked, this, &WavAnalysisControlsWidget::exportSpectrumPlotRequested);
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
    s.bins = selectedBins();

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

std::size_t WavAnalysisControlsWidget::selectedBins() const
{
    const auto selected = static_cast<WavAnalysisEngine::SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());

    if (selected == WavAnalysisEngine::SpectrumAlgorithm::Fft) {
        return static_cast<std::size_t>(m_binsComboBox->currentData().toULongLong());
    }

    return static_cast<std::size_t>(m_binsSpinBox->value());
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

void WavAnalysisControlsWidget::updateBinsInputMode()
{
    const auto selected = static_cast<WavAnalysisEngine::SpectrumAlgorithm>(m_algorithmComboBox->currentData().toInt());

    if (selected == WavAnalysisEngine::SpectrumAlgorithm::Fft) {
        m_binsInputStack->setCurrentWidget(m_binsComboBox);
    } else {
        m_binsInputStack->setCurrentWidget(m_binsSpinBox);
    }
}

void WavAnalysisControlsWidget::rebuildFftBinsCombo(const SessionData& session)
{
    // It generates a list of valid FFT sizes (powers of 2)
    // and selects the best value based on the current UI state.
    const qulonglong previousValue = m_binsComboBox->currentData().toULongLong();

    m_binsComboBox->clear();

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
        m_binsComboBox->addItem(
            QString::number(static_cast<qulonglong>(value)),
            static_cast<qulonglong>(value)
            );
    }

    if (m_binsComboBox->count() == 0) {
        return;
    }

    qulonglong targetValue = previousValue;

    if (targetValue == 0) {
        targetValue = m_binsSpinBox->value() > 0
                          ? static_cast<qulonglong>(m_binsSpinBox->value())
                          : m_binsComboBox->itemData(m_binsComboBox->count() - 1).toULongLong();
    }

    // Pick the largest available value that does not exceed the target
    int bestIndex = 0;
    for (int i = 0; i < m_binsComboBox->count(); ++i) {
        const qulonglong value = m_binsComboBox->itemData(i).toULongLong();
        if (value <= targetValue) {
            bestIndex = i;
        } else {
            break;
        }
    }

    m_binsComboBox->setCurrentIndex(bestIndex);
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

    const std::size_t bins = selectedBins();
    const std::size_t total = samples.size();

    // Keep the selected segment fully inside the available sample range
    const int maxFrom = (bins >= total) ? 0 : static_cast<int>(total - bins);

    m_fromSpinBox->setRange(0, maxFrom);

    if (m_fromSpinBox->value() > maxFrom) {
        m_fromSpinBox->setValue(maxFrom);
    }
}

void WavAnalysisControlsWidget::updateFromSpinStep()
{
    // Scale navigation step with segment size to make larger windows easier to browse
    const std::size_t bins = selectedBins();
    const int step = std::max<int>(1, static_cast<int>(bins / 32));
    m_fromSpinBox->setSingleStep(step);
}

} // namespace pdv