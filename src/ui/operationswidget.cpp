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
#include "shiftlogtab.h"
#include "cycletimetab.h"

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

    // Inventory
    m_inventoryTab = new InventoryTab(m_manager->database(), this);
    m_subTabs->addTab(m_inventoryTab, "Inventory");

    // Equipment Operations
    m_subTabs->addTab(new EquipmentOperationsTab(m_manager, this), "Equipment");

    // Fuel & Fluids Log
    m_subTabs->addTab(new FuelLogWidget(m_manager, this), "Fuel Log");

    // Material Movement (Core tab)
    m_subTabs->addTab(new MaterialMovementTab(m_manager, this), "Material Movement");

    // Production (with subtabs: Calculator, Log, Cost Analysis)
    m_productionTab = new ProductionTab(m_manager->database(), m_inventoryTab, this);
    m_subTabs->addTab(m_productionTab, "Production");

    // Shift Log
    m_shiftLogTab = new ShiftLogTab(m_manager->database(), this);
    m_subTabs->addTab(m_shiftLogTab, "Shift Log");

    // Cycle Time Analyzer
    m_cycleTimeTab = new CycleTimeTab(m_manager->database(), this);
    m_subTabs->addTab(m_cycleTimeTab, "Cycle Time");

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

ShiftLogTab* OperationsWidget::shiftLogTab() const
{
    return m_shiftLogTab;
}

CycleTimeTab* OperationsWidget::cycleTimeTab() const
{
    return m_cycleTimeTab;
}
