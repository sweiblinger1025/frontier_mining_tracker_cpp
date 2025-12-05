#include "ui/mainwindow.h"

#include <QApplication>




int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Frontier Mining Tracker");
    QApplication::setOrganizationName("FrontierSuite");
    QApplication::setApplicationVersion("0.1.0");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
