#pragma once

#include <pdt/core/dataset.h>
#include <pdt/core/anomaly.h>

#include <string>
#include <vector>
#include <optional>
#include <cstddef>
#include <chrono>

namespace pdv {

class CsvAnalysisEngine
{
public:
    enum class AnomalyMethod {
        ZScore,
        IQR,
        MAD
    };

    struct AnalysisSettings {
        std::string sensor;
        bool useSensor{false};

        bool useFrom{false};
        bool useTo{false};

        std::optional<std::chrono::sys_seconds> from;
        std::optional<std::chrono::sys_seconds> to;

        AnomalyMethod anomalyMethod{AnomalyMethod::ZScore};
        double anomalyThreshold{1.5};
        std::size_t topN{20};
    };

    struct AnalysisResult {
        AnalysisSettings usedSettings{};
        pdt::DataSet filteredDataSet;
        pdt::AnomalySummary anomalySummary{};

        std::vector<std::size_t> anomalyIndices;

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
    [[nodiscard]] static pdt::AnomalyMethod toPdtMethod(AnomalyMethod method);
    static void computeBasicStats(const pdt::DataSet& dataSet, AnalysisResult& result);
};

} // namespace pdv
