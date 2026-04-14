#pragma once

#include <QObject>
#include <QFutureWatcher>
#include <QString>

#include "pdv/core/session_data.h"

#include "pdt/pipeline/wav_analysis_session.h"

#include <optional>
#include <memory>

namespace pdv {

class WavAnalysisController : public QObject
{
    Q_OBJECT

public:
    explicit WavAnalysisController(const SessionData& session, QObject* parent = nullptr);

    void recompute();

    void setSettings(const pdt::WavAnalysisSettingsCache& settings);
    [[nodiscard]] const pdt::WavAnalysisSettingsCache& settings() const noexcept;

    [[nodiscard]] bool isBusy() const noexcept;
    [[nodiscard]] bool hasResult() const noexcept;
    [[nodiscard]] const pdt::WavAnalysisResult& result() const;

    [[nodiscard]] const SessionData& session() const noexcept;

signals:
    void resultChanged(const pdt::WavAnalysisResult& result);
    void busyChanged(bool busy);
    void analysisFailed(const QString& message);

private:
    void startRecompute();
    void handleRecomputeFinished();

    const SessionData& m_session;
    pdt::WavAnalysisSettingsCache m_settings{};

    std::optional<pdt::WavAnalysisResult> m_result;
    std::unique_ptr<pdt::WavAnalysisSession> m_analysisSession;

    QFutureWatcher<pdt::WavAnalysisResult>* m_recomputeWatcher = nullptr;
    bool m_isBusy = false;
    bool m_hasPendingRecompute = false;
};

} // namespace pdv
