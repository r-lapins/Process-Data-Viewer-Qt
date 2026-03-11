#pragma once

#include <QMainWindow>

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
    void openFile();
    void updateWindowTitle();

    FileLoaderService m_fileLoaderService;
    std::optional<SessionData> m_currentSession;
};

} // namespace pdv
