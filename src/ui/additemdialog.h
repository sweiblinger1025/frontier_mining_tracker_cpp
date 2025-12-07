/**
 * @file additemdialog.h
 * @brief Dialog for adding/editing items
 */

#ifndef ADDITEMDIALOG_H
#define ADDITEMDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>

#include "core/types.h"

class AddItemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddItemDialog(QWidget *parent = nullptr);
    ~AddItemDialog();

    // Get the item data from the form
    Frontier::Item getItem() const;

    // Set item data (for editing)
    void setItem(const Frontier::Item &item);

    // Set available categories for dropdowns
    void setCategories(const QStringList &mainCategories, const QStringList &subCategories);

private slots:
    void onPricingGroupChanged(int index);
    void onBuyPriceChanged(double value);
    void validateAndAccept();

private:
    void setupUi();
    void calculateSellPrice();

    // Form fields
    QLineEdit *m_nameEdit;
    QComboBox *m_categoryMainCombo;
    QComboBox *m_categorySubCombo;
    QDoubleSpinBox *m_buyPriceSpin;
    QDoubleSpinBox *m_sellPriceSpin;
    QComboBox *m_pricingGroupCombo;
    QCheckBox *m_canBuyCheck;
    QCheckBox *m_canSellCheck;
    QCheckBox *m_craftableCheck;
    QLineEdit *m_notesEdit;

    // Buttons
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    // Store original item ID for editing
    std::optional<int> m_itemId;
};

#endif // ADDITEMDIALOG_H
