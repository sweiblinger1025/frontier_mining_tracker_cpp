/**
 * @file productiontab.cpp
 * @brief Production tab container implementation
 */

#include "productiontab.h"
#include "costanalysistab.h"
#include "productioncalculatortab.h"
#include "productionlogtab.h"

#include <QVBoxLayout>
#include <QLabel>

ProductionTab::ProductionTab(Frontier::Database *database,
                             InventoryTab *inventoryTab,
                             QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_inventoryTab(inventoryTab)
{
    setupUi();
}

void ProductionTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_subTabs = new QTabWidget();

    // Production Calculator
    m_calculatorTab = new ProductionCalculatorTab(m_database, m_inventoryTab, this);
    m_subTabs->addTab(m_calculatorTab, tr("Calculator"));

    // Production Log
    m_logTab = new ProductionLogTab(m_database, m_inventoryTab, this);
    m_subTabs->addTab(m_logTab, tr("Log"));

    // Cost Analysis
    m_costAnalysisTab = new CostAnalysisTab(m_database, this);
    m_subTabs->addTab(m_costAnalysisTab, tr("Cost Analysis"));

    mainLayout->addWidget(m_subTabs);
}

void ProductionTab::refreshData()
{
    if (m_calculatorTab) {
        m_calculatorTab->refreshData();
    }
    if (m_logTab) {
        m_logTab->refreshData();
    }
    if (m_costAnalysisTab) {
        m_costAnalysisTab->refreshData();
    }
}
