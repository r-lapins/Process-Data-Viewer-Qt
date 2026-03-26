#include "pdv/core/analysis_tab.h"
#include "pdv/csv/csv_analysis_tab.h"
#include "pdv/wav/wav_analysis_tab.h"

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
    using enum SessionData::FileKind;
    switch (session.kind) {
    case Csv:
        return new CsvAnalysisTab(session, parent);

    case Wav:
        return new WavAnalysisTab(session, parent);

    case Unknown:
    default:
        return nullptr;
    }
}

} // namespace pdv
