#include "pdv/csv/csv_analysis_engine.h"

namespace pdv {

CsvAnalysisEngine::AnalysisResult CsvAnalysisEngine::analyze(const pdt::DataSet& dataSet, const AnalysisSettings& settings)
{
    AnalysisResult result{};
    result.usedSettings = settings;

    if (hasInvalidTimeRange(settings)) {
        result.invalidTimeRange = true;
        return result;
    }

    result.filteredDataSet = dataSet.filter(toFilterOptions(settings));

    if (result.filteredDataSet.empty()) { return result; }

    computeBasicStats(result.filteredDataSet, result);

    result.anomalySummary = pdt::detect_anomalies_global(result.filteredDataSet,
                                                         settings.anomalyMethod,
                                                         settings.anomalyThreshold,
                                                         settings.topN
                                                         );

    return result;
}

bool CsvAnalysisEngine::hasInvalidTimeRange(const AnalysisSettings& settings)
{
    return settings.useFrom && settings.useTo &&
           settings.filter.from.has_value() && settings.filter.to.has_value() &&
           *settings.filter.from > *settings.filter.to;
}

pdt::FilterOptions CsvAnalysisEngine::toFilterOptions(const AnalysisSettings& settings)
{
    pdt::FilterOptions filter{};

    if (settings.useSensor && settings.filter.sensor) { filter.sensor = settings.filter.sensor; }
    if (settings.useFrom)   { filter.from = settings.filter.from; }
    if (settings.useTo)     { filter.to = settings.filter.to; }

    return filter;
}

void CsvAnalysisEngine::computeBasicStats(const pdt::DataSet& dataSet, AnalysisResult& result)
{
    if (dataSet.empty()) { return; }

    const auto stats = dataSet.stats();
    result.stats.min =      stats.min;
    result.stats.max =      stats.max;
    result.stats.mean =     stats.mean;
    result.stats.stddev =   stats.stddev;
}

} // namespace pdv
