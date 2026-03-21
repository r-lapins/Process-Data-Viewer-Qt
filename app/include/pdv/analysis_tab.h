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

    QString style = R"(
    QPushButton {
        padding: 4px 10px;
    }
    QPushButton:checked {
        background-color: #2E7D32;
        color: white;
        border: 1px solid #1B5E20;
    })";

signals:
    void preferredSizeChanged();

protected:
    SessionData m_session;
};



} // namespace pdv
