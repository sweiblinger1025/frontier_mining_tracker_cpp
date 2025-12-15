/**
 * @file ledgertab.h
 * @brief Ledger - chronological record of all financial transactions
 */

#ifndef LEDGERTAB_H
#define LEDGERTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGroupBox>

#include "core/types.h"

namespace Frontier {
class Database;
}

class LedgerTab : public QWidget
{
    Q_OBJECT

public:
    explicit LedgerTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

signals:
    void transactionChanged();  // Emitted when transactions are added/edited/deleted
    void balancesChanged();     // Emitted when balances might have changed

private slots:
    void onFilterChanged();
    void onAddTransaction();
    void onEditTransaction();
    void onDeleteTransaction();
    void onSelectionChanged();
    void onTransactionDoubleClicked(int row, int column);

private:
    void setupUi();
    QWidget* createFilterPanel();
    QWidget* createSummaryPanel();
    QWidget* createDetailsPanel();

    void loadTransactions();
    void updateSummary();
    void showTransactionDialog(bool isEdit);
    void populateDetails(const Frontier::Transaction &transaction);
    void clearDetails();

    QString formatAmount(double amount, Frontier::TransactionType type) const;

    Frontier::Database *m_database;

    // Filters
    QDateEdit *m_fromDateEdit;
    QDateEdit *m_toDateEdit;
    QComboBox *m_typeCombo;
    QComboBox *m_accountCombo;
    QComboBox *m_categoryCombo;
    QLineEdit *m_searchEdit;
    QPushButton *m_refreshBtn;

    // Summary
    QLabel *m_totalIncomeLabel;
    QLabel *m_personalIncomeLabel;
    QLabel *m_companyIncomeLabel;
    QLabel *m_totalExpensesLabel;
    QLabel *m_personalExpensesLabel;
    QLabel *m_companyExpensesLabel;
    QLabel *m_netLabel;
    QLabel *m_transactionCountLabel;

    // Table
    QTableWidget *m_ledgerTable;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;

    // Details panel
    QGroupBox *m_detailsGroup;
    QLabel *m_detailDateLabel;
    QLabel *m_detailTypeLabel;
    QLabel *m_detailAccountLabel;
    QLabel *m_detailItemLabel;
    QLabel *m_detailCategoryLabel;
    QLabel *m_detailQuantityLabel;
    QLabel *m_detailUnitPriceLabel;
    QLabel *m_detailTotalLabel;
    QLabel *m_detailNotesLabel;

    // Data
    QVector<Frontier::Transaction> m_transactions;
};

#endif // LEDGERTAB_H
