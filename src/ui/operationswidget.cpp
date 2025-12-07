/**
 * @file operationswidget.cpp
 * @brief Implementation of Operations tab
 */

#include "operationswidget.h"
#include "equipmentoperationstab.h"
#include "fuellogwidget.h"
#include "materialmovementtab.h"
#include "operationssettingstab.h"

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

    // Equipment Operations
    m_subTabs->addTab(new EquipmentOperationsTab(m_manager, this), "Equipment");

    // Fuel & Fluids Log
    m_subTabs->addTab(new FuelLogWidget(m_manager, this), "Fuel Log");

    // Material Movement (Core tab)
    m_subTabs->addTab(new MaterialMovementTab(m_manager, this), "Material Movement");

    // Stub tabs - Maintenance
    QWidget *maintenanceTab = new QWidget();
    QVBoxLayout *maintLayout = new QVBoxLayout(maintenanceTab);
    QLabel *maintLabel = new QLabel("Maintenance & Repairs - Coming Soon");
    maintLabel->setAlignment(Qt::AlignCenter);
    maintLabel->setStyleSheet("color: gray; font-size: 14px;");
    maintLayout->addWidget(maintLabel);
    m_subTabs->addTab(maintenanceTab, "Maintenance");

    // Stub tabs - Production
    QWidget *productionTab = new QWidget();
    QVBoxLayout *prodLayout = new QVBoxLayout(productionTab);
    QLabel *prodLabel = new QLabel("Production Tracking - Coming Soon");
    prodLabel->setAlignment(Qt::AlignCenter);
    prodLabel->setStyleSheet("color: gray; font-size: 14px;");
    prodLayout->addWidget(prodLabel);
    m_subTabs->addTab(productionTab, "Production");

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
