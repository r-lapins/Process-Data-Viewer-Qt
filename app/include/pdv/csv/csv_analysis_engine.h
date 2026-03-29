#pragma once

#include <pdt/csv/anomaly.h>
#include <pdt/csv/dataset.h>
#include <pdt/csv/types.h>

#include <cstddef>

namespace pdv {

class CsvAnalysisEngine
{
public:
    struct AnalysisSettings {
        pdt::FilterOptions filter;
        pdt::AnomalyMethod anomalyMethod{pdt::AnomalyMethod::ZScore};
        double anomalyThreshold{1.5};
        std::size_t topN{20};

        bool useSensor{false};
        bool useFrom{false};
        bool useTo{false};
    };

    struct AnalysisResult {
        pdt::AnomalySummary anomalySummary{};
        AnalysisSettings usedSettings{};
        pdt::DataSet filteredDataSet;
        pdt::Stats stats{};

        bool invalidTimeRange{false};
    };

    [[nodiscard]] static AnalysisResult analyze(const pdt::DataSet& dataSet, const AnalysisSettings& settings);

private:
    [[nodiscard]] static pdt::FilterOptions toFilterOptions(const AnalysisSettings& settings);
    [[nodiscard]] static bool hasInvalidTimeRange(const AnalysisSettings& settings);
    static void computeBasicStats(const pdt::DataSet& dataSet, AnalysisResult& result);
};

} // namespace pdv
