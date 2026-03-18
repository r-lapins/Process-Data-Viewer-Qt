#include "pdv/main_window.h"
#include "pdv/analysis_tab.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QtConcurrent/QtConcurrentRun>

namespace pdv {

MainWindow::MainWindow()
{
    resize(1000, 700);

    m_loadWatcher = new QFutureWatcher<LoadResult>(this);
    connect(m_loadWatcher, &QFutureWatcher<LoadResult>::finished,
            this, &MainWindow::handleLoadFinished);

    createMenu();
    createToolbar();
    createCentralWorkspace();

    statusBar()->showMessage("Ready");
    updateWindowTitle();
}

void MainWindow::createMenu()
{
    auto* fileMenu = menuBar()->addMenu("File");
    m_openAction = fileMenu->addAction("Open");

    connect(m_openAction, &QAction::triggered,
            this, &MainWindow::openFile);
}

void MainWindow::createToolbar()
{
    auto* toolbar = addToolBar("Quick Access");
    toolbar->setMovable(false);

    m_quickOpenAction = toolbar->addAction("Quick Open");

    connect(m_quickOpenAction, &QAction::triggered,
            this, &MainWindow::openFileFromDataFolder);
}

void MainWindow::createCentralWorkspace()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setDocumentMode(true);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, [this](int index) {
                QWidget* page = m_tabWidget->widget(index);
                m_tabWidget->removeTab(index);
                delete page;
                updateWindowTitle();
            });

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, [this](int) {
                updateWindowTitle();
            });

    setCentralWidget(m_tabWidget);
}

void MainWindow::openFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open file",
        QString(),
        "Supported files (*.csv *.wav);;CSV files (*.csv);;WAV files (*.wav);;All files (*)"
        );

    if (filePath.isEmpty()) {
        statusBar()->showMessage("Open file canceled", 2000);
        return;
    }

    loadFileAsync(filePath);
}

void MainWindow::openFileFromDataFolder()
{
    const QString startDir = QDir::homePath() + "/Documents/QtProjekty/process_data_viewer_qt/examples/";

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open file from data folder",
        startDir,
        "Supported files (*.csv *.wav);;CSV files (*.csv);;WAV files (*.wav);;All files (*)"
        );

    if (filePath.isEmpty()) {
        statusBar()->showMessage("Open file canceled", 2000);
        return;
    }

    loadFileAsync(filePath);
}

void MainWindow::loadFileAsync(const QString &filePath)
{
    if (m_isLoading) {
        return;
    }

    setLoadingUiState(true);

    auto future = QtConcurrent::run([this, filePath]() {
        return m_fileLoaderService.loadFile(filePath);
    });

    m_loadWatcher->setFuture(future);
}

void MainWindow::handleLoadFinished()
{
    setLoadingUiState(false);

    const LoadResult result = m_loadWatcher->result();

    if (!result.success) {
        statusBar()->showMessage(
            QString("Failed to load file: %1").arg(result.errorMessage),
            5000
            );
        return;
    }

    auto* tab = new AnalysisTab(result.session, m_tabWidget);
    const int index = m_tabWidget->addTab(tab, tab->tabTitle());
    m_tabWidget->setCurrentIndex(index);

    statusBar()->showMessage(
        QString("Loaded file: %1").arg(tab->tabTitle()),
        3000
        );

    updateWindowTitle();
}

void MainWindow::setLoadingUiState(bool loading)
{
    m_isLoading = loading;

    if (m_openAction != nullptr) {
        m_openAction->setEnabled(!loading);
    }

    if (m_quickOpenAction != nullptr) {
        m_quickOpenAction->setEnabled(!loading);
    }

    if (loading) {
        statusBar()->showMessage("Loading file...");
        QApplication::setOverrideCursor(Qt::WaitCursor);
    } else {
        QApplication::restoreOverrideCursor();
    }
}

void MainWindow::updateWindowTitle()
{
    constexpr auto kAppTitle = "Process Data Viewer";

    if (m_tabWidget == nullptr || m_tabWidget->count() == 0) {
        setWindowTitle(kAppTitle);
        return;
    }

    const int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex < 0) {
        setWindowTitle(kAppTitle);
        return;
    }

    setWindowTitle(QString("%1 - %2").arg(kAppTitle, m_tabWidget->tabText(currentIndex)));
}

} // namespace pdv

