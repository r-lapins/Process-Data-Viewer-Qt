#pragma once

#include <QObject>
#include <QFutureWatcher>

#include "pdv/core/session_data.h"
#include "pdv/wav/wav_analysis_engine.h"

#include <optional>

namespace pdv {

class WavAnalysisController : public QObject
{
    Q_OBJECT

public:
    explicit WavAnalysisController(const SessionData& session, QObject* parent = nullptr);

    void recompute();

    void setSettings(const WavAnalysisEngine::AnalysisSettings& settings);
    [[nodiscard]] const WavAnalysisEngine::AnalysisSettings& settings() const noexcept;

    [[nodiscard]] bool isBusy() const noexcept;
    [[nodiscard]] bool hasResult() const noexcept;
    [[nodiscard]] const WavAnalysisEngine::AnalysisResult& result() const;

    [[nodiscard]] const SessionData& session() const noexcept;

signals:
    void resultChanged(const pdv::WavAnalysisEngine::AnalysisResult& result);
    void busyChanged(bool busy);

private:
    void startRecompute();
    void handleRecomputeFinished();

    bool tryUpdateDominantPeaksOnly();

    [[nodiscard]] static bool requiresFullRecompute(
        const WavAnalysisEngine::AnalysisSettings& previous,
        const WavAnalysisEngine::AnalysisSettings& current) noexcept;

    const SessionData& m_session;
    WavAnalysisEngine::AnalysisSettings m_settings{};
    std::optional<WavAnalysisEngine::AnalysisResult> m_result;

    QFutureWatcher<WavAnalysisEngine::AnalysisResult>* m_recomputeWatcher = nullptr;
    bool m_isBusy = false;
    bool m_hasPendingRecompute = false;
};

} // namespace pdv
