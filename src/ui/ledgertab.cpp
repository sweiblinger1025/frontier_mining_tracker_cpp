/**
 * @file ledgertab.cpp
 * @brief Ledger implementation - chronological record of all financial transactions
 */

#include "ledgertab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QLocale>
#include <cmath>
#include <algorithm>

namespace {
// Format currency with commas, no decimals: $100,000
QString formatCurrency(double amount)
{
    QLocale locale(QLocale::English);
    return "$" + locale.toString(static_cast<qint64>(qRound(amount)));
}

// Format currency with +/- prefix
QString formatCurrencySigned(double amount, bool positive)
{
    QString prefix = positive ? "+" : "-";
    QLocale locale(QLocale::English);
    return prefix + "$" + locale.toString(static_cast<qint64>(qRound(qAbs(amount))));
}
}
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSplitter>

// =============================================================================
// Constructor & Setup
// =============================================================================

LedgerTab::LedgerTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void LedgerTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Top: Filters
    mainLayout->addWidget(createFilterPanel());

    // Summary bar
    mainLayout->addWidget(createSummaryPanel());

    // Main area: Table + Details
    auto *splitter = new QSplitter(Qt::Horizontal);

    // Left: Ledger table
    auto *tableWidget = new QWidget();
    auto *tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_ledgerTable = new QTableWidget();
    m_ledgerTable->setColumnCount(8);
    m_ledgerTable->setHorizontalHeaderLabels({
        tr("Date"), tr("Type"), tr("Account"), tr("Item/Description"),
        tr("Category"), tr("Qty"), tr("Unit Price"), tr("Total")
    });
    m_ledgerTable->setAlternatingRowColors(true);
    m_ledgerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ledgerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ledgerTable->setSortingEnabled(true);
    m_ledgerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ledgerTable->verticalHeader()->setVisible(false);

    auto *header = m_ledgerTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Date
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Type
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Account
    header->setSectionResizeMode(3, QHeaderView::Stretch);           // Item
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);  // Category
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);  // Qty
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);  // Unit Price
    header->setSectionResizeMode(7, QHeaderView::ResizeToContents);  // Total

    connect(m_ledgerTable, &QTableWidget::itemSelectionChanged,
            this, &LedgerTab::onSelectionChanged);
    connect(m_ledgerTable, &QTableWidget::cellDoubleClicked,
            this, &LedgerTab::onTransactionDoubleClicked);

    tableLayout->addWidget(m_ledgerTable);

    // Buttons
    auto *btnLayout = new QHBoxLayout();

    m_addBtn = new QPushButton(tr("+ Add Transaction"));
    m_addBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold;");
    connect(m_addBtn, &QPushButton::clicked, this, &LedgerTab::onAddTransaction);
    btnLayout->addWidget(m_addBtn);

    btnLayout->addStretch();

    m_editBtn = new QPushButton(tr("Edit"));
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &LedgerTab::onEditTransaction);
    btnLayout->addWidget(m_editBtn);

    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &LedgerTab::onDeleteTransaction);
    btnLayout->addWidget(m_deleteBtn);

    tableLayout->addLayout(btnLayout);

    splitter->addWidget(tableWidget);

    // Right: Details panel
    splitter->addWidget(createDetailsPanel());
    splitter->setSizes({700, 300});

    mainLayout->addWidget(splitter, 1);
}

