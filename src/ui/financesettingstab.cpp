/**
 * @file financesettingstab.cpp
 * @brief Finance Settings implementation
 */

#include "financesettingstab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QLocale>

namespace {
QString formatCurrency(double amount)
{
    QLocale locale(QLocale::English);
    return "$" + locale.toString(static_cast<qint64>(qRound(amount)));
}
}

// =============================================================================
// Constructor & Setup
// =============================================================================

FinanceSettingsTab::FinanceSettingsTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    loadSettings();
}

void FinanceSettingsTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Auto-split settings
    auto *splitGroup = new QGroupBox(tr("Auto-Split Rules"));
    auto *splitLayout = new QVBoxLayout(splitGroup);

    auto *splitDesc = new QLabel(tr("When recording income, automatically split between Company and Personal accounts:"));
    splitDesc->setWordWrap(true);
    splitLayout->addWidget(splitDesc);

    auto *ratioLayout = new QHBoxLayout();

    ratioLayout->addWidget(new QLabel(tr("Company:")));
    m_companySplitSpin = new QSpinBox();
    m_companySplitSpin->setRange(0, 100);
    m_companySplitSpin->setValue(90);
    m_companySplitSpin->setSuffix("%");
    connect(m_companySplitSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &FinanceSettingsTab::onSplitRatioChanged);
    ratioLayout->addWidget(m_companySplitSpin);

    ratioLayout->addSpacing(20);

    ratioLayout->addWidget(new QLabel(tr("Personal:")));
    m_personalSplitSpin = new QSpinBox();
    m_personalSplitSpin->setRange(0, 100);
    m_personalSplitSpin->setValue(10);
    m_personalSplitSpin->setSuffix("%");
    m_personalSplitSpin->setEnabled(false);  // Calculated automatically
    ratioLayout->addWidget(m_personalSplitSpin);

    ratioLayout->addStretch();

    splitLayout->addLayout(ratioLayout);

    m_splitExampleLabel = new QLabel();
    m_splitExampleLabel->setStyleSheet("color: #666; font-style: italic; padding: 10px; background-color: #f5f5f5; border-radius: 5px;");
    splitLayout->addWidget(m_splitExampleLabel);
    onSplitRatioChanged();  // Update example

    mainLayout->addWidget(splitGroup);

    // Display options
    auto *displayGroup = new QGroupBox(tr("Display Options"));
    auto *displayLayout = new QVBoxLayout(displayGroup);

    m_trackPersonalCheck = new QCheckBox(tr("Track Personal account separately"));
    m_trackPersonalCheck->setChecked(true);
    displayLayout->addWidget(m_trackPersonalCheck);

    m_showCentsCheck = new QCheckBox(tr("Show cents in amounts (e.g., $1,234.56 vs $1,235)"));
    m_showCentsCheck->setChecked(true);
    displayLayout->addWidget(m_showCentsCheck);

    mainLayout->addWidget(displayGroup);

    // Category management
    auto *categoryGroup = new QGroupBox(tr("Transaction Categories"));
    auto *categoryLayout = new QHBoxLayout(categoryGroup);

    m_categoryList = new QListWidget();
    m_categoryList->setAlternatingRowColors(true);
    categoryLayout->addWidget(m_categoryList);

    auto *catBtnLayout = new QVBoxLayout();

    m_addCategoryBtn = new QPushButton(tr("Add Category"));
    connect(m_addCategoryBtn, &QPushButton::clicked, this, &FinanceSettingsTab::onAddCategory);
    catBtnLayout->addWidget(m_addCategoryBtn);

    m_deleteCategoryBtn = new QPushButton(tr("Delete"));
    connect(m_deleteCategoryBtn, &QPushButton::clicked, this, &FinanceSettingsTab::onDeleteCategory);
    catBtnLayout->addWidget(m_deleteCategoryBtn);

    catBtnLayout->addStretch();

    categoryLayout->addLayout(catBtnLayout);

    mainLayout->addWidget(categoryGroup);

    // Info section
    auto *infoGroup = new QGroupBox(tr("About Finance Tracking"));
    auto *infoLayout = new QVBoxLayout(infoGroup);

    auto *infoLabel = new QLabel(tr(
        "The Finance tab uses pure cash-based accounting:\n"
        "• Income from sales goes IN (+)\n"
        "• Expenses from purchases go OUT (-)\n"
        "• Transfers move money between accounts\n"
        "• No loans or credit - just cash in/out tracking\n\n"
        "The 'Opening' transaction type is used to set initial balances."
    ));
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("padding: 10px; background-color: #e8f5e9; border-radius: 5px;");
    infoLayout->addWidget(infoLabel);

    mainLayout->addWidget(infoGroup);

    mainLayout->addStretch();
}

void FinanceSettingsTab::refreshData()
{
    loadSettings();
}

void FinanceSettingsTab::loadSettings()
{
    // Load categories from existing transactions
    m_categoryList->clear();
    auto allTransactions = m_database->getAllTransactions();

    QSet<QString> categories;
    for (const auto &t : allTransactions) {
        if (!t.category.isEmpty()) {
            categories.insert(t.category);
        }
    }

    // Add default categories if none exist
    if (categories.isEmpty()) {
        categories.insert("Materials");
        categories.insert("Vehicles");
        categories.insert("Fuel");
        categories.insert("Equipment");
        categories.insert("Labor");
        categories.insert("Maintenance");
        categories.insert("Transfer");
    }

    for (const auto &cat : categories) {
        m_categoryList->addItem(cat);
    }

    m_categoryList->sortItems();
}

// =============================================================================
// Slots
// =============================================================================

void FinanceSettingsTab::onSplitRatioChanged()
{
    int company = m_companySplitSpin->value();
    int personal = 100 - company;
    m_personalSplitSpin->setValue(personal);

    double exampleAmount = 1000.0;
    double companyAmount = exampleAmount * company / 100.0;
    double personalAmount = exampleAmount * personal / 100.0;

    m_splitExampleLabel->setText(QString(tr(
        "Example: A $1,000 sale would split as:\n"
        "  • Company Account: %1 (%2%)\n"
        "  • Personal Wallet: %3 (%4%)"
    )).arg(formatCurrency(companyAmount)).arg(company).arg(formatCurrency(personalAmount)).arg(personal));
}

void FinanceSettingsTab::onAddCategory()
{
    bool ok;
    QString category = QInputDialog::getText(this, tr("Add Category"),
        tr("Category name:"), QLineEdit::Normal, "", &ok);

    if (ok && !category.trimmed().isEmpty()) {
        // Check if already exists
        for (int i = 0; i < m_categoryList->count(); ++i) {
            if (m_categoryList->item(i)->text().toLower() == category.trimmed().toLower()) {
                QMessageBox::information(this, tr("Category Exists"),
                    tr("This category already exists."));
                return;
            }
        }

        m_categoryList->addItem(category.trimmed());
        m_categoryList->sortItems();
    }
}

void FinanceSettingsTab::onDeleteCategory()
{
    auto selected = m_categoryList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select a category to delete."));
        return;
    }

    auto result = QMessageBox::question(this, tr("Delete Category"),
        tr("Delete category '%1'?\n\nNote: This only removes it from the list. "
           "Existing transactions with this category are not affected.")
            .arg(selected.first()->text()));

    if (result == QMessageBox::Yes) {
        delete selected.first();
    }
}
