/**
 * @file vehiclespecstab.cpp
 * @brief Vehicle Specs tab implementation for Data Hub
 */

#include "vehiclespecstab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

VehicleSpecsTab::VehicleSpecsTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_model(nullptr)
    , m_proxyModel(nullptr)
{
    setupUi();
    loadCategories();
    loadVehicles();
}

VehicleSpecsTab::~VehicleSpecsTab()
{
}

void VehicleSpecsTab::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // === Filter Bar ===
    QHBoxLayout *filterLayout = new QHBoxLayout();

    // Category filter
    QLabel *categoryLabel = new QLabel("Category:");
    m_categoryFilter = new QComboBox();
    m_categoryFilter->setMinimumWidth(150);

    // Search box
    QLabel *searchLabel = new QLabel("Search:");
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Type to search...");
    m_searchBox->setClearButtonEnabled(true);

    filterLayout->addWidget(categoryLabel);
    filterLayout->addWidget(m_categoryFilter);
    filterLayout->addSpacing(20);
    filterLayout->addWidget(searchLabel);
    filterLayout->addWidget(m_searchBox, 1);

    mainLayout->addLayout(filterLayout);

    // === Table View ===
    m_tableView = new QTableView();
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);

    // Create model
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({
        "Name", "Category", "Bucket (m³)", "Truck (m³)", 
        "Tank (L)", "Fuel (L/hr)", "Price", "Status"
    });

    // Create proxy model for filtering/sorting
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);  // Search all columns

    m_tableView->setModel(m_proxyModel);

    mainLayout->addWidget(m_tableView, 1);

    // === Bottom Bar ===
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    m_vehicleCountLabel = new QLabel("Vehicles: 0");

    bottomLayout->addWidget(m_vehicleCountLabel);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);

    // === Connect Signals ===
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VehicleSpecsTab::onCategoryFilterChanged);

    connect(m_searchBox, &QLineEdit::textChanged,
            this, &VehicleSpecsTab::onSearchTextChanged);

    connect(m_tableView, &QTableView::doubleClicked,
            this, &VehicleSpecsTab::onVehicleDoubleClicked);
}

void VehicleSpecsTab::loadCategories()
{
    m_categoryFilter->clear();
    m_categoryFilter->addItem("All Categories", "");

    // Get unique main categories from vehicles
    QSet<QString> categories;
    auto vehicles = m_database->getAllVehicles();

    for (const auto &vehicle : vehicles) {
        if (!vehicle.categoryMain.isEmpty()) {
            categories.insert(vehicle.categoryMain);
        }
    }

    // Sort and add to combo box
    QStringList sortedCategories = categories.values();
    sortedCategories.sort();

    for (const QString &category : sortedCategories) {
        m_categoryFilter->addItem(category, category);
    }
}

void VehicleSpecsTab::loadVehicles()
{
    m_vehicles = m_database->getAllVehicles();
    populateTable(m_vehicles);
}

void VehicleSpecsTab::populateTable(const QVector<Frontier::Vehicle> &vehicles)
{
    m_model->removeRows(0, m_model->rowCount());

    for (const auto &vehicle : vehicles) {
        QList<QStandardItem*> row;

        // Name
        QStandardItem *nameItem = new QStandardItem(vehicle.name);
        nameItem->setData(vehicle.id, Qt::UserRole);  // Store ID for later use
        row.append(nameItem);

        // Category
        QString category = vehicle.categoryMain;
        if (!vehicle.categorySub.isEmpty()) {
            category += " - " + vehicle.categorySub;
        }
        row.append(new QStandardItem(category));

        // Bucket Capacity (m³)
        QStandardItem *bucketItem = new QStandardItem();
        if (vehicle.bucketCapacityM3 > 0) {
            bucketItem->setData(QString("%1").arg(vehicle.bucketCapacityM3, 0, 'f', 1), Qt::DisplayRole);
        } else {
            bucketItem->setData("-", Qt::DisplayRole);
        }
        bucketItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(bucketItem);

        // Truck Capacity (m³)
        QStandardItem *truckItem = new QStandardItem();
        if (vehicle.truckCapacityM3 > 0) {
            truckItem->setData(QString("%1").arg(vehicle.truckCapacityM3, 0, 'f', 1), Qt::DisplayRole);
        } else {
            truckItem->setData("-", Qt::DisplayRole);
        }
        truckItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(truckItem);

        // Tank Capacity (L)
        QStandardItem *tankItem = new QStandardItem();
        tankItem->setData(QString("%L1").arg(vehicle.tankCapacityL, 0, 'f', 0), Qt::DisplayRole);
        tankItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(tankItem);

        // Fuel Use (L/hr)
        QStandardItem *fuelItem = new QStandardItem();
        fuelItem->setData(QString("%1").arg(vehicle.fuelUseLPerHour, 0, 'f', 1), Qt::DisplayRole);
        fuelItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(fuelItem);

        // Purchase Price
        QStandardItem *priceItem = new QStandardItem();
        priceItem->setData(QString("$%L1").arg(vehicle.purchasePrice, 0, 'f', 0), Qt::DisplayRole);
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(priceItem);

        // Status
        QStandardItem *statusItem = new QStandardItem();
        statusItem->setData(vehicle.active ? "Active" : "Inactive", Qt::DisplayRole);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (!vehicle.active) {
            statusItem->setForeground(QBrush(Qt::gray));
        }
        row.append(statusItem);

        m_model->appendRow(row);
    }

    m_vehicleCountLabel->setText(QString("Vehicles: %1").arg(vehicles.size()));
    m_tableView->resizeColumnsToContents();
}

