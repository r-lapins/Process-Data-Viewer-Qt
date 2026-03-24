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

    [[nodiscard]] QString tabTitle() const;
    [[nodiscard]] const SessionData& session() const noexcept;

    [[nodiscard]] static AnalysisTab* create(const SessionData& session, QWidget* parent = nullptr);

signals:
    void preferredSizeChanged();
    void analysisStatusChanged(bool busy, const QString& message);

protected:
    SessionData m_session;
};



} // namespace pdv
