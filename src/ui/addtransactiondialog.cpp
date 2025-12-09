/**
 * @file addtransactiondialog.cpp
 * @brief Implementation of Add/Edit Transaction dialog
 */

#include "addtransactiondialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

AddTransactionDialog::AddTransactionDialog(QWidget *parent)
    : QDialog(parent)
    , m_transactionId(std::nullopt)
{
    setupUi();
}

AddTransactionDialog::~AddTransactionDialog()
{
}

void AddTransactionDialog::setupUi()
{
    setWindowTitle("Add Transaction");
    setMinimumWidth(450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // === Transaction Details Group ===
    QGroupBox *detailsGroup = new QGroupBox("Transaction Details");
    QFormLayout *formLayout = new QFormLayout(detailsGroup);

    // Date
    m_dateEdit = new QDateEdit();
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDate(QDate::currentDate());
    formLayout->addRow("Date:", m_dateEdit);

    // Type
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem("Sale", static_cast<int>(Frontier::TransactionType::Sale));
    m_typeCombo->addItem("Purchase", static_cast<int>(Frontier::TransactionType::Purchase));
    m_typeCombo->addItem("Transfer", static_cast<int>(Frontier::TransactionType::Transfer));
    m_typeCombo->addItem("Fuel", static_cast<int>(Frontier::TransactionType::Fuel));
    formLayout->addRow("Type:", m_typeCombo);

    // Account
    m_accountCombo = new QComboBox();
    m_accountCombo->addItem("Company", static_cast<int>(Frontier::AccountType::Company));
    m_accountCombo->addItem("Personal", static_cast<int>(Frontier::AccountType::Personal));
    formLayout->addRow("Account:", m_accountCombo);

    mainLayout->addWidget(detailsGroup);

    // === Item Details Group ===
    QGroupBox *itemGroup = new QGroupBox("Item Details");
    QFormLayout *itemLayout = new QFormLayout(itemGroup);

    // Item dropdown
    m_itemCombo = new QComboBox();
    m_itemCombo->addItem("-- Select Item --", -1);
    m_itemCombo->addItem("Custom Entry...", 0);
    itemLayout->addRow("Item:", m_itemCombo);

    // Custom item entry (hidden by default)
    m_customItemEdit = new QLineEdit();
    m_customItemEdit->setPlaceholderText("Enter custom item name...");
    m_customItemEdit->setVisible(false);
    itemLayout->addRow("Custom Item:", m_customItemEdit);

    // Category
    m_categoryCombo = new QComboBox();
    m_categoryCombo->setEditable(true);
    itemLayout->addRow("Category:", m_categoryCombo);

    mainLayout->addWidget(itemGroup);

    // === Amounts Group ===
    QGroupBox *amountsGroup = new QGroupBox("Amounts");
    QFormLayout *amountsLayout = new QFormLayout(amountsGroup);

    // Quantity
    m_quantitySpin = new QSpinBox();
    m_quantitySpin->setRange(1, 999999);
    m_quantitySpin->setValue(1);
    amountsLayout->addRow("Quantity:", m_quantitySpin);

    // Unit Price
    m_unitPriceSpin = new QDoubleSpinBox();
    m_unitPriceSpin->setRange(0, 99999999);
    m_unitPriceSpin->setDecimals(2);
    m_unitPriceSpin->setPrefix("$");
    m_unitPriceSpin->setSingleStep(100);
    amountsLayout->addRow("Unit Price:", m_unitPriceSpin);

    // Total (calculated)
    m_totalSpin = new QDoubleSpinBox();
    m_totalSpin->setRange(-99999999, 99999999);
    m_totalSpin->setDecimals(2);
    m_totalSpin->setPrefix("$");
    m_totalSpin->setReadOnly(true);
    m_totalSpin->setStyleSheet("background-color: #f0f0f0;");
    amountsLayout->addRow("Total:", m_totalSpin);

    mainLayout->addWidget(amountsGroup);

    // === Notes ===
    QGroupBox *notesGroup = new QGroupBox("Notes");
    QVBoxLayout *notesLayout = new QVBoxLayout(notesGroup);

    m_notesEdit = new QLineEdit();
    m_notesEdit->setPlaceholderText("Optional notes...");
    notesLayout->addWidget(m_notesEdit);

    mainLayout->addWidget(notesGroup);

    // === Buttons ===
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton("Add Transaction");
    m_okButton->setDefault(true);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // === Connect Signals ===
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddTransactionDialog::onTypeChanged);

    connect(m_itemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddTransactionDialog::onItemChanged);

    connect(m_quantitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AddTransactionDialog::onQuantityChanged);

    connect(m_unitPriceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AddTransactionDialog::onUnitPriceChanged);

    connect(m_okButton, &QPushButton::clicked,
            this, &AddTransactionDialog::validateAndAccept);

    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);
}

