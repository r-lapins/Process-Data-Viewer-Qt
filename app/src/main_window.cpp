#include "pdv/main_window.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QStatusBar>

namespace pdv {

MainWindow::MainWindow()
{
    resize(1000, 700);
    createMenu();
    statusBar()->showMessage("Ready");
}

void MainWindow::createMenu()
{
    auto fileMenu = menuBar()->addMenu("File");
    auto openAction = fileMenu->addAction("Open");

    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
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
        statusBar()->showMessage(QString("Failed to load file: %1").arg(result.errorMessage), 5000);
        return;
    }

    statusBar()->showMessage(QString("Selected file: %1").arg(result.session.filePath));
}

} // namespace pdv