QWidget* LedgerTab::createFilterPanel()
{
    auto *group = new QGroupBox(tr("Filters"));
    auto *layout = new QHBoxLayout(group);

    // Date range
    layout->addWidget(new QLabel(tr("From:")));
    m_fromDateEdit = new QDateEdit(QDate::currentDate().addMonths(-1));
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("yyyy-MM-dd");
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &LedgerTab::onFilterChanged);
    layout->addWidget(m_fromDateEdit);

    layout->addWidget(new QLabel(tr("To:")));
    m_toDateEdit = new QDateEdit(QDate::currentDate());
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("yyyy-MM-dd");
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &LedgerTab::onFilterChanged);
    layout->addWidget(m_toDateEdit);

    // Type filter
    layout->addWidget(new QLabel(tr("Type:")));
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem(tr("All Types"), "");
    m_typeCombo->addItem(tr("Opening"), "Opening");
    m_typeCombo->addItem(tr("Sale"), "Sale");
    m_typeCombo->addItem(tr("Purchase"), "Purchase");
    m_typeCombo->addItem(tr("Transfer"), "Transfer");
    m_typeCombo->addItem(tr("Fuel"), "Fuel");
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LedgerTab::onFilterChanged);
    layout->addWidget(m_typeCombo);

    // Account filter
    layout->addWidget(new QLabel(tr("Account:")));
    m_accountCombo = new QComboBox();
    m_accountCombo->addItem(tr("All Accounts"), "");
    m_accountCombo->addItem(tr("Company"), "Company");
    m_accountCombo->addItem(tr("Personal"), "Personal");
    connect(m_accountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LedgerTab::onFilterChanged);
    layout->addWidget(m_accountCombo);

    // Category filter
    layout->addWidget(new QLabel(tr("Category:")));
    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItem(tr("All Categories"), "");
    // Categories will be populated from data
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LedgerTab::onFilterChanged);
    layout->addWidget(m_categoryCombo);

    layout->addStretch();

    // Search
    layout->addWidget(new QLabel(tr("Search:")));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Item name..."));
    m_searchEdit->setMaximumWidth(150);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &LedgerTab::onFilterChanged);
    layout->addWidget(m_searchEdit);

    // Refresh
    m_refreshBtn = new QPushButton(tr("Refresh"));
    connect(m_refreshBtn, &QPushButton::clicked, this, &LedgerTab::refreshData);
    layout->addWidget(m_refreshBtn);

    return group;
}

QWidget* LedgerTab::createSummaryPanel()
{
    auto *group = new QGroupBox(tr("Period Summary"));
    auto *layout = new QHBoxLayout(group);

    // Total Income
    auto *totalIncomeLayout = new QVBoxLayout();
    totalIncomeLayout->addWidget(new QLabel(tr("Total Income")));
    m_totalIncomeLabel = new QLabel("$0");
    m_totalIncomeLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2e7d32;");
    totalIncomeLayout->addWidget(m_totalIncomeLabel);
    layout->addLayout(totalIncomeLayout);

    // Personal Income
    auto *personalIncomeLayout = new QVBoxLayout();
    personalIncomeLayout->addWidget(new QLabel(tr("Personal Income")));
    m_personalIncomeLabel = new QLabel("$0");
    m_personalIncomeLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #7b1fa2;");
    personalIncomeLayout->addWidget(m_personalIncomeLabel);
    layout->addLayout(personalIncomeLayout);

    // Company Income
    auto *companyIncomeLayout = new QVBoxLayout();
    companyIncomeLayout->addWidget(new QLabel(tr("Company Income")));
    m_companyIncomeLabel = new QLabel("$0");
    m_companyIncomeLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #1976d2;");
    companyIncomeLayout->addWidget(m_companyIncomeLabel);
    layout->addLayout(companyIncomeLayout);

    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::VLine);
    layout->addWidget(sep1);

    // Total Expenses
    auto *totalExpensesLayout = new QVBoxLayout();
    totalExpensesLayout->addWidget(new QLabel(tr("Total Expenses")));
    m_totalExpensesLabel = new QLabel("$0");
    m_totalExpensesLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #c62828;");
    totalExpensesLayout->addWidget(m_totalExpensesLabel);
    layout->addLayout(totalExpensesLayout);

    // Personal Expenses
    auto *personalExpensesLayout = new QVBoxLayout();
    personalExpensesLayout->addWidget(new QLabel(tr("Personal Expenses")));
    m_personalExpensesLabel = new QLabel("$0");
    m_personalExpensesLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #c62828;");
    personalExpensesLayout->addWidget(m_personalExpensesLabel);
    layout->addLayout(personalExpensesLayout);

    // Company Expenses
    auto *companyExpensesLayout = new QVBoxLayout();
    companyExpensesLayout->addWidget(new QLabel(tr("Company Expenses")));
    m_companyExpensesLabel = new QLabel("$0");
    m_companyExpensesLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #c62828;");
    companyExpensesLayout->addWidget(m_companyExpensesLabel);
    layout->addLayout(companyExpensesLayout);

    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::VLine);
    layout->addWidget(sep2);

    // Net
    auto *netLayout = new QVBoxLayout();
    netLayout->addWidget(new QLabel(tr("Net")));
    m_netLabel = new QLabel("$0");
    m_netLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    netLayout->addWidget(m_netLabel);
    layout->addLayout(netLayout);

    auto *sep3 = new QFrame();
    sep3->setFrameShape(QFrame::VLine);
    layout->addWidget(sep3);

    // Transaction count
    auto *countLayout = new QVBoxLayout();
    countLayout->addWidget(new QLabel(tr("Transactions")));
    m_transactionCountLabel = new QLabel("0");
    m_transactionCountLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    countLayout->addWidget(m_transactionCountLabel);
    layout->addLayout(countLayout);

    layout->addStretch();

    return group;
}

