#include "pdv/wav_analysis_engine.h"

#include <pdt/signal/dft.h>
#include <pdt/signal/fft.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace pdv {

WavAnalysisEngine::AnalysisResult
WavAnalysisEngine::analyze(const pdt::WavData& wav, const AnalysisSettings& settings)
{
    AnalysisResult result{};
    result.usedSettings = settings;

    if (wav.sample_rate == 0 || wav.samples.empty()) {
        return result;
    }

    result.rawSegment = selectSegment(wav.samples, settings.from, settings.windowSize);
    if (result.rawSegment.empty()) {
        return result;
    }

    result.processedSegment = result.rawSegment;

    if (settings.useWindow) {
        result.processedSegment = pdt::apply_window(result.rawSegment, settings.window);
    }

    computeBasicStats(result.rawSegment, result);

    using enum SpectrumAlgorithm;
    switch (settings.algorithm) {
    case Dft:
        result.spectrum = pdt::compute_single_sided_spectrum(
            result.processedSegment,
            static_cast<double>(wav.sample_rate)
            );
        break;

    case Fft:
        if (!pdt::is_power_of_two(result.processedSegment.size())) {
            return result;
        }

        result.spectrum = pdt::compute_single_sided_spectrum_fft(
            result.processedSegment,
            static_cast<double>(wav.sample_rate)
            );
        break;
    }

    result.allPeaks = pdt::find_peaks(
        result.spectrum.frequencies,
        result.spectrum.magnitudes,
        settings.threshold,
        settings.peakMode
        );

    result.dominantPeaks = pdt::detect_dominant_peaks(
        result.spectrum,
        settings.threshold,
        settings.peakMode,
        settings.topPeaks
        );

    return result;
}

// Extract analysis window starting at 'from' with length 'windowSize'.
// Clamps to available samples to avoid out-of-range access.
std::vector<double> WavAnalysisEngine::selectSegment(
    std::span<const double> samples,
    std::size_t from,
    std::size_t windowSize
    )
{
    if (samples.empty() || from >= samples.size()) {
        return {};
    }

    const std::size_t available = samples.size() - from;
    const std::size_t used = std::min(windowSize, available);

    return std::vector<double>(
        samples.begin() + static_cast<std::ptrdiff_t>(from),
        samples.begin() + static_cast<std::ptrdiff_t>(from + used)
        );
}

void WavAnalysisEngine::computeBasicStats(
    std::span<const double> samples,
    AnalysisResult& result
    )
{
    if (samples.empty()) {
        return;
    }

    const auto [minIt, maxIt] = std::minmax_element(samples.begin(), samples.end());
    result.minValue = *minIt;
    result.maxValue = *maxIt;

    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    result.meanValue = sum / static_cast<double>(samples.size());

    double sqSum = 0.0;
    for (double sample : samples) {
        const double diff = sample - result.meanValue;
        sqSum += diff * diff;
    }

    result.stddevValue = std::sqrt(sqSum / static_cast<double>(samples.size()));
}

} // namespace pdv
