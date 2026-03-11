#pragma once

#include <QMainWindow>

#include <optional>

class QLabel;

#include "pdv/file_loader_service.h"
#include "pdv/session_data.h"

namespace pdv {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private:
    void createMenu();
    void createCentralWorkspace();
    void openFile();
    void updateWindowTitle();

    FileLoaderService m_fileLoaderService;
    std::optional<SessionData> m_currentSession;

    QLabel* m_dataPlaceholderLabel = nullptr;
    QLabel* m_statisticsPlaceholderLabel = nullptr;
    QLabel* m_alertsPlaceholderLabel = nullptr;
};

} // namespace pdv