QWidget* LedgerTab::createDetailsPanel()
{
    m_detailsGroup = new QGroupBox(tr("Transaction Details"));
    auto *layout = new QFormLayout(m_detailsGroup);

    m_detailDateLabel = new QLabel("-");
    layout->addRow(tr("Date:"), m_detailDateLabel);

    m_detailTypeLabel = new QLabel("-");
    layout->addRow(tr("Type:"), m_detailTypeLabel);

    m_detailAccountLabel = new QLabel("-");
    layout->addRow(tr("Account:"), m_detailAccountLabel);

    m_detailItemLabel = new QLabel("-");
    layout->addRow(tr("Item:"), m_detailItemLabel);

    m_detailCategoryLabel = new QLabel("-");
    layout->addRow(tr("Category:"), m_detailCategoryLabel);

    m_detailQuantityLabel = new QLabel("-");
    layout->addRow(tr("Quantity:"), m_detailQuantityLabel);

    m_detailUnitPriceLabel = new QLabel("-");
    layout->addRow(tr("Unit Price:"), m_detailUnitPriceLabel);

    m_detailTotalLabel = new QLabel("-");
    m_detailTotalLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addRow(tr("Total:"), m_detailTotalLabel);

    // Separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    layout->addRow(sep);

    m_detailNotesLabel = new QLabel("-");
    m_detailNotesLabel->setWordWrap(true);
    layout->addRow(tr("Notes:"), m_detailNotesLabel);

    return m_detailsGroup;
}

// =============================================================================
// Data Loading
// =============================================================================

void LedgerTab::refreshData()
{
    loadTransactions();
    updateSummary();

    // Populate category filter
    QSet<QString> categories;
    for (const auto &t : m_transactions) {
        if (!t.category.isEmpty()) {
            categories.insert(t.category);
        }
    }

    m_categoryCombo->blockSignals(true);
    QString current = m_categoryCombo->currentData().toString();
    m_categoryCombo->clear();
    m_categoryCombo->addItem(tr("All Categories"), "");
    for (const auto &cat : categories) {
        m_categoryCombo->addItem(cat, cat);
    }
    int idx = m_categoryCombo->findData(current);
    if (idx >= 0) m_categoryCombo->setCurrentIndex(idx);
    m_categoryCombo->blockSignals(false);
}

