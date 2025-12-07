/**
 * @file addtransactiondialog.h
 * @brief Dialog for adding/editing transactions
 */

#ifndef ADDTRANSACTIONDIALOG_H
#define ADDTRANSACTIONDIALOG_H

#include <QDialog>
#include <QDateEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

#include "core/types.h"

class AddTransactionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTransactionDialog(QWidget *parent = nullptr);
    ~AddTransactionDialog();

    // Get the transaction data from the form
    Frontier::Transaction getTransaction() const;

    // Set transaction data (for editing)
    void setTransaction(const Frontier::Transaction &transaction);

    // Set available items for dropdown
    void setItems(const QVector<Frontier::Item> &items);

private slots:
    void onTypeChanged(int index);
    void onItemChanged(int index);
    void onQuantityChanged(int value);
    void onUnitPriceChanged(double value);
    void validateAndAccept();

private:
    void setupUi();
    void calculateTotal();
    void updateCategories();

    // Form fields
    QDateEdit *m_dateEdit;
    QComboBox *m_typeCombo;
    QComboBox *m_accountCombo;
    QComboBox *m_itemCombo;
    QLineEdit *m_customItemEdit;
    QComboBox *m_categoryCombo;
    QSpinBox *m_quantitySpin;
    QDoubleSpinBox *m_unitPriceSpin;
    QDoubleSpinBox *m_totalSpin;
    QLineEdit *m_notesEdit;

    // Buttons
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    // Store original transaction ID for editing
    std::optional<int> m_transactionId;

    // Items cache for lookup
    QVector<Frontier::Item> m_items;

    // Store selected item prices for game pricing logic
    double m_selectedBuyInternal = 0;
    double m_selectedBuyDisplay = 0;
    double m_selectedSellInternal = 0;
    double m_selectedSellDisplay = 0;
};

#endif // ADDTRANSACTIONDIALOG_H
