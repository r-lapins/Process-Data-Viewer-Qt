#include "pdv/wav/wav_analysis_controller.h"

#include <QtConcurrent/QtConcurrentRun>

#include <cassert>
#include <QString>
#include <exception>

namespace pdv {

WavAnalysisController::WavAnalysisController(const SessionData& session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
    if (m_session.wavData.has_value()) {
        m_analysisSession = std::make_unique<pdt::WavAnalysisSession>(m_session.wavData->samples,
                                                                      static_cast<double>(m_session.wavData->sample_rate));
    }

    m_recomputeWatcher = new QFutureWatcher<pdt::WavAnalysisResult>(this);

    connect(m_recomputeWatcher, &QFutureWatcher<pdt::WavAnalysisResult>::finished,
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
    if (!m_analysisSession) {
        pdt::WavAnalysisResult emptyResult{};
        emptyResult.used_settings = pdt::WavAnalysisSettingsCache{.sample_rate = 0.0,
                                                                  .algorithm = m_settings.algorithm,
                                                                  .peak_mode = m_settings.peak_mode,
                                                                  .window = m_settings.window,
                                                                  .top_peaks = m_settings.top_peaks,
                                                                  .from = m_settings.from,
                                                                  .window_size = m_settings.window_size,
                                                                  .threshold = m_settings.threshold};

        m_result = std::move(emptyResult);
        emit resultChanged(*m_result);
        return;
    }

    m_isBusy = true;
    emit busyChanged(true);

    auto* analysisSession = m_analysisSession.get();
    const auto settings = m_settings;

    m_recomputeWatcher->setFuture(QtConcurrent::run([analysisSession, settings]() {
        const auto& cached = analysisSession->analyze(settings);
        return pdt::WavAnalysisResult{cached};
    }));
}

void WavAnalysisController::handleRecomputeFinished()
{
    try {
        m_result = m_recomputeWatcher->result();
    } catch (const std::exception& ex) {
        m_isBusy = false;
        emit busyChanged(false);
        emit analysisFailed(QString::fromUtf8(ex.what()));
        return;
    } catch (...) {
        m_isBusy = false;
        emit busyChanged(false);
        emit analysisFailed("Unknown analysis error");
        return;
    }

    m_isBusy = false;
    emit busyChanged(false);

    emit resultChanged(*m_result);

    if (m_hasPendingRecompute) {
        m_hasPendingRecompute = false;
        startRecompute();
    }
}

void WavAnalysisController::setSettings(const pdt::WavAnalysisSettingsCache& settings)
{
    m_settings = settings;
}

const pdt::WavAnalysisSettingsCache& WavAnalysisController::settings() const noexcept
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

const pdt::WavAnalysisResult& WavAnalysisController::result() const
{
    assert(m_result.has_value());
    return *m_result;
}

const SessionData& WavAnalysisController::session() const noexcept
{
    return m_session;
}

} // namespace pdv