void LedgerTab::loadTransactions()
{
    QDate fromDate = m_fromDateEdit->date();
    QDate toDate = m_toDateEdit->date();

    m_transactions = m_database->getTransactionsByDateRange(fromDate, toDate);

    // Apply filters
    QString typeFilter = m_typeCombo->currentData().toString();
    QString accountFilter = m_accountCombo->currentData().toString();
    QString categoryFilter = m_categoryCombo->currentData().toString();
    QString searchFilter = m_searchEdit->text().toLower();

    QVector<Frontier::Transaction> filtered;
    for (const auto &t : m_transactions) {
        if (!typeFilter.isEmpty() && Frontier::transactionTypeToString(t.type) != typeFilter) {
            continue;
        }
        if (!accountFilter.isEmpty() && Frontier::accountTypeToString(t.account) != accountFilter) {
            continue;
        }
        if (!categoryFilter.isEmpty() && t.category != categoryFilter) {
            continue;
        }
        if (!searchFilter.isEmpty() && !t.item.toLower().contains(searchFilter)) {
            continue;
        }
        filtered.append(t);
    }

    m_transactions = filtered;

    // Sort: Opening transactions first, then by date descending
    std::sort(m_transactions.begin(), m_transactions.end(),
              [](const Frontier::Transaction &a, const Frontier::Transaction &b) {
                  // Opening transactions come first
                  bool aIsOpening = (a.type == Frontier::TransactionType::Opening);
                  bool bIsOpening = (b.type == Frontier::TransactionType::Opening);

                  if (aIsOpening && !bIsOpening) return true;
                  if (!aIsOpening && bIsOpening) return false;

                  // Then sort by date descending (newest first)
                  if (a.date != b.date) return a.date > b.date;

                  // Then by ID descending
                  return a.id.value_or(0) > b.id.value_or(0);
              });

    // Populate table
    m_ledgerTable->setSortingEnabled(false);
    m_ledgerTable->setRowCount(m_transactions.size());

    for (int row = 0; row < m_transactions.size(); ++row) {
        const auto &t = m_transactions[row];

        // Date
        auto *dateItem = new QTableWidgetItem(t.date.toString("yyyy-MM-dd"));
        dateItem->setData(Qt::UserRole, t.id.value_or(0));
        m_ledgerTable->setItem(row, 0, dateItem);

        // Type
        QString typeStr = Frontier::transactionTypeToString(t.type);
        auto *typeItem = new QTableWidgetItem(typeStr);
        QColor typeColor;
        switch (t.type) {
        case Frontier::TransactionType::Opening:
            typeColor = QColor("#1565c0"); // Blue
            break;
        case Frontier::TransactionType::Sale:
            typeColor = QColor("#2e7d32"); // Green
            break;
        case Frontier::TransactionType::Purchase:
        case Frontier::TransactionType::Fuel:
            typeColor = QColor("#c62828"); // Red
            break;
        case Frontier::TransactionType::Transfer:
            typeColor = QColor("#7b1fa2"); // Purple
            break;
        }
        typeItem->setForeground(typeColor);
        m_ledgerTable->setItem(row, 1, typeItem);

        // Account
        m_ledgerTable->setItem(row, 2, new QTableWidgetItem(Frontier::accountTypeToString(t.account)));

        // Item
        m_ledgerTable->setItem(row, 3, new QTableWidgetItem(t.item));

        // Category
        m_ledgerTable->setItem(row, 4, new QTableWidgetItem(t.category));

        // Quantity
        auto *qtyItem = new QTableWidgetItem(QString::number(t.quantity));
        qtyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_ledgerTable->setItem(row, 5, qtyItem);

        // Unit Price
        auto *priceItem = new QTableWidgetItem(formatCurrency(t.unitPrice));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_ledgerTable->setItem(row, 6, priceItem);

        // Total
        auto *totalItem = new QTableWidgetItem(formatAmount(t.totalAmount, t.type));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setData(Qt::UserRole, t.totalAmount);
        if (t.type == Frontier::TransactionType::Sale || t.type == Frontier::TransactionType::Opening) {
            totalItem->setForeground(QColor("#2e7d32"));
        } else if (t.type == Frontier::TransactionType::Purchase || t.type == Frontier::TransactionType::Fuel) {
            totalItem->setForeground(QColor("#c62828"));
        }
        m_ledgerTable->setItem(row, 7, totalItem);
    }

    m_ledgerTable->setSortingEnabled(true);
    clearDetails();
}

