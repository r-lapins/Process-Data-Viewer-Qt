#pragma once

#include <pdt/csv/dataset.h>
#include <pdt/csv/anomaly.h>

#include <string>
#include <optional>
#include <cstddef>
#include <chrono>

namespace pdv {

class CsvAnalysisEngine
{
public:
    struct AnalysisSettings {
        bool useSensor{false};
        bool useFrom{false};
        bool useTo{false};

        std::string sensor;
        std::optional<std::chrono::sys_seconds> from;
        std::optional<std::chrono::sys_seconds> to;

        pdt::AnomalyMethod anomalyMethod{pdt::AnomalyMethod::ZScore};
        double anomalyThreshold{1.5};
        std::size_t topN{20};
    };

    struct AnalysisResult {
        AnalysisSettings usedSettings{};
        pdt::DataSet filteredDataSet;
        pdt::AnomalySummary anomalySummary{};

        double minValue{0.0};
        double maxValue{0.0};
        double meanValue{0.0};
        double stddevValue{0.0};

        bool invalidTimeRange{false};
    };

    [[nodiscard]] static AnalysisResult analyze(const pdt::DataSet& dataSet, const AnalysisSettings& settings);

private:
    [[nodiscard]] static bool hasInvalidTimeRange(const AnalysisSettings& settings);
    [[nodiscard]] static pdt::FilterOptions toFilterOptions(const AnalysisSettings& settings);
    static void computeBasicStats(const pdt::DataSet& dataSet, AnalysisResult& result);
};

} // namespace pdv
