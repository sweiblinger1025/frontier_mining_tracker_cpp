#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>

#include "core/database.h"
#include "core/operationsmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DataHubWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Frontier::Database *database, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onImportItems();
    void onImportVehicles();
    void onImportRecipes();
    void onImportLocations();

private:
    Ui::MainWindow *ui;

    // Database reference
    Frontier::Database *m_database;

    // Operations manager
    Frontier::OperationsManager *m_operationsManager;

    // Tab widget
    QTabWidget *m_tabWidget;

    // Data Hub reference for refresh
    DataHubWidget *m_dataHubWidget;

    // Status bar widgets
    QLabel *m_dayLabel;
    QLabel *m_balanceLabel;

    // Import actions
    QAction *m_importItemsAction;
    QAction *m_importVehiclesAction;
    QAction *m_importRecipesAction;
    QAction *m_importLocationsAction;
};

#endif // MAINWINDOW_H
