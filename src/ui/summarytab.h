/**
 * @file summarytab.h
 * @brief Income & Expense Summary - financial overview and reporting
 */

#ifndef SUMMARYTAB_H
#define SUMMARYTAB_H

#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>

#include "core/types.h"

namespace Frontier {
class Database;
}

class SummaryTab : public QWidget
{
    Q_OBJECT

public:
    explicit SummaryTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    void onPeriodChanged();

private:
    void setupUi();
    QWidget* createOverviewPanel();
    QWidget* createBreakdownPanel();
    void updateSummary();

    Frontier::Database *m_database;

    // Period selection
    QComboBox *m_periodCombo;
    QDateEdit *m_fromDateEdit;
    QDateEdit *m_toDateEdit;
    QPushButton *m_refreshBtn;

    // Overview labels
    QLabel *m_totalIncomeLabel;
    QLabel *m_totalExpensesLabel;
    QLabel *m_netProfitLabel;
    QLabel *m_profitMarginLabel;

    // Breakdown tables
    QTableWidget *m_incomeTable;
    QTableWidget *m_expenseTable;
};

#endif // SUMMARYTAB_H
