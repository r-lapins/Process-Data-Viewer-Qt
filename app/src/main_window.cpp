#include "pdv/main_window.h"

#include <QMenuBar>
#include <QFileDialog>

namespace pdv {

MainWindow::MainWindow()
{
    resize(1000, 700);
    createMenu();
}

void MainWindow::createMenu()
{
    auto fileMenu = menuBar()->addMenu("File");

    auto openAction = fileMenu->addAction("Open");

    connect(openAction, &QAction::triggered, this, [this]() {
        QFileDialog::getOpenFileName(this, "Open file");
    });
}

}