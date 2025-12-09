// Own header first
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "core/itemimporter.h"
#include "core/vehicleimporter.h"
#include "core/recipeimporter.h"
#include "core/locationimporter.h"

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

MainWindow::MainWindow(Frontier::Database *database, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_database(database)
    , m_operationsManager(new Frontier::OperationsManager(database, this))
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
    OperationsWidget *operationsWidget = new OperationsWidget(m_operationsManager, this);
    m_dataHubWidget = new DataHubWidget(m_database, this);
    AuditorWidget *auditorWidget = new AuditorWidget(m_database, this);

    // Add tabs with icons
    m_tabWidget->addTab(dashboardWidget, QIcon(":/icons/icons/chart-bar.svg"), "Dashboard");
    m_tabWidget->addTab(financeWidget, QIcon(":/icons/icons/receipt-2.svg"), "Finance");
    m_tabWidget->addTab(operationsWidget, QIcon(":/icons/icons/building.svg"), "Operations");
    m_tabWidget->addTab(m_dataHubWidget, QIcon(":/icons/icons/book.svg"), "Data Hub");
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

    m_importRecipesAction = new QAction("&Recipes...", this);
    m_importRecipesAction->setStatusTip("Import recipes/workbenches from JSON file");
    connect(m_importRecipesAction, &QAction::triggered,
            this, &MainWindow::onImportRecipes);
    importMenu->addAction(m_importRecipesAction);

    m_importLocationsAction = new QAction("&Locations...", this);
    m_importLocationsAction->setStatusTip("Import Locations from JSON file");
    connect(m_importLocationsAction, &QAction::triggered,
            this, &MainWindow::onImportLocations);
    importMenu->addAction(m_importLocationsAction);

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
        "Select Items Data File",
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
        "No = Merge with existing (update if name exists)",
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

        // Refresh Data Hub
        m_dataHubWidget->refreshData();

        statusBar()->showMessage(QString("Imported %1 items").arg(count), 5000);
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

        // Refresh Data Hub
        m_dataHubWidget->refreshData();

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

void MainWindow::onImportRecipes()
{
    // Start in the application's data directory if it exists
    QString startDir = QCoreApplication::applicationDirPath() + "/data";
    if (!QDir(startDir).exists()) {
        startDir = QDir::homePath();
    }

    QString jsonPath = QFileDialog::getOpenFileName(
        this,
        "Select Workbenches/Recipes File",
        startDir,
        "JSON Files (*.json);;All Files (*)"
        );

    if (jsonPath.isEmpty()) {
        return;  // User cancelled
    }

    // Confirm import (recipes are always replaced)
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Import Recipes",
        "This will replace all existing recipes and workbenches.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Perform import
    RecipeImporter importer(m_database);
    bool success = importer.importFromJson(jsonPath);

    if (success) {
        QMessageBox::information(
            this,
            "Import Complete",
            QString("Successfully imported:\n"
                    "• %1 workbenches\n"
                    "• %2 recipes")
                .arg(importer.workbenchesImported())
                .arg(importer.recipesImported())
            );

        // Refresh Data Hub to show new recipes
        m_dataHubWidget->refreshData();

        statusBar()->showMessage(
            QString("Imported %1 recipes from %2 workbenches")
                .arg(importer.recipesImported())
                .arg(importer.workbenchesImported()),
            5000
            );
    } else {
        QMessageBox::critical(
            this,
            "Import Failed",
            QString("Could not import recipes.\n\nError: %1")
                .arg(importer.lastError())
            );
    }
}

void MainWindow::onImportLocations()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Locations Data Directory"),
        QString(),
        QFileDialog::ShowDirsOnly
        );

    if (dir.isEmpty()) {
        return;
    }

    // Check required files exist
    QDir locDir(dir);
    if (!locDir.exists("maps.json") ||
        !locDir.exists("location_types.json") ||
        !locDir.exists("locations.json")) {
        QMessageBox::warning(this, tr("Missing Files"),
                             tr("The selected directory must contain:\n"
                                "- maps.json\n"
                                "- location_types.json\n"
                                "- locations.json"));
        return;
    }

    bool success = Frontier::LocationImporter::importFromDirectory(dir, m_database, true);

    if (success) {
        QMessageBox::information(this, tr("Import Complete"),
                                 tr("Imported:\n"
                                    "- %1 maps\n"
                                    "- %2 location types\n"
                                    "- %3 locations")
                                     .arg(Frontier::LocationImporter::mapsImported())
                                     .arg(Frontier::LocationImporter::typesImported())
                                     .arg(Frontier::LocationImporter::locationsImported()));

        // Refresh the Data Hub if it's visible
        // You may need to emit a signal or call refreshData() on the LocationsTab
    } else {
        QMessageBox::warning(this, tr("Import Failed"),
                             tr("Failed to import locations:\n%1")
                                 .arg(Frontier::LocationImporter::lastError()));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
