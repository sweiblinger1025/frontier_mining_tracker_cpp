/**
 * @file budgetoverviewtab.h
 * @brief Budget Overview - Shows affordability dashboard for capital planning
 */

#ifndef BUDGETOVERVIEWTAB_H
#define BUDGETOVERVIEWTAB_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>

namespace Frontier {
class Database;
}

class BudgetOverviewTab : public QWidget
{
    Q_OBJECT

public:
    explicit BudgetOverviewTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private:
    void setupUi();
    void updateAffordability();

    Frontier::Database *m_database;

    // Available Funds
    QLabel *m_companyBalanceLabel;
    QLabel *m_personalBalanceLabel;
    QLabel *m_totalAvailableLabel;

    // Planned Costs
    QLabel *m_equipmentTotalLabel;
    QLabel *m_facilityTotalLabel;
    QLabel *m_grandTotalLabel;

    // Affordability
    QLabel *m_remainingLabel;
    QProgressBar *m_affordabilityBar;
    QLabel *m_statusLabel;

    // Power Summary
    QLabel *m_powerRequiredLabel;
    QLabel *m_powerGeneratedLabel;
    QLabel *m_powerBalanceLabel;
    QProgressBar *m_powerBar;
    QLabel *m_powerStatusLabel;
};

#endif // BUDGETOVERVIEWTAB_H
