#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("vova_ok");
    QCoreApplication::setApplicationName("ONBexplorer");

    QSurfaceFormat format;
    format.setSamples(4);
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);

    QStringList arguments = QCoreApplication::arguments();
    bool minim = arguments.contains("--minimized");

    MainWindow w(minim);

    if (minim)
        w.showMinimized();
    else
        w.show();
    
    return a.exec();
}
