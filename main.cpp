#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("ONBexplorer");

    QSurfaceFormat format;
    format.setSamples(4);
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
