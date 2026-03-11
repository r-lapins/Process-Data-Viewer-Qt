#include "pdv/main_window.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QStatusBar>

namespace pdv {

MainWindow::MainWindow()
{
    resize(1000, 700);
    createMenu();
    statusBar()->showMessage("Ready");
    updateWindowTitle();
}

void MainWindow::createMenu()
{
    auto fileMenu = menuBar()->addMenu("File");
    auto openAction = fileMenu->addAction("Open");

    connect(openAction, &QAction::triggered,
            this, &MainWindow::openFile);
}

void MainWindow::openFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Open file", QString(), "Supported files (*.csv *.wav);;CSV files (*.csv);;WAV files (*.wav);;All files (*)"
        );

    if (filePath.isEmpty()) {
        statusBar()->showMessage("Open file canceled", 2000);
        return;
    }

    const LoadResult result = m_fileLoaderService.loadFile(filePath);

    if (!result.success) {
        m_currentSession.reset();
        updateWindowTitle();

        statusBar()->showMessage(
            QString("Failed to load file: %1").arg(result.errorMessage),
            5000
            );
        return;
    }

    m_currentSession = result.session;
    updateWindowTitle();

    const QFileInfo fileInfo(result.session.filePath);
    switch (result.session.kind){
    case pdv::SessionData::FileKind::Csv:
        statusBar()->showMessage(
            QString("Loaded CSV file: %1").arg(fileInfo.fileName()),
            5000
            );
        break;

    case pdv::SessionData::FileKind::Wav:
        statusBar()->showMessage(
            QString("Loaded WAV file: %1").arg(fileInfo.fileName()),
            5000
            );
        break;

    case pdv::SessionData::FileKind::Unknown:
        statusBar()->showMessage(
            QString("Loaded file: %1").arg(fileInfo.fileName()),
            5000
            );
        break;
    }
}

void MainWindow::updateWindowTitle()
{
    constexpr auto kAppTitle = "Process Data Viewer";

    if (!m_currentSession.has_value()) {
        setWindowTitle(kAppTitle);
        return;
    }

    const QFileInfo fileInfo(m_currentSession->filePath);
    setWindowTitle(QString("%1 - %2").arg(kAppTitle, fileInfo.fileName()));
}

} // namespace pdv