void AddTransactionDialog::onTypeChanged(int index)
{
    Q_UNUSED(index);

    // Update unit price based on new type
    int itemData = m_itemCombo->currentData().toInt();
    if (itemData > 0) {
        Frontier::TransactionType type = static_cast<Frontier::TransactionType>(
            m_typeCombo->currentData().toInt());

        // Use display price unless it's 0 (items < $1 round to 0)
        double priceToShow = 0;
        if (type == Frontier::TransactionType::Sale) {
            priceToShow = (m_selectedSellDisplay > 0) ? m_selectedSellDisplay : m_selectedSellInternal;
        } else {
            priceToShow = (m_selectedBuyDisplay > 0) ? m_selectedBuyDisplay : m_selectedBuyInternal;
        }
        m_unitPriceSpin->setValue(priceToShow);
    }

    calculateTotal();
}

void AddTransactionDialog::onItemChanged(int index)
{
    Q_UNUSED(index);

    int itemData = m_itemCombo->currentData().toInt();

    // Show/hide custom item entry
    m_customItemEdit->setVisible(itemData == 0);

    // If a real item is selected, auto-fill price and category
    if (itemData > 0) {
        for (const auto &item : m_items) {
            if (item.id.has_value() && item.id.value() == itemData) {
                // Store all prices for game pricing logic
                m_selectedBuyInternal = item.buyPriceInternal;
                m_selectedBuyDisplay = item.buyPriceDisplay;
                m_selectedSellInternal = item.sellPriceInternal;
                m_selectedSellDisplay = item.sellPriceDisplay;

                // Set category
                QString category = item.displayCategory();
                int catIndex = m_categoryCombo->findText(category);
                if (catIndex >= 0) {
                    m_categoryCombo->setCurrentIndex(catIndex);
                } else {
                    m_categoryCombo->setCurrentText(category);
                }

                // Set unit price based on transaction type
                // Use display price unless it's 0 (items < $1 round to 0)
                // In that case, use internal price for accurate display
                Frontier::TransactionType type = static_cast<Frontier::TransactionType>(
                    m_typeCombo->currentData().toInt());

                double priceToShow = 0;
                if (type == Frontier::TransactionType::Sale) {
                    priceToShow = (m_selectedSellDisplay > 0) ? m_selectedSellDisplay : m_selectedSellInternal;
                } else {
                    priceToShow = (m_selectedBuyDisplay > 0) ? m_selectedBuyDisplay : m_selectedBuyInternal;
                }
                m_unitPriceSpin->setValue(priceToShow);
                break;
            }
        }
    } else {
        // Reset stored prices for custom items
        m_selectedBuyInternal = 0;
        m_selectedBuyDisplay = 0;
        m_selectedSellInternal = 0;
        m_selectedSellDisplay = 0;
    }

    calculateTotal();
}

void AddTransactionDialog::onQuantityChanged(int value)
{
    Q_UNUSED(value);
    calculateTotal();
}

void AddTransactionDialog::onUnitPriceChanged(double value)
{
    Q_UNUSED(value);
    calculateTotal();
}

