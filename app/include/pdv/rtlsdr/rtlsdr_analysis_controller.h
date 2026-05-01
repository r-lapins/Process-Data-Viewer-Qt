#pragma once

#include <QObject>
#include <QFutureWatcher>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

#include <QString>

#include "pdt/io/rtlsdr/iq_frame.h"
#include "pdt/io/rtlsdr/rtl_sdr_device.h"
#include "pdt/io/rtlsdr/rtl_sdr_stream.h"
#include "pdt/pipeline/spectrum_analysis_types.h"

class QTimer;

namespace pdv {

struct RtlSdrAnalysisSettings
{
    pdt::RtlSdrConfig device;
    std::size_t blockBytes{16384};
    int refreshIntervalMs{200};
    pdt::SpectrumAlgorithm algorithm{pdt::SpectrumAlgorithm::Fft};
    pdt::WindowType window{pdt::WindowType::Hann};
    pdt::PeakDetectionMode peakMode{pdt::PeakDetectionMode::LocalMaxima};
    double threshold{0.20};
    std::size_t maxPeaks{15};
};

struct RtlSdrAnalysisResult
{
    pdt::IqFrame frame;
    pdt::SpectrumAnalysisResult analysis;
    RtlSdrAnalysisSettings settings;
    std::uint64_t generation{};
};

class RtlSdrAnalysisController : public QObject
{
    Q_OBJECT

public:
    explicit RtlSdrAnalysisController(QObject* parent = nullptr);
    ~RtlSdrAnalysisController() override;

    [[nodiscard]] static std::vector<pdt::RtlSdrDeviceInfo> enumerateDevices();

    void start(const RtlSdrAnalysisSettings& settings);
    void stop();
    void updateSettings(const RtlSdrAnalysisSettings& settings);
    [[nodiscard]] bool isRunning() const noexcept;

signals:
    void resultChanged(const pdv::RtlSdrAnalysisResult& result);
    void runningChanged(bool running);
    void streamFailed(const QString& message);

private:
    void storeFrame(pdt::IqFrame&& frame);
    void startAnalysisIfReady();
    void handleAnalysisFinished();
    [[nodiscard]] bool requiresStreamRestart(const RtlSdrAnalysisSettings& settings) const noexcept;
    void startStream(const RtlSdrAnalysisSettings& settings, bool emitRunning);
    void stopStream(bool emitRunning);

    pdt::RtlSdrStream m_stream;
    QTimer* m_analysisTimer = nullptr;
    QFutureWatcher<RtlSdrAnalysisResult>* m_analysisWatcher = nullptr;
    RtlSdrAnalysisSettings m_settings{};
    std::mutex m_latestFrameMutex;
    std::optional<pdt::IqFrame> m_latestFrame;
    std::atomic_bool m_running{false};
    std::uint64_t m_generation{0};
};

} // namespace pdv
