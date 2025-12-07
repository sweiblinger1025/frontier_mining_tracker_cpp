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
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>

MainWindow::MainWindow(Frontier::Database *database, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_database(database)
{
    ui->setupUi(this);

    // Set window properties
    setWindowTitle("Frontier Mining Tracker");
    setMinimumSize(1024, 700);
    resize(1280, 800);

    // === Create Tab Widget ===
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    // Create tab widgets - pass database to DataHub
    DashboardWidget *dashboardWidget = new DashboardWidget(this);
    FinanceWidget *financeWidget = new FinanceWidget(m_database, this);
    OperationsWidget *operationsWidget = new OperationsWidget(this);
    DataHubWidget *dataHubWidget = new DataHubWidget(m_database, this);
    AuditorWidget *auditorWidget = new AuditorWidget(this);

    // Add tabs with icons
    m_tabWidget->addTab(dashboardWidget, QIcon(":/icons/icons/chart-bar.svg"), "Dashboard");
    m_tabWidget->addTab(financeWidget, QIcon(":/icons/icons/receipt-2.svg"), "Finance");
    m_tabWidget->addTab(operationsWidget, QIcon(":/icons/icons/building.svg"), "Operations");
    m_tabWidget->addTab(dataHubWidget, QIcon(":/icons/icons/book.svg"), "Data Hub");
    m_tabWidget->addTab(auditorWidget, QIcon(":/icons/icons/settings.svg"), "Auditor");

    // === Menu Bar ===
    // File Menu
    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *newAction = fileMenu->addAction("&New Session");
    newAction->setShortcut(QKeySequence("Ctrl+N"));

    QAction *openAction = fileMenu->addAction("&Open");
    openAction->setShortcut(QKeySequence::Open);

    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);

    QAction *saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Alt+F4"));
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit Menu
    QMenu *editMenu = menuBar()->addMenu("&Edit");

    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);

    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);

    // View Menu
    QMenu *viewMenu = menuBar()->addMenu("&View");

    QAction *dashboardAction = viewMenu->addAction("&Dashboard");
    dashboardAction->setShortcut(QKeySequence("Ctrl+1"));
    connect(dashboardAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(0);
    });

    QAction *financeAction = viewMenu->addAction("&Finance");
    financeAction->setShortcut(QKeySequence("Ctrl+2"));
    connect(financeAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(1);
    });

    QAction *operationsAction = viewMenu->addAction("&Operations");
    operationsAction->setShortcut(QKeySequence("Ctrl+3"));
    connect(operationsAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(2);
    });

    QAction *dataHubAction = viewMenu->addAction("Data &Hub");
    dataHubAction->setShortcut(QKeySequence("Ctrl+4"));
    connect(dataHubAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(3);
    });

    QAction *auditorAction = viewMenu->addAction("&Auditor");
    auditorAction->setShortcut(QKeySequence("Ctrl+5"));
    connect(auditorAction, &QAction::triggered, this, [this]() {
        m_tabWidget->setCurrentIndex(4);
    });

    // Help Menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");

    // === Status Bar ===
    statusBar()->showMessage("Ready");

    m_dayLabel = new QLabel("Day: 1");
    m_balanceLabel = new QLabel("Balance: $10,000");

    statusBar()->addPermanentWidget(m_dayLabel);
    statusBar()->addPermanentWidget(m_balanceLabel);
}

MainWindow::~MainWindow()
{
    delete ui;
}