void LedgerTab::updateSummary()
{
    double totalIncome = 0;
    double personalIncome = 0;
    double companyIncome = 0;
    double totalExpenses = 0;
    double personalExpenses = 0;
    double companyExpenses = 0;

    for (const auto &t : m_transactions) {
        if (t.type == Frontier::TransactionType::Sale || t.type == Frontier::TransactionType::Opening) {
            totalIncome += t.totalAmount;
            if (t.account == Frontier::AccountType::Personal) {
                personalIncome += t.totalAmount;
            } else {
                companyIncome += t.totalAmount;
            }
        } else if (t.type == Frontier::TransactionType::Purchase || t.type == Frontier::TransactionType::Fuel) {
            totalExpenses += t.totalAmount;
            if (t.account == Frontier::AccountType::Personal) {
                personalExpenses += t.totalAmount;
            } else {
                companyExpenses += t.totalAmount;
            }
        }
    }

    double net = totalIncome - totalExpenses;

    m_totalIncomeLabel->setText(formatCurrency(totalIncome));
    m_personalIncomeLabel->setText(formatCurrency(personalIncome));
    m_companyIncomeLabel->setText(formatCurrency(companyIncome));
    m_totalExpensesLabel->setText(formatCurrency(totalExpenses));
    m_personalExpensesLabel->setText(formatCurrency(personalExpenses));
    m_companyExpensesLabel->setText(formatCurrency(companyExpenses));
    m_netLabel->setText(formatCurrency(net));

    if (net >= 0) {
        m_netLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2e7d32;");
    } else {
        m_netLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #c62828;");
    }

    m_transactionCountLabel->setText(QString::number(m_transactions.size()));
}

void LedgerTab::populateDetails(const Frontier::Transaction &t)
{
    m_detailDateLabel->setText(t.date.toString("yyyy-MM-dd"));
    m_detailTypeLabel->setText(Frontier::transactionTypeToString(t.type));
    m_detailAccountLabel->setText(Frontier::accountTypeToString(t.account));
    m_detailItemLabel->setText(t.item.isEmpty() ? "-" : t.item);
    m_detailCategoryLabel->setText(t.category.isEmpty() ? "-" : t.category);
    m_detailQuantityLabel->setText(QString::number(t.quantity));
    m_detailUnitPriceLabel->setText(formatCurrency(t.unitPrice));
    m_detailTotalLabel->setText(formatAmount(t.totalAmount, t.type));
    m_detailNotesLabel->setText(t.notes.isEmpty() ? "-" : t.notes);
}

void LedgerTab::clearDetails()
{
    m_detailDateLabel->setText("-");
    m_detailTypeLabel->setText("-");
    m_detailAccountLabel->setText("-");
    m_detailItemLabel->setText("-");
    m_detailCategoryLabel->setText("-");
    m_detailQuantityLabel->setText("-");
    m_detailUnitPriceLabel->setText("-");
    m_detailTotalLabel->setText("-");
    m_detailNotesLabel->setText("-");
}

QString LedgerTab::formatAmount(double amount, Frontier::TransactionType type) const
{
    bool isPositive = (type == Frontier::TransactionType::Sale || type == Frontier::TransactionType::Opening);
    return formatCurrencySigned(amount, isPositive);
}

// =============================================================================
// Slots
// =============================================================================

void LedgerTab::onFilterChanged()
{
    loadTransactions();
    updateSummary();
}

void LedgerTab::onSelectionChanged()
{
    auto selected = m_ledgerTable->selectedItems();
    bool hasSelection = !selected.isEmpty();

    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);

    if (hasSelection) {
        int row = selected.first()->row();
        if (row >= 0 && row < m_transactions.size()) {
            populateDetails(m_transactions[row]);
        }
    } else {
        clearDetails();
    }
}

