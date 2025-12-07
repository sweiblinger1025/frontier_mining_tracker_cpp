/**
 * @file additemdialog.cpp
 * @brief Implementation of Add/Edit Item dialog
 */

#include "additemdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>

AddItemDialog::AddItemDialog(QWidget *parent)
    : QDialog(parent)
    , m_itemId(std::nullopt)
{
    setupUi();
}

AddItemDialog::~AddItemDialog()
{
}

void AddItemDialog::setupUi()
{
    setWindowTitle("Add Item");
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // === Item Details Group ===
    QGroupBox *detailsGroup = new QGroupBox("Item Details");
    QFormLayout *formLayout = new QFormLayout(detailsGroup);

    // Name
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Enter item name...");
    formLayout->addRow("Name:", m_nameEdit);

    // Category Main
    m_categoryMainCombo = new QComboBox();
    m_categoryMainCombo->setEditable(true);
    formLayout->addRow("Main Category:", m_categoryMainCombo);

    // Category Sub
    m_categorySubCombo = new QComboBox();
    m_categorySubCombo->setEditable(true);
    formLayout->addRow("Sub Category:", m_categorySubCombo);

    mainLayout->addWidget(detailsGroup);

    // === Pricing Group ===
    QGroupBox *pricingGroup = new QGroupBox("Pricing");
    QFormLayout *pricingLayout = new QFormLayout(pricingGroup);

    // Pricing Group dropdown
    m_pricingGroupCombo = new QComboBox();
    m_pricingGroupCombo->addItem("Base 70%", "Base70");
    m_pricingGroupCombo->addItem("Resource 72%", "Resource72");
    m_pricingGroupCombo->addItem("Special 50%", "Special50");
    m_pricingGroupCombo->addItem("Custom", "Custom");
    pricingLayout->addRow("Pricing Group:", m_pricingGroupCombo);

    // Buy Price
    m_buyPriceSpin = new QDoubleSpinBox();
    m_buyPriceSpin->setRange(0, 99999999);
    m_buyPriceSpin->setDecimals(2);
    m_buyPriceSpin->setPrefix("$");
    m_buyPriceSpin->setSingleStep(100);
    pricingLayout->addRow("Buy Price:", m_buyPriceSpin);

    // Sell Price
    m_sellPriceSpin = new QDoubleSpinBox();
    m_sellPriceSpin->setRange(0, 99999999);
    m_sellPriceSpin->setDecimals(2);
    m_sellPriceSpin->setPrefix("$");
    m_sellPriceSpin->setSingleStep(100);
    m_sellPriceSpin->setEnabled(false);  // Calculated by default
    pricingLayout->addRow("Sell Price:", m_sellPriceSpin);

    mainLayout->addWidget(pricingGroup);

    // === Flags Group ===
    QGroupBox *flagsGroup = new QGroupBox("Item Flags");
    QHBoxLayout *flagsLayout = new QHBoxLayout(flagsGroup);

    m_canBuyCheck = new QCheckBox("Can Buy");
    m_canBuyCheck->setChecked(true);
    flagsLayout->addWidget(m_canBuyCheck);

    m_canSellCheck = new QCheckBox("Can Sell");
    m_canSellCheck->setChecked(true);
    flagsLayout->addWidget(m_canSellCheck);

    m_craftableCheck = new QCheckBox("Craftable");
    flagsLayout->addWidget(m_craftableCheck);

    flagsLayout->addStretch();

    mainLayout->addWidget(flagsGroup);

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

    m_okButton = new QPushButton("Add Item");
    m_okButton->setDefault(true);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // === Connect Signals ===
    connect(m_pricingGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddItemDialog::onPricingGroupChanged);

    connect(m_buyPriceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AddItemDialog::onBuyPriceChanged);

    connect(m_okButton, &QPushButton::clicked,
            this, &AddItemDialog::validateAndAccept);

    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);
}

void AddItemDialog::onPricingGroupChanged(int index)
{
    QString group = m_pricingGroupCombo->currentData().toString();

    // Enable/disable sell price based on Custom selection
    m_sellPriceSpin->setEnabled(group == "Custom");

    // Recalculate if not custom
    if (group != "Custom") {
        calculateSellPrice();
    }
}

