#include "pdv/analysis_tab.h"

#include "pdv/csv_analysis_tab.h"
#include "pdv/wav_analysis_tab.h"

#include <QFileInfo>

namespace pdv {

AnalysisTab::AnalysisTab(const SessionData& session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
}

QString AnalysisTab::tabTitle() const
{
    return QFileInfo(m_session.filePath).fileName();
}

const SessionData& AnalysisTab::session() const noexcept
{
    return m_session;
}

AnalysisTab* AnalysisTab::create(const SessionData& session, QWidget* parent)
{
    switch (session.kind) {
    case SessionData::FileKind::Csv:
        return new CsvAnalysisTab(session, parent);

    case SessionData::FileKind::Wav:
        return new WavAnalysisTab(session, parent);

    case SessionData::FileKind::Unknown:
    default:
        return nullptr;
    }
}

} // namespace pdv
