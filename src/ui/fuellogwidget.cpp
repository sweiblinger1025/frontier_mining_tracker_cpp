/**
 * @file fuellogwidget.cpp
 * @brief Implementation of Fuel & Fluids Log subtab
 */

#include "fuellogwidget.h"
#include "core/unitconverter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>

FuelLogWidget::FuelLogWidget(Frontier::OperationsManager *manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    setupUi();
    loadEquipmentFilter();
    loadFuelLog();

    // Connect to manager signals
    connect(m_manager, &Frontier::OperationsManager::unitSystemChanged,
            this, &FuelLogWidget::onUnitSystemChanged);
    connect(m_manager, &Frontier::OperationsManager::fuelLogUpdated,
            this, &FuelLogWidget::onFuelLogUpdated);
}

FuelLogWidget::~FuelLogWidget()
{
}

void FuelLogWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // === Filter Controls ===
    QGroupBox *filterGroup = new QGroupBox("Filters");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);

    // Date range
    QLabel *fromLabel = new QLabel("From:");
    m_fromDateEdit = new QDateEdit();
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDate(QDate::currentDate().addMonths(-1));
    m_fromDateEdit->setDisplayFormat("yyyy-MM-dd");

    QLabel *toLabel = new QLabel("To:");
    m_toDateEdit = new QDateEdit();
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDate(QDate::currentDate());
    m_toDateEdit->setDisplayFormat("yyyy-MM-dd");

    // Equipment filter
    QLabel *equipLabel = new QLabel("Equipment:");
    m_equipmentCombo = new QComboBox();
    m_equipmentCombo->setMinimumWidth(200);

    // Refresh button
    m_refreshButton = new QPushButton("Refresh");

    filterLayout->addWidget(fromLabel);
    filterLayout->addWidget(m_fromDateEdit);
    filterLayout->addSpacing(10);
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(m_toDateEdit);
    filterLayout->addSpacing(20);
    filterLayout->addWidget(equipLabel);
    filterLayout->addWidget(m_equipmentCombo);
    filterLayout->addStretch();
    filterLayout->addWidget(m_refreshButton);

    mainLayout->addWidget(filterGroup);

    // === Table View ===
    QGroupBox *tableGroup = new QGroupBox("Fuel Log Entries");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableGroup);

    m_tableView = new QTableView();
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);

    m_model = new QStandardItemModel(this);
    m_tableView->setModel(m_model);

    tableLayout->addWidget(m_tableView);

    mainLayout->addWidget(tableGroup, 1);

    // === Summary ===
    QGroupBox *summaryGroup = new QGroupBox("Summary");
    QHBoxLayout *summaryLayout = new QHBoxLayout(summaryGroup);

    m_entryCountLabel = new QLabel("Entries: 0");
    m_totalFuelLabel = new QLabel("Total Fuel: 0");
    m_totalFuelLabel->setStyleSheet("font-weight: bold;");
    m_totalCostLabel = new QLabel("Total Cost: $0.00");
    m_totalCostLabel->setStyleSheet("font-weight: bold; color: green;");

    summaryLayout->addWidget(m_entryCountLabel);
    summaryLayout->addSpacing(30);
    summaryLayout->addWidget(m_totalFuelLabel);
    summaryLayout->addSpacing(30);
    summaryLayout->addWidget(m_totalCostLabel);
    summaryLayout->addStretch();

    mainLayout->addWidget(summaryGroup);

    // === Connections ===
    connect(m_refreshButton, &QPushButton::clicked,
            this, &FuelLogWidget::onRefreshClicked);
    connect(m_fromDateEdit, &QDateEdit::dateChanged,
            this, &FuelLogWidget::onFilterChanged);
    connect(m_toDateEdit, &QDateEdit::dateChanged,
            this, &FuelLogWidget::onFilterChanged);
    connect(m_equipmentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FuelLogWidget::onFilterChanged);
}

void FuelLogWidget::loadEquipmentFilter()
{
    m_equipmentCombo->clear();
    m_equipmentCombo->addItem("All Equipment", "");

    auto vehicles = m_manager->getAllVehicles();
    for (const auto &vehicle : vehicles) {
        m_equipmentCombo->addItem(
            QString("%1 (%2)").arg(vehicle.name, vehicle.categoryMain),
            vehicle.id
            );
    }
}

