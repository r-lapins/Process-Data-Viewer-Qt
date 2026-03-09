#include <QApplication>
#include "pdv/main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    pdv::MainWindow window;
    window.show();

    return app.exec();
}