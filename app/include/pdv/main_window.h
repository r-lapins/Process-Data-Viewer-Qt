#pragma once

#include <QMainWindow>
#include <QFutureWatcher>

class QAction;
class QTabWidget;

#include "pdv/file_loader_service.h"

namespace pdv {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private:
    void createMenu();
    void createToolbar();
    void createCentralWorkspace();

    void openFile();
    void openFileFromDataFolder();

    void loadFileAsync(const QString& filePath);
    void handleLoadFinished();
    void setLoadingUiState(bool loading);
    void updateWindowTitle();

    void growToFitCurrentTab();

    FileLoaderService m_fileLoaderService;

    QAction* m_openAction = nullptr;
    QAction* m_quickOpenAction = nullptr;
    QFutureWatcher<LoadResult>* m_loadWatcher = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    bool m_isLoading = false;
};

} // namespace pdv
