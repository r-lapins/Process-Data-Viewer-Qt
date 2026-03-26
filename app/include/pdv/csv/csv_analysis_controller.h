#pragma once

#include <QObject>
#include <QFutureWatcher>

#include "pdv/csv/csv_analysis_engine.h"
#include "pdv/core/session_data.h"

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

    [[nodiscard]] bool isBusy() const noexcept;
    [[nodiscard]] bool hasResult() const noexcept;
    [[nodiscard]] const CsvAnalysisEngine::AnalysisResult& result() const;

    [[nodiscard]] const SessionData& session() const noexcept;

signals:
    void resultChanged(const pdv::CsvAnalysisEngine::AnalysisResult& result);
    void busyChanged(bool busy);

private:
    void startRecompute();
    void handleRecomputeFinished();

    const SessionData& m_session;
    CsvAnalysisEngine::AnalysisSettings m_settings{};
    std::optional<CsvAnalysisEngine::AnalysisResult> m_result;

    QFutureWatcher<CsvAnalysisEngine::AnalysisResult>* m_recomputeWatcher = nullptr;
    bool m_isBusy = false;
    bool m_hasPendingRecompute = false;
};

} // namespace pdv