void VehicleSpecsTab::refreshData()
{
    loadCategories();
    loadVehicles();
}

void VehicleSpecsTab::onCategoryFilterChanged(int index)
{
    Q_UNUSED(index);

    QString selectedCategory = m_categoryFilter->currentData().toString();

    if (selectedCategory.isEmpty()) {
        // Show all vehicles
        loadVehicles();
    } else {
        // Filter by category
        QVector<Frontier::Vehicle> filtered;
        auto allVehicles = m_database->getAllVehicles();
        
        for (const auto &vehicle : allVehicles) {
            if (vehicle.categoryMain == selectedCategory) {
                filtered.append(vehicle);
            }
        }
        
        populateTable(filtered);
    }
}

void VehicleSpecsTab::onSearchTextChanged(const QString &text)
{
    m_proxyModel->setFilterRegularExpression(
        QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));

    m_vehicleCountLabel->setText(QString("Vehicles: %1 / %2")
                                  .arg(m_proxyModel->rowCount())
                                  .arg(m_model->rowCount()));
}

void VehicleSpecsTab::onVehicleDoubleClicked(const QModelIndex &index)
{
    // Get the source index
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    QString vehicleId = m_model->item(sourceIndex.row(), 0)->data(Qt::UserRole).toString();

    // Get the vehicle
    auto vehicleOpt = m_database->getVehicle(vehicleId);
    if (!vehicleOpt.has_value()) {
        return;
    }

    Frontier::Vehicle vehicle = vehicleOpt.value();

    // Show details in a message box (can be expanded to a dialog later)
    QString details = QString(
        "<b>%1</b><br><br>"
        "<table>"
        "<tr><td>ID:</td><td>%2</td></tr>"
        "<tr><td>Category:</td><td>%3 - %4</td></tr>"
        "<tr><td>Bucket Capacity:</td><td>%5 m³</td></tr>"
        "<tr><td>Truck Capacity:</td><td>%6 m³</td></tr>"
        "<tr><td>Tank Capacity:</td><td>%7 L</td></tr>"
        "<tr><td>Fuel Consumption:</td><td>%8 L/hr</td></tr>"
        "<tr><td>Purchase Price:</td><td>$%L9</td></tr>"
        "<tr><td>Status:</td><td>%10</td></tr>"
        "</table>"
        "%11"
    ).arg(vehicle.name)
     .arg(vehicle.id)
     .arg(vehicle.categoryMain)
     .arg(vehicle.categorySub)
     .arg(vehicle.bucketCapacityM3, 0, 'f', 1)
     .arg(vehicle.truckCapacityM3, 0, 'f', 1)
     .arg(vehicle.tankCapacityL, 0, 'f', 0)
     .arg(vehicle.fuelUseLPerHour, 0, 'f', 1)
     .arg(vehicle.purchasePrice, 0, 'f', 0)
     .arg(vehicle.active ? "Active" : "Inactive")
     .arg(vehicle.notes.isEmpty() ? "" : QString("<br><br><b>Notes:</b><br>%1").arg(vehicle.notes));

    QMessageBox::information(this, "Vehicle Details", details);
}
