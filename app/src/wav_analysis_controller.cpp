#include "pdv/wav_analysis_controller.h"

#include <cassert>

namespace pdv {

WavAnalysisController::WavAnalysisController(const SessionData& session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
}

void WavAnalysisController::setSettings(const WavAnalysisEngine::AnalysisSettings& settings)
{
    m_settings = settings;
}

const WavAnalysisEngine::AnalysisSettings& WavAnalysisController::settings() const noexcept
{
    return m_settings;
}

void WavAnalysisController::recompute()
{
    WavAnalysisEngine::AnalysisResult newResult{};

    if (m_session.wavData.has_value()) {
        newResult = WavAnalysisEngine::analyze(*m_session.wavData, m_settings);
    }

    m_result = std::move(newResult);
    emit resultChanged(*m_result);
}

bool WavAnalysisController::hasResult() const noexcept
{
    return m_result.has_value();
}

const WavAnalysisEngine::AnalysisResult& WavAnalysisController::result() const
{
    assert(m_result.has_value());
    return *m_result;
}

const SessionData& WavAnalysisController::session() const noexcept
{
    return m_session;
}

} // namespace pdv
