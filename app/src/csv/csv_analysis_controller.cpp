#include "pdv/csv/csv_analysis_controller.h"

#include <QtConcurrent/QtConcurrentRun>

#include <cassert>

namespace pdv {

CsvAnalysisController::CsvAnalysisController(const SessionData& session, QObject* parent)
    : QObject(parent) , m_session(session)
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

    if (tryUpdateTopAnomaliesOnly()) {
        emit resultChanged(*m_result);
        return;
    }

    startRecompute();
}

void CsvAnalysisController::startRecompute()
{
    if (!m_session.csvData.has_value()) {
        CsvAnalysisEngine::AnalysisResult emptyResult{};
        emptyResult.usedSettings = m_settings;
        m_result = std::move(emptyResult);
        emit resultChanged(*m_result);
        return;
    }

    m_isBusy = true;
    emit busyChanged(true);

    const auto settings = m_settings;
    const auto& dataSet = m_session.csvData->dataSet;

    m_recomputeWatcher->setFuture(QtConcurrent::run([dataSet, settings]() {
        return CsvAnalysisEngine::analyze(dataSet, settings);
    }));
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

bool CsvAnalysisController::tryUpdateTopAnomaliesOnly()
{
    if (!m_result.has_value()) { return false; }

    const auto& previousSettings = m_result->usedSettings;
    const auto& currentSettings = m_settings;

    if (requiresFullRecompute(previousSettings, currentSettings)) { return false; }

    m_result->usedSettings.topN = currentSettings.topN;
    m_result->anomalySummary.top = pdt::select_top_anomalies(m_result->anomalySummary.all, currentSettings.topN);

    return true;
}

bool CsvAnalysisController::requiresFullRecompute(const CsvAnalysisEngine::AnalysisSettings& previous,
                                                  const CsvAnalysisEngine::AnalysisSettings& current) noexcept
{
    return previous.filter.sensor != current.filter.sensor ||
           previous.useSensor != current.useSensor ||
           previous.useFrom != current.useFrom ||
           previous.useTo != current.useTo ||
           previous.filter.from != current.filter.from ||
           previous.filter.to != current.filter.to ||
           previous.anomalyMethod != current.anomalyMethod ||
           previous.anomalyThreshold != current.anomalyThreshold;
}

void CsvAnalysisController::setSettings(const CsvAnalysisEngine::AnalysisSettings& settings)
{
    m_settings = settings;
}

const CsvAnalysisEngine::AnalysisSettings& CsvAnalysisController::settings() const noexcept
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

const CsvAnalysisEngine::AnalysisResult& CsvAnalysisController::result() const
{
    assert(m_result.has_value());
    return *m_result;
}

const SessionData& CsvAnalysisController::session() const noexcept
{
    return m_session;
}

} // namespace pdv