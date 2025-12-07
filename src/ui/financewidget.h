/**
 * @file financewidget.h
 * @brief Finance tab - Transaction tracking and financial management
 */

#ifndef FINANCEWIDGET_H
#define FINANCEWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDateEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "core/database.h"

class FinanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FinanceWidget(Frontier::Database *database, QWidget *parent = nullptr);
    ~FinanceWidget();

    void refreshData();

private slots:
    // Ledger slots
    void onFilterChanged();
    void onSearchTextChanged(const QString &text);
    void onAddTransactionClicked();
    void onEditTransactionClicked();
    void onDeleteTransactionClicked();
    void onTransactionDoubleClicked(const QModelIndex &index);

private:
    void setupUi();

    // Subtab creation
    QWidget* createLedgerTab();
    void loadTransactions();
    void updateSummary();

    // Database reference
    Frontier::Database *m_database;

    // Subtabs
    QTabWidget *m_subTabs;

    // Ledger UI Components
    QDateEdit *m_dateFrom;
    QDateEdit *m_dateTo;
    QComboBox *m_categoryFilter;
    QComboBox *m_accountFilter;
    QComboBox *m_typeFilter;
    QLineEdit *m_searchBox;
    QTableView *m_tableView;

    // Summary labels
    QLabel *m_transactionCountLabel;
    QLabel *m_incomeLabel;
    QLabel *m_expenseLabel;

    // Buttons
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;

    // Data Model
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_proxyModel;

    // Data cache
    QVector<Frontier::Transaction> m_transactions;
};

#endif // FINANCEWIDGET_H
