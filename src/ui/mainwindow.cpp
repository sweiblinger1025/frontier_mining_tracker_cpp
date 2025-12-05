// Own header first
#include "mainwindow.h"
#include "./ui_mainwindow.h"

// Project headers
#include "dashboardwidget.h"
#include "financewidget.h"
#include "operationswidget.h"
#include "datahubwidget.h"
#include "auditorwidget.h"

// Qt headers
#include <QTabWidget>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Prevent window from being resized too small
    setMinimumSize(1024, 700);

    // Create Tab Widget
    QTabWidget *tabWidget = new QTabWidget(this);

    // Add the 5 main tabs
    tabWidget->addTab(new DashboardWidget(), QIcon(":/icons/icons/chart-bar.svg"), "Dashboard");
    tabWidget->addTab(new FinanceWidget(), QIcon(":/icons/icons/receipt-2.svg"), "Finance");
    tabWidget->addTab(new OperationsWidget(), QIcon(":/icons/icons/building.svg"), "Operations");
    tabWidget->addTab(new DataHubWidget(), QIcon(":/icons/icons/book.svg"), "Data Hub");
    tabWidget->addTab(new AuditorWidget(), QIcon(":/icons/icons/settings.svg"), "Auditor");

    // Set as central widget
    setCentralWidget(tabWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}
