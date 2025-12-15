/**
 * @file financewidget.h
 * @brief Finance tab container with all finance subtabs
 */

#ifndef FINANCEWIDGET_H
#define FINANCEWIDGET_H

#include <QWidget>
#include <QTabWidget>

namespace Frontier {
class Database;
}

class LedgerTab;
class AccountsTab;
class SummaryTab;
class BudgetsTab;
class FinanceSettingsTab;
class CapitalPlannerWidget;

class FinanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FinanceWidget(Frontier::Database *database, QWidget *parent = nullptr);

    LedgerTab* ledgerTab() const { return m_ledgerTab; }
    AccountsTab* accountsTab() const { return m_accountsTab; }

public slots:
    void refreshAll();

private:
    void setupUi();

    Frontier::Database *m_database;
    QTabWidget *m_subTabs;

    LedgerTab *m_ledgerTab;
    AccountsTab *m_accountsTab;
    SummaryTab *m_summaryTab;
    BudgetsTab *m_budgetsTab;
    FinanceSettingsTab *m_settingsTab;
    CapitalPlannerWidget *m_capitalPlannerTab;
};

#endif // FINANCEWIDGET_H
