// Own header first
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "core/itemimporter.h"
#include "core/vehicleimporter.h"

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
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

MainWindow::MainWindow(Frontier::Database *database, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_database(database)
    , m_operationsManager(nullptr)
{
    ui->setupUi(this);

    // Set window properties
    setWindowTitle("Frontier Mining Tracker");
    setMinimumSize(1024, 700);
    resize(1280, 800);

    // Create Operations Manager
    m_operationsManager = new Frontier::OperationsManager(m_database, this);

    // === Create Tab Widget ===
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    // Create tab widgets
    DashboardWidget *dashboardWidget = new DashboardWidget(this);
    FinanceWidget *financeWidget = new FinanceWidget(m_database, this);
    OperationsWidget *operationsWidget = new OperationsWidget(m_operationsManager, this);
    DataHubWidget *dataHubWidget = new DataHubWidget(m_database, this);
    AuditorWidget *auditorWidget = new AuditorWidget(m_database, this);

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

    // Import submenu
    QMenu *importMenu = fileMenu->addMenu("&Import");

    m_importItemsAction = new QAction("&Items...", this);
    m_importItemsAction->setStatusTip("Import items from JSON file");
    connect(m_importItemsAction, &QAction::triggered,
            this, &MainWindow::onImportItems);
    importMenu->addAction(m_importItemsAction);

    m_importVehiclesAction = new QAction("&Vehicles...", this);
    m_importVehiclesAction->setStatusTip("Import vehicles from JSON file");
    connect(m_importVehiclesAction, &QAction::triggered,
            this, &MainWindow::onImportVehicles);
    importMenu->addAction(m_importVehiclesAction);

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

void MainWindow::onImportItems()
{
    // Start in the application's data directory if it exists
    QString startDir = QCoreApplication::applicationDirPath() + "/data";
    if (!QDir(startDir).exists()) {
        startDir = QDir::homePath();
    }

    QString jsonPath = QFileDialog::getOpenFileName(
        this,
        "Select Item Data File",
        startDir,
        "JSON Files (*.json);;All Files (*)"
    );

    if (jsonPath.isEmpty()) {
        return;  // User cancelled
    }

    // Ask about clearing existing data
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Import Items",
        "Do you want to replace all existing items?\n\n"
        "Yes = Clear existing and import new\n"
        "No = Add to existing items",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
    );

    if (reply == QMessageBox::Cancel) {
        return;
    }

    bool clearExisting = (reply == QMessageBox::Yes);

    // Perform import
    int count = Frontier::ItemImporter::importFromJson(
        jsonPath, m_database, clearExisting
    );

    if (count > 0) {
        QMessageBox::information(
            this,
            "Import Complete",
            QString("Successfully imported %1 items.").arg(count)
        );

        statusBar()->showMessage(QString("Imported %1 items").arg(count), 5000);

        // Refresh Data Hub if it's the current tab
        // The Data Hub will need to reload its data
    } else if (count == 0) {
        QMessageBox::warning(
            this,
            "Import Warning",
            "No items were imported. The file may be empty."
        );
    } else {
        QMessageBox::critical(
            this,
            "Import Failed",
            "Could not import items from the selected file.\n\n"
            "Please ensure the file is a valid items JSON file."
        );
    }
}

void MainWindow::onImportVehicles()
{
    // Start in the application's data directory if it exists
    QString startDir = QCoreApplication::applicationDirPath() + "/data";
    if (!QDir(startDir).exists()) {
        startDir = QDir::homePath();
    }

    QString jsonPath = QFileDialog::getOpenFileName(
        this,
        "Select Vehicle Data File",
        startDir,
        "JSON Files (*.json);;All Files (*)"
    );

    if (jsonPath.isEmpty()) {
        return;  // User cancelled
    }

    // Ask about clearing existing data
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Import Vehicles",
        "Do you want to replace all existing vehicles?\n\n"
        "Yes = Clear existing and import new\n"
        "No = Merge with existing (update if ID exists)",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
    );

    if (reply == QMessageBox::Cancel) {
        return;
    }

    bool clearExisting = (reply == QMessageBox::Yes);

    // Perform import
    int count = Frontier::VehicleImporter::importFromJson(
        jsonPath, m_database, clearExisting
    );

    if (count > 0) {
        QMessageBox::information(
            this,
            "Import Complete",
            QString("Successfully imported %1 vehicles.").arg(count)
        );

        statusBar()->showMessage(QString("Imported %1 vehicles").arg(count), 5000);
    } else if (count == 0) {
        QMessageBox::warning(
            this,
            "Import Warning",
            "No vehicles were imported. The file may be empty."
        );
    } else {
        QMessageBox::critical(
            this,
            "Import Failed",
            "Could not import vehicles from the selected file.\n\n"
            "Please ensure the file is a valid vehicles JSON file."
        );
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
