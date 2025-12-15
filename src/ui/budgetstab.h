/**
 * @file budgetstab.h
 * @brief Budgets & Forecasting - planning and budget tracking
 */

#ifndef BUDGETSTAB_H
#define BUDGETSTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QProgressBar>

#include "core/types.h"

namespace Frontier {
class Database;
}

class BudgetsTab : public QWidget
{
    Q_OBJECT

public:
    explicit BudgetsTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    void onMonthChanged();
    void onAddBudget();
    void onEditBudget();
    void onDeleteBudget();
    void onSelectionChanged();

private:
    void setupUi();
    void loadBudgets();
    void showBudgetDialog(bool isEdit);

    Frontier::Database *m_database;

    // Month selection
    QSpinBox *m_yearSpin;
    QComboBox *m_monthCombo;
    QPushButton *m_prevMonthBtn;
    QPushButton *m_nextMonthBtn;

    // Summary
    QLabel *m_totalBudgetLabel;
    QLabel *m_totalActualLabel;
    QLabel *m_remainingLabel;
    QProgressBar *m_overallProgress;

    // Budget table
    QTableWidget *m_budgetTable;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;

    // Forecast section
    QLabel *m_forecastLabel;
};

#endif // BUDGETSTAB_H
