/**
 * @file operationswidget.cpp
 * @brief Implementation of Operations tab
 */

#include "operationswidget.h"
#include "equipmentoperationstab.h"
#include "fuellogwidget.h"
#include "materialmovementtab.h"
#include "operationssettingstab.h"
#include "inventorytab.h"
#include "productiontab.h"

#include <QVBoxLayout>
#include <QLabel>

OperationsWidget::OperationsWidget(Frontier::OperationsManager *manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    setupUi();
}

OperationsWidget::~OperationsWidget()
{
}

void OperationsWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_subTabs = new QTabWidget();

    // Inventory - ADD THIS (uses database from manager)
    m_inventoryTab = new InventoryTab(m_manager->database(), this);
    m_subTabs->addTab(m_inventoryTab, "Inventory");

    // Equipment Operations
    m_subTabs->addTab(new EquipmentOperationsTab(m_manager, this), "Equipment");

    // Fuel & Fluids Log
    m_subTabs->addTab(new FuelLogWidget(m_manager, this), "Fuel Log");

    // Material Movement (Core tab)
    m_subTabs->addTab(new MaterialMovementTab(m_manager, this), "Material Movement");

    // Stub tabs - Maintenance: TODO future update
    // QWidget *maintenanceTab = new QWidget();
    // QVBoxLayout *maintLayout = new QVBoxLayout(maintenanceTab);
    // QLabel *maintLabel = new QLabel("Maintenance & Repairs - Coming Soon");
    // maintLabel->setAlignment(Qt::AlignCenter);
    // maintLabel->setStyleSheet("color: gray; font-size: 14px;");
    // maintLayout->addWidget(maintLabel);
    // m_subTabs->addTab(maintenanceTab, "Maintenance");

    // Production (with subtabs: Calculator, Log, Cost Analysis)
    m_productionTab = new ProductionTab(m_manager->database(), m_inventoryTab, this);
    m_subTabs->addTab(m_productionTab, "Production");

    // Stub tabs - Shift Log
    QWidget *shiftTab = new QWidget();
    QVBoxLayout *shiftLayout = new QVBoxLayout(shiftTab);
    QLabel *shiftLabel = new QLabel("Shift Log - Coming Soon");
    shiftLabel->setAlignment(Qt::AlignCenter);
    shiftLabel->setStyleSheet("color: gray; font-size: 14px;");
    shiftLayout->addWidget(shiftLabel);
    m_subTabs->addTab(shiftTab, "Shift Log");

    // Stub tabs - Cycle Time
    QWidget *cycleTab = new QWidget();
    QVBoxLayout *cycleLayout = new QVBoxLayout(cycleTab);
    QLabel *cycleLabel = new QLabel("Cycle Time Analyzer - Coming Soon");
    cycleLabel->setAlignment(Qt::AlignCenter);
    cycleLabel->setStyleSheet("color: gray; font-size: 14px;");
    cycleLayout->addWidget(cycleLabel);
    m_subTabs->addTab(cycleTab, "Cycle Time");

    // Operations Settings
    m_subTabs->addTab(new OperationsSettingsTab(m_manager, this), "Settings");

    mainLayout->addWidget(m_subTabs);
}

InventoryTab* OperationsWidget::inventoryTab() const
{
    return m_inventoryTab;
}

ProductionTab* OperationsWidget::productionTab() const
{
    return m_productionTab;
}
