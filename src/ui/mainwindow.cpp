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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Prevent window from being resized too small
    setMinimumSize(1024, 700);

    // Create Tab Widget
    m_tabWidget = new QTabWidget(this);

    // Add the 5 main tabs
    m_tabWidget->addTab(new DashboardWidget(), QIcon(":/icons/icons/chart-bar.svg"), "Dashboard");
    m_tabWidget->addTab(new FinanceWidget(), QIcon(":/icons/icons/receipt-2.svg"), "Finance");
    m_tabWidget->addTab(new OperationsWidget(), QIcon(":/icons/icons/building.svg"), "Operations");
    m_tabWidget->addTab(new DataHubWidget(), QIcon(":/icons/icons/book.svg"), "Data Hub");
    m_tabWidget->addTab(new AuditorWidget(), QIcon(":/icons/icons/settings.svg"), "Auditor");

    // Set as central widget
    setCentralWidget(m_tabWidget);


    // Create Menu Bar
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File Menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    QAction *newAction = fileMenu->addAction("&New Session");
    newAction->setShortcut(QKeySequence::New);

    QAction *openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);

    QAction *saveAsAction = fileMenu->addAction("&Save &As...");
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);

    // Edit Menu
    QMenu *editMenu = menuBar->addMenu("&Edit");

    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);

    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);

    // View Menu
    QMenu *viewMenu = menuBar->addMenu("&View");

    QAction *dashboardAction = viewMenu->addAction("&Dashboard");
    dashboardAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));

    QAction *financeAction = viewMenu->addAction("&Finance");
    financeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));

    QAction *operationsAction = viewMenu->addAction("&Operations");
    operationsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));

    QAction *dataHubAction = viewMenu->addAction("Data &Hub");
    dataHubAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));

    QAction *auditorAction = viewMenu->addAction("&Auditor");
    auditorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_5));

    // Help Menu
    QMenu *helpMenu = menuBar->addMenu("&Help");

    QAction *aboutAction = helpMenu->addAction("&About");

    // Connect View menu actions to tab switching
    connect(dashboardAction, &QAction::triggered, this, [this]() {
       m_tabWidget->setCurrentIndex(0);
    });

    connect(financeAction, &QAction::triggered, this, [this]() {
       m_tabWidget->setCurrentIndex(1);
    });

    connect(operationsAction, &QAction::triggered, this, [this]() {
       m_tabWidget->setCurrentIndex(2);
    });

    connect(dataHubAction, &QAction::triggered, this, [this]() {
       m_tabWidget->setCurrentIndex(3);
    });

    connect(auditorAction, &QAction::triggered, this, [this]() {
       m_tabWidget->setCurrentIndex(4);
    });

    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
}

MainWindow::~MainWindow()
{
    delete ui;
}