void AddItemDialog::onBuyPriceChanged(double value)
{
    Q_UNUSED(value);

    QString group = m_pricingGroupCombo->currentData().toString();
    if (group != "Custom") {
        calculateSellPrice();
    }
}

void AddItemDialog::calculateSellPrice()
{
    double buyPrice = m_buyPriceSpin->value();
    QString group = m_pricingGroupCombo->currentData().toString();

    double sellPrice = 0;
    if (group == "Base70") {
        sellPrice = buyPrice * 0.70;
    } else if (group == "Resource72") {
        sellPrice = buyPrice * 0.72;
    } else if (group == "Special50") {
        sellPrice = buyPrice * 0.50;
    } else {
        sellPrice = buyPrice * 0.70;  // Default
    }

    m_sellPriceSpin->setValue(sellPrice);
}

void AddItemDialog::validateAndAccept()
{
    // Validate required fields
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Item name is required.");
        m_nameEdit->setFocus();
        return;
    }

    if (m_categoryMainCombo->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Main category is required.");
        m_categoryMainCombo->setFocus();
        return;
    }

    accept();
}

Frontier::Item AddItemDialog::getItem() const
{
    Frontier::Item item;

    item.id = m_itemId;
    item.name = m_nameEdit->text().trimmed();
    item.categoryMain = m_categoryMainCombo->currentText().trimmed();
    item.categorySub = m_categorySubCombo->currentText().trimmed();
    item.buyPriceInternal = m_buyPriceSpin->value();
    item.sellPriceInternal = m_sellPriceSpin->value();
    item.buyPriceDisplay = static_cast<int>(std::round(item.buyPriceInternal));
    item.sellPriceDisplay = static_cast<int>(std::round(item.sellPriceInternal));
    item.isPurchasable = m_canBuyCheck->isChecked();
    item.isSellable = m_canSellCheck->isChecked();
    item.isCraftable = m_craftableCheck->isChecked();
    item.notes = m_notesEdit->text().trimmed();

    // Set pricing group
    QString groupStr = m_pricingGroupCombo->currentData().toString();
      if (groupStr == "Resource72") {
    //     item.pricingGroup = Frontier::PricingGroup::Resource72;
    // } else if (groupStr == "Special75") {
    //     item.pricingGroup = Frontier::PricingGroup::Special75;
    } else if (groupStr == "Custom") {
        item.pricingGroup = Frontier::PricingGroup::Custom;
    } else {
        item.pricingGroup = Frontier::PricingGroup::Base70;
    }

    return item;
}

void AddItemDialog::setItem(const Frontier::Item &item)
{
    m_itemId = item.id;

    m_nameEdit->setText(item.name);
    m_categoryMainCombo->setCurrentText(item.categoryMain);
    m_categorySubCombo->setCurrentText(item.categorySub);
    m_buyPriceSpin->setValue(item.buyPriceInternal);
    m_sellPriceSpin->setValue(item.sellPriceInternal);
    m_canBuyCheck->setChecked(item.isPurchasable);
    m_canSellCheck->setChecked(item.isSellable);
    m_craftableCheck->setChecked(item.isCraftable);
    m_notesEdit->setText(item.notes);

    // Set pricing group combo
    switch (item.pricingGroup) {
    // case Frontier::PricingGroup::Resource72:
    //     m_pricingGroupCombo->setCurrentIndex(1);
    //     break;
    // case Frontier::PricingGroup::Special75:
    //     m_pricingGroupCombo->setCurrentIndex(2);
        break;
    case Frontier::PricingGroup::Custom:
        m_pricingGroupCombo->setCurrentIndex(3);
        break;
    default:
        m_pricingGroupCombo->setCurrentIndex(0);
        break;
    }

    // Update window title for edit mode
    setWindowTitle("Edit Item");
    m_okButton->setText("Save Changes");
}

void AddItemDialog::setCategories(const QStringList &mainCategories, const QStringList &subCategories)
{
    m_categoryMainCombo->clear();
    m_categoryMainCombo->addItems(mainCategories);

    m_categorySubCombo->clear();
    m_categorySubCombo->addItems(subCategories);
}
