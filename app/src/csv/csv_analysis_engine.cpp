#include "pdv/csv/csv_analysis_engine.h"

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

    result.anomalySummary = pdt::detect_anomalies_global(
        result.filteredDataSet,
        toPdtMethod(settings.anomalyMethod),
        settings.anomalyThreshold,
        settings.topN
        );

    const auto& samples = result.filteredDataSet.samples();
    // Preserve the same ordering as anomalySummary.top so each displayed anomaly
    // can be paired with its corresponding row index in the filtered table view.
    // In this dataset model each sample is expected to be unique by (timestamp, sensor, value).
    for (const auto& anomaly : result.anomalySummary.top) {
        auto it = std::find_if(samples.begin(), samples.end(),
                               [&anomaly](const auto& sample) {
                                   return sample.timestamp == anomaly.timestamp &&
                                          sample.sensor == anomaly.sensor &&
                                          sample.value == anomaly.value;
                               });

        if (it != samples.end()) {
            result.anomalyIndices.push_back(
                static_cast<std::size_t>(std::distance(samples.begin(), it))
                );
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

pdt::AnomalyMethod CsvAnalysisEngine::toPdtMethod(AnomalyMethod method)
{
    using GuiMethod = AnomalyMethod;
    using PdtMethod = pdt::AnomalyMethod;

    switch (method) {
    case GuiMethod::ZScore:
        return PdtMethod::ZScore;
    case GuiMethod::IQR:
        return PdtMethod::IQR;
    case GuiMethod::MAD:
        return PdtMethod::MAD;
    }

    return PdtMethod::ZScore;
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
