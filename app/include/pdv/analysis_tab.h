#pragma once

#include <QWidget>

#include "pdv/session_data.h"

namespace pdv {

class AnalysisTab : public QWidget
{
    Q_OBJECT

public:
    explicit AnalysisTab(const SessionData& session, QWidget* parent = nullptr);
    ~AnalysisTab() override = default;

    QString tabTitle() const;
    const SessionData& session() const noexcept;

    static AnalysisTab* create(const SessionData& session, QWidget* parent = nullptr);

signals:
    void preferredSizeChanged();

protected:
    SessionData m_session;
};



} // namespace pdv
