#include "ui/mainwindow.h"
#include "core/database.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set application metadata
    QCoreApplication::setApplicationName("Frontier Mining Tracker");
    QCoreApplication::setOrganizationName("Frontier");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Initialize database (this connects AND creates tables)
    Frontier::Database db;
    if (!db.initialize("frontier_mining.db")) {
        qDebug() << "Failed to initialize database:" << db.lastError();
        return 1;
    }

    // Show database status
    auto items = db.getAllItems();
    qDebug() << "";
    qDebug() << "=== Frontier Mining Tracker ===";
    qDebug() << "Database connected:" << items.size() << "items loaded";
    qDebug() << "================================";
    qDebug() << "";

    // Pass database to MainWindow
    MainWindow w(&db);
    w.show();
    return a.exec();
}
