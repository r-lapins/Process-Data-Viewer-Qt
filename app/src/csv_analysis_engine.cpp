#include "pdv/csv_analysis_engine.h"

namespace pdv {

CsvAnalysisEngine::AnalysisResult
CsvAnalysisEngine::analyze(const pdt::DataSet& dataSet, const AnalysisSettings& settings)
{
    AnalysisResult result{};
    result.usedSettings = settings;

    if (hasInvalidTimeRange(settings)) {
        result.invalidTimeRange = true;
        return result;
    }

    result.filteredDataSet = dataSet.filter(toFilterOptions(settings));

    if (result.filteredDataSet.empty()) {
        return result;
    }

    computeBasicStats(result.filteredDataSet, result);

    result.anomalySummary = pdt::detect_zscore_global(
        result.filteredDataSet,
        settings.zThreshold,
        settings.topN
        );

    const auto& samples = result.filteredDataSet.samples();

    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto& sample = samples[i];

        // Match anomaly entries back to original sample indices
        for (const auto& anomaly : result.anomalySummary.top) {
            if (sample.timestamp == anomaly.timestamp &&
                sample.sensor == anomaly.sensor &&
                sample.value == anomaly.value) {
                result.anomalyIndices.push_back(i);
                break;
            }
        }
    }

    return result;
}

bool CsvAnalysisEngine::hasInvalidTimeRange(const AnalysisSettings& settings)
{
    return settings.useFrom && settings.useTo &&
           settings.from.has_value() && settings.to.has_value() &&
           *settings.from > *settings.to;
}

pdt::FilterOptions CsvAnalysisEngine::toFilterOptions(const AnalysisSettings& settings)
{
    pdt::FilterOptions filter{};

    if (settings.useSensor && !settings.sensor.empty()) {
        filter.sensor = settings.sensor;
    }

    if (settings.useFrom) {
        filter.from = settings.from;
    }

    if (settings.useTo) {
        filter.to = settings.to;
    }

    return filter;
}

void CsvAnalysisEngine::computeBasicStats(
    const pdt::DataSet& dataSet,
    AnalysisResult& result
    )
{
    if (dataSet.empty()) {
        return;
    }

    const auto stats = dataSet.stats();
    result.minValue = stats.min;
    result.maxValue = stats.max;
    result.meanValue = stats.mean;
    result.stddevValue = stats.stddev;
}

} // namespace pdv
