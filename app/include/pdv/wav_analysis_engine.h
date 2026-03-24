#pragma once

#include <pdt/signal/window.h>
#include <pdt/signal/peak_detection.h>
#include <pdt/signal/wav_reader.h>

#include <vector>
#include <span>
#include <cstddef>

namespace pdv {

class WavAnalysisEngine
{
public:
    enum class SpectrumAlgorithm { Dft, Fft };

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
        AnalysisSettings usedSettings;

        std::vector<double> rawSegment;
        pdt::Spectrum spectrum;

        std::vector<double> processedSegment;
        std::vector<pdt::Peak> allPeaks;
        std::vector<pdt::Peak> dominantPeaks;

        double minValue{0.0};
        double maxValue{0.0};
        double meanValue{0.0};
        double stddevValue{0.0};
    };

    [[nodiscard]] static AnalysisResult analyze(const pdt::WavData& wav, const AnalysisSettings& settings);

private:
    [[nodiscard]] static std::vector<double> selectSegment(
        std::span<const double> samples,
        std::size_t from,
        std::size_t bins
        );

    static void computeBasicStats(
        std::span<const double> samples,
        AnalysisResult& result
        );
};

} // namespace pdv
