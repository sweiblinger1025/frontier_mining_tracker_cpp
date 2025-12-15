/**
 * @file capitalplannerwidget.cpp
 * @brief Capital Planner implementation
 */

#include "capitalplannerwidget.h"
#include "budgetoverviewtab.h"
#include "equipmentplannertab.h"
#include "facilityplannertab.h"
#include "core/database.h"

#include <QVBoxLayout>

CapitalPlannerWidget::CapitalPlannerWidget(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
}

void CapitalPlannerWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget();

    m_overviewTab = new BudgetOverviewTab(m_database, this);
    m_equipmentTab = new EquipmentPlannerTab(m_database, this);
    m_facilityTab = new FacilityPlannerTab(m_database, this);

    m_tabs->addTab(m_overviewTab, tr("Budget Overview"));
    m_tabs->addTab(m_equipmentTab, tr("Equipment Planner"));
    m_tabs->addTab(m_facilityTab, tr("Facility Planner"));

    mainLayout->addWidget(m_tabs);

    // Connect planners to update overview when items change
    connect(m_equipmentTab, &EquipmentPlannerTab::planChanged,
            m_overviewTab, &BudgetOverviewTab::refreshData);
    connect(m_facilityTab, &FacilityPlannerTab::planChanged,
            m_overviewTab, &BudgetOverviewTab::refreshData);
}

void CapitalPlannerWidget::refreshData()
{
    m_overviewTab->refreshData();
    m_equipmentTab->refreshData();
    m_facilityTab->refreshData();
}
