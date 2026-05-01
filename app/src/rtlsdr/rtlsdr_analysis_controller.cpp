#include "pdv/rtlsdr/rtlsdr_analysis_controller.h"

#include "pdt/compute/make_fft_backend.h"
#include "pdt/pipeline/iq_spectrum_engine.h"

#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include <exception>
#include <stdexcept>
#include <utility>

namespace pdv {
namespace {

bool sameDeviceStreamSettings(const RtlSdrAnalysisSettings& lhs, const RtlSdrAnalysisSettings& rhs) noexcept
{
    return lhs.device.device_index == rhs.device.device_index
           && lhs.device.center_frequency == rhs.device.center_frequency
           && lhs.device.sample_rate == rhs.device.sample_rate
           && lhs.device.tuner_gain_tenth_db == rhs.device.tuner_gain_tenth_db
           && lhs.device.bias_tee == rhs.device.bias_tee
           && lhs.blockBytes == rhs.blockBytes;
}

} // namespace

RtlSdrAnalysisController::RtlSdrAnalysisController(QObject* parent)
    : QObject(parent)
{
    m_analysisTimer = new QTimer(this);
    m_analysisTimer->setInterval(200);
    connect(m_analysisTimer, &QTimer::timeout, this, &RtlSdrAnalysisController::startAnalysisIfReady);

    m_analysisWatcher = new QFutureWatcher<RtlSdrAnalysisResult>(this);
    connect(m_analysisWatcher, &QFutureWatcher<RtlSdrAnalysisResult>::finished,
            this, &RtlSdrAnalysisController::handleAnalysisFinished);
}

RtlSdrAnalysisController::~RtlSdrAnalysisController()
{
    stop();
}

std::vector<pdt::RtlSdrDeviceInfo> RtlSdrAnalysisController::enumerateDevices()
{
    return pdt::RtlSdrDevice::enumerate();
}

void RtlSdrAnalysisController::start(const RtlSdrAnalysisSettings& settings)
{
    try {
        stopStream(false);
        startStream(settings, true);
    } catch (const std::exception& ex) {
        m_running = false;
        emit streamFailed(QString::fromUtf8(ex.what()));
    }
}

void RtlSdrAnalysisController::stop()
{
    stopStream(true);
}

void RtlSdrAnalysisController::updateSettings(const RtlSdrAnalysisSettings& settings)
{
    if (!m_running.load()) {
        m_settings = settings;
        m_analysisTimer->setInterval(settings.refreshIntervalMs);
        return;
    }

    try {
        if (requiresStreamRestart(settings)) {
            stopStream(false);
            startStream(settings, false);
            return;
        }

        m_settings = settings;
        m_analysisTimer->setInterval(settings.refreshIntervalMs);
        ++m_generation;
    } catch (const std::exception& ex) {
        emit streamFailed(QString::fromUtf8(ex.what()));
        stopStream(true);
    }
}

void RtlSdrAnalysisController::stopStream(bool emitRunning)
{
    const bool wasRunning = m_running.exchange(false);
    ++m_generation;

    if (m_analysisTimer != nullptr) {
        m_analysisTimer->stop();
    }

    m_stream.stop();

    {
        std::scoped_lock lock(m_latestFrameMutex);
        m_latestFrame.reset();
    }

    if (emitRunning && wasRunning) {
        emit runningChanged(false);
    }
}

bool RtlSdrAnalysisController::isRunning() const noexcept
{
    return m_running.load();
}

void RtlSdrAnalysisController::storeFrame(pdt::IqFrame&& frame)
{
    if (!m_running.load()) { return; }

    std::scoped_lock lock(m_latestFrameMutex);
    m_latestFrame = std::move(frame);
}

void RtlSdrAnalysisController::startAnalysisIfReady()
{
    if (!m_running.load() || m_analysisWatcher->isRunning()) { return; }

    std::optional<pdt::IqFrame> frame;
    {
        std::scoped_lock lock(m_latestFrameMutex);
        if (!m_latestFrame.has_value()) { return; }
        frame = std::move(m_latestFrame);
        m_latestFrame.reset();
    }

    const auto settings = m_settings;
    const auto generation = m_generation;

    m_analysisWatcher->setFuture(QtConcurrent::run([frame = std::move(*frame), settings, generation]() mutable {
        auto backend = pdt::create_fft_backend(settings.algorithm);
        if (!backend) {
            throw std::runtime_error("Failed to create FFT backend.");
        }

        pdt::IqSpectrumEngine engine(*backend);

        pdt::SpectrumAnalysisOptions options{.sample_rate = static_cast<double>(frame.sample_rate),
                                             .window = settings.window,
                                             .peak_mode = settings.peakMode,
                                             .threshold = settings.threshold,
                                             .max_peaks = settings.maxPeaks};

        RtlSdrAnalysisResult result{};
        result.frame = std::move(frame);
        result.analysis = engine.process(result.frame, options);
        result.settings = settings;
        result.generation = generation;

        return result;
    }));
}

void RtlSdrAnalysisController::handleAnalysisFinished()
{
    try {
        const auto result = m_analysisWatcher->result();
        if (m_running.load() && result.generation == m_generation) {
            emit resultChanged(result);
        }
    } catch (const std::exception& ex) {
        emit streamFailed(QString::fromUtf8(ex.what()));
        stop();
    }
}

bool RtlSdrAnalysisController::requiresStreamRestart(const RtlSdrAnalysisSettings& settings) const noexcept
{
    return !sameDeviceStreamSettings(m_settings, settings);
}

void RtlSdrAnalysisController::startStream(const RtlSdrAnalysisSettings& settings, bool emitRunning)
{
    m_settings = settings;
    m_analysisTimer->setInterval(settings.refreshIntervalMs);
    ++m_generation;

    {
        std::scoped_lock lock(m_latestFrameMutex);
        m_latestFrame.reset();
    }

    const bool started = m_stream.start(settings.device,
                                       settings.blockBytes,
                                       [this](pdt::IqFrame&& frame) {
                                           storeFrame(std::move(frame));
                                       });

    if (!started) {
        throw std::runtime_error("Failed to start RTL-SDR stream.");
    }

    m_running = true;
    m_analysisTimer->start();

    if (emitRunning) {
        emit runningChanged(true);
    }
}

} // namespace pdv
