/**
 * @file financewidget.cpp
 * @brief Finance tab container implementation
 */

#include "financewidget.h"
#include "ledgertab.h"
#include "accountstab.h"
#include "summarytab.h"
#include "budgetstab.h"
#include "financesettingstab.h"
#include "capitalplannerwidget.h"
#include "core/database.h"

#include <QVBoxLayout>

FinanceWidget::FinanceWidget(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
}

void FinanceWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_subTabs = new QTabWidget();

    // Ledger - Core of Finance
    m_ledgerTab = new LedgerTab(m_database, this);
    m_subTabs->addTab(m_ledgerTab, tr("Ledger"));

    // Accounts - Company vs Personal
    m_accountsTab = new AccountsTab(m_database, this);
    m_subTabs->addTab(m_accountsTab, tr("Accounts"));

    // Income & Expense Summary
    m_summaryTab = new SummaryTab(m_database, this);
    m_subTabs->addTab(m_summaryTab, tr("Summary"));

    // Budgets & Forecasting
    m_budgetsTab = new BudgetsTab(m_database, this);
    m_subTabs->addTab(m_budgetsTab, tr("Budgets"));

    // Capital Planner
    m_capitalPlannerTab = new CapitalPlannerWidget(m_database, this);
    m_subTabs->addTab(m_capitalPlannerTab, tr("Capital Planner"));

    // Finance Settings
    m_settingsTab = new FinanceSettingsTab(m_database, this);
    m_subTabs->addTab(m_settingsTab, tr("Settings"));

    // Connect signals for cross-tab updates
    connect(m_ledgerTab, &LedgerTab::balancesChanged, m_accountsTab, &AccountsTab::refreshData);
    connect(m_ledgerTab, &LedgerTab::transactionChanged, m_summaryTab, &SummaryTab::refreshData);
    connect(m_ledgerTab, &LedgerTab::transactionChanged, m_budgetsTab, &BudgetsTab::refreshData);
    connect(m_ledgerTab, &LedgerTab::balancesChanged, m_capitalPlannerTab, &CapitalPlannerWidget::refreshData);
    connect(m_accountsTab, &AccountsTab::transferCompleted, m_ledgerTab, &LedgerTab::refreshData);

    mainLayout->addWidget(m_subTabs);
}

void FinanceWidget::refreshAll()
{
    m_ledgerTab->refreshData();
    m_accountsTab->refreshData();
    m_summaryTab->refreshData();
    m_budgetsTab->refreshData();
    m_capitalPlannerTab->refreshData();
    m_settingsTab->refreshData();
}