void AddTransactionDialog::calculateTotal()
{
    int quantity = m_quantitySpin->value();
    double total = 0;

    Frontier::TransactionType type = static_cast<Frontier::TransactionType>(
        m_typeCombo->currentData().toInt());

    int itemData = m_itemCombo->currentData().toInt();

    if (itemData > 0) {
        // Known item - use game pricing logic
        // Single item uses display (rounded) price
        // Multiple items use internal (fractional) price, then round total
        if (quantity == 1) {
            if (type == Frontier::TransactionType::Sale) {
                total = m_selectedSellDisplay;
            } else {
                total = m_selectedBuyDisplay;
            }
        } else {
            if (type == Frontier::TransactionType::Sale) {
                total = m_selectedSellInternal * quantity;
            } else {
                total = m_selectedBuyInternal * quantity;
            }
            // Game rounds the final total to whole number
            total = std::round(total);
        }
    } else {
        // Custom item - just multiply unit price by quantity
        total = m_unitPriceSpin->value() * quantity;
    }

    // Make total negative for purchases/fuel
    if (type == Frontier::TransactionType::Purchase ||
        type == Frontier::TransactionType::Fuel) {
        total = -total;
    }

    m_totalSpin->setValue(total);
}

void AddTransactionDialog::validateAndAccept()
{
    // Validate item selection
    int itemData = m_itemCombo->currentData().toInt();

    if (itemData == -1) {
        QMessageBox::warning(this, "Validation Error", "Please select an item.");
        m_itemCombo->setFocus();
        return;
    }

    if (itemData == 0 && m_customItemEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a custom item name.");
        m_customItemEdit->setFocus();
        return;
    }

    if (m_categoryCombo->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please select or enter a category.");
        m_categoryCombo->setFocus();
        return;
    }

    if (m_unitPriceSpin->value() <= 0) {
        QMessageBox::warning(this, "Validation Error", "Unit price must be greater than zero.");
        m_unitPriceSpin->setFocus();
        return;
    }

    accept();
}

Frontier::Transaction AddTransactionDialog::getTransaction() const
{
    Frontier::Transaction trans;

    trans.id = m_transactionId;
    trans.date = m_dateEdit->date();
    trans.type = static_cast<Frontier::TransactionType>(m_typeCombo->currentData().toInt());
    trans.account = static_cast<Frontier::AccountType>(m_accountCombo->currentData().toInt());

    // Item name
    int itemData = m_itemCombo->currentData().toInt();
    if (itemData == 0) {
        trans.item = m_customItemEdit->text().trimmed();
    } else {
        trans.item = m_itemCombo->currentText();
    }

    trans.category = m_categoryCombo->currentText().trimmed();
    trans.quantity = m_quantitySpin->value();
    trans.unitPrice = m_unitPriceSpin->value();
    trans.totalAmount = m_totalSpin->value();

    return trans;
}

void AddTransactionDialog::setTransaction(const Frontier::Transaction &transaction)
{
    m_transactionId = transaction.id;

    m_dateEdit->setDate(transaction.date);

    // Set type
    int typeIndex = m_typeCombo->findData(static_cast<int>(transaction.type));
    if (typeIndex >= 0) {
        m_typeCombo->setCurrentIndex(typeIndex);
    }

    // Set account
    int accountIndex = m_accountCombo->findData(static_cast<int>(transaction.account));
    if (accountIndex >= 0) {
        m_accountCombo->setCurrentIndex(accountIndex);
    }

    // Set item - try to find in dropdown, otherwise use custom
    int itemIndex = m_itemCombo->findText(transaction.item);
    if (itemIndex >= 0) {
        m_itemCombo->setCurrentIndex(itemIndex);
    } else {
        m_itemCombo->setCurrentIndex(1);  // Custom Entry
        m_customItemEdit->setText(transaction.item);
    }

    m_categoryCombo->setCurrentText(transaction.category);
    m_quantitySpin->setValue(transaction.quantity);
    m_unitPriceSpin->setValue(transaction.unitPrice);

    // Update window title for edit mode
    setWindowTitle("Edit Transaction");
    m_okButton->setText("Save Changes");
}

void AddTransactionDialog::setItems(const QVector<Frontier::Item> &items)
{
    m_items = items;

    // Clear and rebuild item combo (keep first two entries)
    while (m_itemCombo->count() > 2) {
        m_itemCombo->removeItem(2);
    }

    // Add items grouped by category
    QSet<QString> categories;
    for (const auto &item : items) {
        m_itemCombo->addItem(item.name, item.id.value_or(-1));
        categories.insert(item.displayCategory());
    }

    // Populate category dropdown
    m_categoryCombo->clear();
    QStringList catList = categories.values();
    catList.sort();
    m_categoryCombo->addItems(catList);
}
