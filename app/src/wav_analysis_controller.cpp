#include "pdv/wav_analysis_controller.h"

#include <QtConcurrent/QtConcurrentRun>

#include <cassert>

namespace pdv {

WavAnalysisController::WavAnalysisController(const SessionData& session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
    m_recomputeWatcher = new QFutureWatcher<WavAnalysisEngine::AnalysisResult>(this);

    connect(m_recomputeWatcher, &QFutureWatcher<WavAnalysisEngine::AnalysisResult>::finished,
            this, &WavAnalysisController::handleRecomputeFinished);
}

void WavAnalysisController::recompute()
{
    if (m_isBusy) {
        m_hasPendingRecompute = true;
        return;
    }

    startRecompute();
}

void WavAnalysisController::startRecompute()
{
    if (!m_session.wavData.has_value()) {
        WavAnalysisEngine::AnalysisResult emptyResult{};
        emptyResult.usedSettings = m_settings;
        m_result = std::move(emptyResult);
        emit resultChanged(*m_result);
        return;
    }

    m_isBusy = true;
    emit busyChanged(true);

    const auto wavData = *m_session.wavData;
    const auto settings = m_settings;

    m_recomputeWatcher->setFuture(QtConcurrent::run([wavData, settings]() { return WavAnalysisEngine::analyze(wavData, settings); } ));
}

void WavAnalysisController::handleRecomputeFinished()
{
    m_result = m_recomputeWatcher->result();

    m_isBusy = false;
    emit busyChanged(false);

    emit resultChanged(*m_result);

    if (m_hasPendingRecompute) {
        m_hasPendingRecompute = false;
        startRecompute();
    }
}

void WavAnalysisController::setSettings(const WavAnalysisEngine::AnalysisSettings& settings)
{
    m_settings = settings;
}

const WavAnalysisEngine::AnalysisSettings& WavAnalysisController::settings() const noexcept
{
    return m_settings;
}

bool WavAnalysisController::isBusy() const noexcept
{
    return m_isBusy;
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
