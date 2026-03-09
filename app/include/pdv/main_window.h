#pragma once

#include <QMainWindow>

#include "pdv/file_loader_service.h"

namespace pdv {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private:
    void createMenu();
    void openFile();

    FileLoaderService m_fileLoaderService;
};

} // namespace pdv