void FuelLogWidget::loadFuelLog()
{
    using UC = Frontier::UnitConverter;
    Frontier::UnitSystem units = m_manager->unitSystem();

    // Clear existing data
    m_model->clear();

    // Set headers based on unit system
    QStringList headers;
    headers << "Date/Time"
            << "Equipment"
            << QString("Fuel (%1)").arg(UC::fuelUnitLabel(units))
            << "Unit Price"
            << "Total Cost"
            << "Hours/Meter"
            << "Source"
            << "Notes";
    m_model->setHorizontalHeaderLabels(headers);

    // Get date range
    QDateTime fromDateTime(m_fromDateEdit->date(), QTime(0, 0, 0));
    QDateTime toDateTime(m_toDateEdit->date(), QTime(23, 59, 59));

    // Get equipment filter
    QString equipmentId = m_equipmentCombo->currentData().toString();

    // Fetch data
    auto entries = m_manager->getFuelLog(fromDateTime, toDateTime, equipmentId);

    // Populate table
    for (const auto &entry : entries) {
        QList<QStandardItem*> row;

        // Date/Time
        row << new QStandardItem(entry.dateTime.toString("yyyy-MM-dd hh:mm"));

        // Equipment name (lookup)
        QString equipName = entry.equipmentId;
        auto vehicle = m_manager->getVehicle(entry.equipmentId);
        if (vehicle.has_value()) {
            equipName = vehicle->name;
        }
        row << new QStandardItem(equipName);

        // Fuel amount (converted to display units)
        double displayFuel = UC::fuelToDisplay(entry.liters, units);
        QStandardItem *fuelItem = new QStandardItem(QString::number(displayFuel, 'f', 0));
        fuelItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row << fuelItem;

        // Unit price
        QStandardItem *priceItem = new QStandardItem(
            entry.unitPrice > 0 ? QString("$%1").arg(entry.unitPrice, 0, 'f', 2) : "-"
            );
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row << priceItem;

        // Total cost
        QStandardItem *costItem = new QStandardItem(
            entry.totalCost > 0 ? QString("$%1").arg(entry.totalCost, 0, 'f', 0) : "-"
            );
        costItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        costItem->setForeground(Qt::darkGreen);
        row << costItem;

        // Hours/Meter
        QStandardItem *hoursItem = new QStandardItem(
            entry.meterOrHours > 0 ? QString::number(entry.meterOrHours, 'f', 1) : "-"
            );
        hoursItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row << hoursItem;

        // Source
        row << new QStandardItem(entry.source);

        // Notes
        row << new QStandardItem(entry.notes);

        m_model->appendRow(row);
    }

    // Resize columns
    m_tableView->resizeColumnsToContents();

    // Update summary
    updateSummary();
}

void FuelLogWidget::updateSummary()
{
    using UC = Frontier::UnitConverter;
    Frontier::UnitSystem units = m_manager->unitSystem();

    // Get date range
    QDateTime fromDateTime(m_fromDateEdit->date(), QTime(0, 0, 0));
    QDateTime toDateTime(m_toDateEdit->date(), QTime(23, 59, 59));

    // Calculate totals
    double totalLiters = m_manager->getTotalFuelInRange(fromDateTime, toDateTime);
    double totalCost = m_manager->getTotalFuelCostInRange(fromDateTime, toDateTime);
    int entryCount = m_model->rowCount();

    // Display in user's units
    double displayFuel = UC::fuelToDisplay(totalLiters, units);

    m_entryCountLabel->setText(QString("Entries: %1").arg(entryCount));
    m_totalFuelLabel->setText(QString("Total Fuel: %1 %2")
                                  .arg(displayFuel, 0, 'f', 0)
                                  .arg(UC::fuelUnitLabel(units)));
    m_totalCostLabel->setText(QString("Total Cost: $%1").arg(totalCost, 0, 'f', 0));
}

void FuelLogWidget::onRefreshClicked()
{
    loadFuelLog();
}

void FuelLogWidget::onFilterChanged()
{
    loadFuelLog();
}

void FuelLogWidget::onUnitSystemChanged(Frontier::UnitSystem system)
{
    Q_UNUSED(system);
    loadFuelLog();
}

void FuelLogWidget::onFuelLogUpdated(const Frontier::FuelLogEntry &entry)
{
    Q_UNUSED(entry);
    // Reload the log when a new entry is added
    loadFuelLog();
}
