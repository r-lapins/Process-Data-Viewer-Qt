#pragma once

#include <QMainWindow>

namespace pdv {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private:
    void createMenu();
};

}