void LedgerTab::onTransactionDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row >= 0 && row < m_transactions.size()) {
        onEditTransaction();
    }
}

void LedgerTab::showTransactionDialog(bool isEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? tr("Edit Transaction") : tr("Add Transaction"));
    dialog.setMinimumWidth(400);

    auto *layout = new QFormLayout(&dialog);

    // Date
    auto *dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    layout->addRow(tr("Date:"), dateEdit);

    // Type
    auto *typeCombo = new QComboBox();
    typeCombo->addItem(tr("Opening"), "Opening");
    typeCombo->addItem(tr("Sale"), "Sale");
    typeCombo->addItem(tr("Purchase"), "Purchase");
    typeCombo->addItem(tr("Transfer"), "Transfer");
    typeCombo->addItem(tr("Fuel"), "Fuel");
    layout->addRow(tr("Type:"), typeCombo);

    // Account
    auto *accountCombo = new QComboBox();
    accountCombo->addItem(tr("Company"), "Company");
    accountCombo->addItem(tr("Personal"), "Personal");
    layout->addRow(tr("Account:"), accountCombo);

    // Item
    auto *itemCombo = new QComboBox();
    itemCombo->setEditable(true);
    auto items = m_database->getAllItems();
    itemCombo->addItem("");
    for (const auto &item : items) {
        itemCombo->addItem(item.name);
    }
    layout->addRow(tr("Item:"), itemCombo);

    // Category
    auto *categoryEdit = new QLineEdit();
    layout->addRow(tr("Category:"), categoryEdit);

    // Quantity
    auto *quantitySpin = new QSpinBox();
    quantitySpin->setRange(1, 999999);
    quantitySpin->setValue(1);
    layout->addRow(tr("Quantity:"), quantitySpin);

    // Unit Price
    auto *priceSpin = new QDoubleSpinBox();
    priceSpin->setRange(0, 9999999);
    priceSpin->setDecimals(2);
    priceSpin->setPrefix("$");
    layout->addRow(tr("Unit Price:"), priceSpin);

    // Total (computed)
    auto *totalLabel = new QLabel("$0");
    totalLabel->setStyleSheet("font-weight: bold;");
    layout->addRow(tr("Total:"), totalLabel);

    // Auto-update total
    auto updateTotal = [=]() {
        double total = quantitySpin->value() * priceSpin->value();
        QLocale locale(QLocale::English);
        totalLabel->setText("$" + locale.toString(static_cast<qint64>(qRound(total))));
    };
    connect(quantitySpin, QOverload<int>::of(&QSpinBox::valueChanged), updateTotal);
    connect(priceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateTotal);

    // Auto-fill price from item
    auto updatePriceFromItem = [=]() {
        QString text = itemCombo->currentText();
        auto item = m_database->getItemByName(text);
        if (item.has_value()) {
            categoryEdit->setText(item->categoryMain);
            QString typeStr = typeCombo->currentData().toString();
            if (typeStr == "Purchase" || typeStr == "Fuel") {
                priceSpin->setValue(item->buyPriceInternal);
            } else {
                // Sale, Opening, Transfer - use sell price
                priceSpin->setValue(item->sellPriceInternal);
            }
        }
    };

    connect(itemCombo, &QComboBox::currentTextChanged, [=](const QString &) {
        updatePriceFromItem();
    });

    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int) {
        updatePriceFromItem();
    });

    // Notes
    auto *notesEdit = new QLineEdit();
    layout->addRow(tr("Notes:"), notesEdit);

    // Populate if editing
    Frontier::Transaction transaction;
    if (isEdit) {
        auto selected = m_ledgerTable->selectedItems();
        if (!selected.isEmpty()) {
            int row = selected.first()->row();
            transaction = m_transactions[row];

            dateEdit->setDate(transaction.date);
            int typeIdx = typeCombo->findData(Frontier::transactionTypeToString(transaction.type));
            if (typeIdx >= 0) typeCombo->setCurrentIndex(typeIdx);
            int accountIdx = accountCombo->findData(Frontier::accountTypeToString(transaction.account));
            if (accountIdx >= 0) accountCombo->setCurrentIndex(accountIdx);
            itemCombo->setCurrentText(transaction.item);
            categoryEdit->setText(transaction.category);
            quantitySpin->setValue(transaction.quantity);
            priceSpin->setValue(transaction.unitPrice);
            notesEdit->setText(transaction.notes);
            updateTotal();
        }
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        transaction.date = dateEdit->date();
        transaction.type = Frontier::stringToTransactionType(typeCombo->currentData().toString());
        transaction.account = Frontier::stringToAccountType(accountCombo->currentData().toString());
        transaction.item = itemCombo->currentText();
        transaction.category = categoryEdit->text();
        transaction.quantity = quantitySpin->value();
        transaction.unitPrice = priceSpin->value();
        transaction.totalAmount = transaction.quantity * transaction.unitPrice;
        transaction.notes = notesEdit->text();

        bool success = false;
        if (isEdit && transaction.id.has_value()) {
            success = m_database->updateTransaction(transaction);
        } else {
            // Check if this qualifies for auto-split (Sale of raw ore/oil)
            bool shouldAutoSplit = false;
            if (transaction.type == Frontier::TransactionType::Sale) {
                QString cat = transaction.category.toLower();
                QString itemName = transaction.item.toLower();
                // Split applies to raw ores and oil
                if (cat.contains("ore") || cat.contains("oil") ||
                    cat.contains("resource") || itemName.contains("ore") ||
                    itemName.contains("crude oil") || itemName.contains("raw")) {
                    shouldAutoSplit = true;
                }
            }

            if (shouldAutoSplit && !isEdit) {
                // Create two transactions: 90% Company, 10% Personal
                double companyPercent = 0.90;
                double personalPercent = 0.10;

                double companyAmount = std::round(transaction.totalAmount * companyPercent);
                double personalAmount = transaction.totalAmount - companyAmount;

                // Company transaction
                Frontier::Transaction companyTx = transaction;
                companyTx.account = Frontier::AccountType::Company;
                companyTx.totalAmount = companyAmount;
                companyTx.notes = transaction.notes.isEmpty()
                                      ? QString("90% split")
                                      : QString("%1 (90% split)").arg(transaction.notes);

                // Personal transaction
                Frontier::Transaction personalTx = transaction;
                personalTx.account = Frontier::AccountType::Personal;
                personalTx.totalAmount = personalAmount;
                personalTx.notes = transaction.notes.isEmpty()
                                       ? QString("10% split")
                                       : QString("%1 (10% split)").arg(transaction.notes);

                success = m_database->addTransaction(companyTx) &&
                          m_database->addTransaction(personalTx);

                if (success) {
                    QMessageBox::information(this, tr("Auto-Split Applied"),
                                             QString(tr("Sale auto-split:\n• Company: %1 (90%)\n• Personal: %2 (10%)"))
                                                 .arg(formatCurrency(companyAmount))
                                                 .arg(formatCurrency(personalAmount)));
                }
            } else {
                success = m_database->addTransaction(transaction);
            }
        }

        if (success) {
            refreshData();
            emit transactionChanged();
            emit balancesChanged();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to save transaction."));
        }
    }
}

void LedgerTab::onAddTransaction()
{
    showTransactionDialog(false);
}

void LedgerTab::onEditTransaction()
{
    showTransactionDialog(true);
}

void LedgerTab::onDeleteTransaction()
{
    auto selected = m_ledgerTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    int transactionId = m_ledgerTable->item(row, 0)->data(Qt::UserRole).toInt();

    auto result = QMessageBox::question(this, tr("Delete Transaction"),
                                        tr("Delete this transaction?\n\nThis cannot be undone."));

    if (result == QMessageBox::Yes) {
        if (m_database->deleteTransaction(transactionId)) {
            refreshData();
            emit transactionChanged();
            emit balancesChanged();
        }
    }
}
