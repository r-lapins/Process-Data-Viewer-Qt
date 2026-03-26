#include "pdv/csv/csv_analysis_controller.h"

#include <QtConcurrent/QtConcurrentRun>

#include <cassert>

namespace pdv {

CsvAnalysisController::CsvAnalysisController(const SessionData& session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
    m_recomputeWatcher = new QFutureWatcher<CsvAnalysisEngine::AnalysisResult>(this);

    connect(m_recomputeWatcher, &QFutureWatcher<CsvAnalysisEngine::AnalysisResult>::finished,
            this, &CsvAnalysisController::handleRecomputeFinished);
}

void CsvAnalysisController::recompute()
{
    if (m_isBusy) {
        m_hasPendingRecompute = true;
        return;
    }

    startRecompute();
}

void CsvAnalysisController::startRecompute()
{
    if (!m_session.dataSet.has_value()) {
        CsvAnalysisEngine::AnalysisResult emptyResult{};
        emptyResult.usedSettings = m_settings;
        m_result = std::move(emptyResult);
        emit resultChanged(*m_result);
        return;
    }

    m_isBusy = true;
    emit busyChanged(true);

    const auto dataSet = *m_session.dataSet;
    const auto settings = m_settings;

    m_recomputeWatcher->setFuture(QtConcurrent::run([dataSet, settings]() { return CsvAnalysisEngine::analyze(dataSet, settings); } ));
}

void CsvAnalysisController::handleRecomputeFinished()
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

void CsvAnalysisController::setSettings(const CsvAnalysisEngine::AnalysisSettings& settings)
{
    m_settings = settings;
}

const CsvAnalysisEngine::AnalysisSettings&
CsvAnalysisController::settings() const noexcept
{
    return m_settings;
}

bool CsvAnalysisController::isBusy() const noexcept
{
    return m_isBusy;
}

bool CsvAnalysisController::hasResult() const noexcept
{
    return m_result.has_value();
}

const CsvAnalysisEngine::AnalysisResult&
CsvAnalysisController::result() const
{
    assert(m_result.has_value());
    return *m_result;
}

const SessionData& CsvAnalysisController::session() const noexcept
{
    return m_session;
}

} // namespace pdv