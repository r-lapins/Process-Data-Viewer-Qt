#pragma once

#include <pdt/wav/window.h>
#include <pdt/wav/peak_detection.h>
#include <pdt/wav/wav_reader.h>
#include <pdt/wav/spectrum.h>
#include <pdt/csv/types.h>

#include <vector>
#include <span>
#include <cstddef>

namespace pdv {

class WavAnalysisEngine
{
public:

    struct AnalysisSettings {
        pdt::PeakDetectionMode peakMode{pdt::PeakDetectionMode::LocalMaxima};
        pdt::SpectrumAlgorithm algorithm{pdt::SpectrumAlgorithm::Fft};
        pdt::WindowType window{pdt::WindowType::Hann};
        std::size_t topPeaks{20};
        std::size_t from{0};
        std::size_t windowSize{1024};
        double threshold{0.20};
    };

    struct AnalysisResult {
        std::vector<double> rawSegment;
        std::vector<double> processedSegment;
        std::vector<pdt::Peak> allPeaks;
        std::vector<pdt::Peak> dominantPeaks;
        AnalysisSettings usedSettings;
        pdt::Spectrum spectrum;
        pdt::Stats stats{};
    };

    [[nodiscard]] static AnalysisResult analyze(const pdt::WavData& wav, const AnalysisSettings& settings);

private:
    [[nodiscard]] static std::vector<double> selectSegment(std::span<const double> samples, std::size_t from, std::size_t windowSize);

    static void computeBasicStats(std::span<const double> samples, AnalysisResult& result);
};

} // namespace pdv
