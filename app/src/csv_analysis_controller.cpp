#include "pdv/csv_analysis_controller.h"

#include <cassert>

namespace pdv {

CsvAnalysisController::CsvAnalysisController(const SessionData& session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
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

void CsvAnalysisController::recompute()
{
    CsvAnalysisEngine::AnalysisResult newResult{};
    newResult.usedSettings = m_settings;

    if (m_session.dataSet.has_value()) {
        newResult = CsvAnalysisEngine::analyze(*m_session.dataSet, m_settings);
    }

    m_result = std::move(newResult);
    emit resultChanged(*m_result);
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