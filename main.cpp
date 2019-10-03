#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Neurobotics");
    QCoreApplication::setApplicationName("ONBexplorer");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
