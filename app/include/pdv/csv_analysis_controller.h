#pragma once

#include <QObject>

#include "pdv/csv_analysis_engine.h"
#include "pdv/session_data.h"

#include <optional>

namespace pdv {

class CsvAnalysisController : public QObject
{
    Q_OBJECT

public:
    explicit CsvAnalysisController(const SessionData& session, QObject* parent = nullptr);

    void setSettings(const CsvAnalysisEngine::AnalysisSettings& settings);
    [[nodiscard]] const CsvAnalysisEngine::AnalysisSettings& settings() const noexcept;

    void recompute();

    [[nodiscard]] bool hasResult() const noexcept;
    [[nodiscard]] const CsvAnalysisEngine::AnalysisResult& result() const;

    [[nodiscard]] const SessionData& session() const noexcept;

signals:
    void resultChanged(const CsvAnalysisEngine::AnalysisResult& result);

private:
    const SessionData& m_session;
    CsvAnalysisEngine::AnalysisSettings m_settings{};
    std::optional<CsvAnalysisEngine::AnalysisResult> m_result;
};

} // namespace pdv