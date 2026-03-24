#pragma once

#include <QObject>

#include "pdv/session_data.h"
#include "pdv/wav_analysis_engine.h"

#include <optional>

namespace pdv {

class WavAnalysisController : public QObject
{
    Q_OBJECT

public:
    explicit WavAnalysisController(const SessionData& session, QObject* parent = nullptr);

    void setSettings(const WavAnalysisEngine::AnalysisSettings& settings);
    [[nodiscard]] const WavAnalysisEngine::AnalysisSettings& settings() const noexcept;

    void recompute();

    [[nodiscard]] bool hasResult() const noexcept;
    [[nodiscard]] const WavAnalysisEngine::AnalysisResult& result() const;

    [[nodiscard]] const SessionData& session() const noexcept;

signals:
    void resultChanged(const WavAnalysisEngine::AnalysisResult& result);

private:
    const SessionData& m_session;
    WavAnalysisEngine::AnalysisSettings m_settings{};
    std::optional<WavAnalysisEngine::AnalysisResult> m_result;
};

} // namespace pdv
