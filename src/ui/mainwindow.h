#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>

#include "core/database.h"
#include "core/operationsmanager.h"

// Forward declarations
class DataHubWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Frontier::Database *database, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onImportItems();
    void onImportVehicles();

private:
    Ui::MainWindow *ui;

    // Database reference
    Frontier::Database *m_database;

    // Operations manager (owns this)
    Frontier::OperationsManager *m_operationsManager;

    // Tab widget
    QTabWidget *m_tabWidget;

    // Keep reference to Data Hub for refreshing after imports
    DataHubWidget *m_dataHubWidget;

    // Status bar widgets
    QLabel *m_dayLabel;
    QLabel *m_balanceLabel;

    // Menu actions
    QAction *m_importItemsAction;
    QAction *m_importVehiclesAction;
};

#endif // MAINWINDOW_H
