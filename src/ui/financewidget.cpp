/**
 * @file financewidget.cpp
 * @brief Implementation of Finance tab
 */

#include "financewidget.h"
#include "addtransactiondialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>

FinanceWidget::FinanceWidget(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_model(nullptr)
    , m_proxyModel(nullptr)
{
    setupUi();
    loadTransactions();
}

FinanceWidget::~FinanceWidget()
{
}

void FinanceWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Create subtabs
    m_subTabs = new QTabWidget();

    // Add tabs
    m_subTabs->addTab(createLedgerTab(), "Ledger");
    m_subTabs->addTab(new QLabel("Transaction Details - Coming Soon"), "Transaction Details");
    m_subTabs->addTab(new QLabel("Bank Accounts - Coming Soon"), "Bank Accounts");
    m_subTabs->addTab(new QLabel("Income & Expense Summary - Coming Soon"), "Income & Expense");
    m_subTabs->addTab(new QLabel("Budgets & Forecasting - Coming Soon"), "Budgets & Forecasting");
    m_subTabs->addTab(new QLabel("Finance Settings - Coming Soon"), "Settings");

    mainLayout->addWidget(m_subTabs);
}

QWidget* FinanceWidget::createLedgerTab()
{
    QWidget *ledgerTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(ledgerTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // === Filter Bar ===
    QHBoxLayout *filterLayout = new QHBoxLayout();

    // Date range
    QLabel *dateLabel = new QLabel("Date:");
    m_dateFrom = new QDateEdit();
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDate(QDate::currentDate().addMonths(-1));

    QLabel *toLabel = new QLabel("to");
    m_dateTo = new QDateEdit();
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDate(QDate::currentDate());

    // Type filter
    QLabel *typeLabel = new QLabel("Type:");
    m_typeFilter = new QComboBox();
    m_typeFilter->addItem("All Types", "");
    m_typeFilter->addItem("Sale", "Sale");
    m_typeFilter->addItem("Purchase", "Purchase");
    m_typeFilter->addItem("Transfer", "Transfer");
    m_typeFilter->addItem("Fuel", "Fuel");

    // Account filter
    QLabel *accountLabel = new QLabel("Account:");
    m_accountFilter = new QComboBox();
    m_accountFilter->addItem("All Accounts", "");
    m_accountFilter->addItem("Company", "Company");
    m_accountFilter->addItem("Personal", "Personal");

    // Search
    QLabel *searchLabel = new QLabel("Search:");
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search items...");
    m_searchBox->setClearButtonEnabled(true);

    filterLayout->addWidget(dateLabel);
    filterLayout->addWidget(m_dateFrom);
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(m_dateTo);
    filterLayout->addSpacing(15);
    filterLayout->addWidget(typeLabel);
    filterLayout->addWidget(m_typeFilter);
    filterLayout->addSpacing(15);
    filterLayout->addWidget(accountLabel);
    filterLayout->addWidget(m_accountFilter);
    filterLayout->addSpacing(15);
    filterLayout->addWidget(searchLabel);
    filterLayout->addWidget(m_searchBox, 1);

    mainLayout->addLayout(filterLayout);

    // === Table View ===
    m_tableView = new QTableView();
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);

    // Create model
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({
        "Date", "Type", "Account", "Item", "Category", "Qty", "Unit Price", "Total"
    });

    // Create proxy model for filtering/sorting
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);

    m_tableView->setModel(m_proxyModel);

    mainLayout->addWidget(m_tableView, 1);

    // === Summary Bar ===
    QHBoxLayout *summaryLayout = new QHBoxLayout();

    m_transactionCountLabel = new QLabel("Transactions: 0");
    m_incomeLabel = new QLabel("Income: $0");
    m_incomeLabel->setStyleSheet("color: green; font-weight: bold;");
    m_expenseLabel = new QLabel("Expenses: $0");
    m_expenseLabel->setStyleSheet("color: red; font-weight: bold;");

    summaryLayout->addWidget(m_transactionCountLabel);
    summaryLayout->addSpacing(30);
    summaryLayout->addWidget(m_incomeLabel);
    summaryLayout->addSpacing(30);
    summaryLayout->addWidget(m_expenseLabel);
    summaryLayout->addStretch();

    mainLayout->addLayout(summaryLayout);

    // === Bottom Button Bar ===
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_addButton = new QPushButton("Add Transaction");
    m_editButton = new QPushButton("Edit");
    m_deleteButton = new QPushButton("Delete");

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);

    mainLayout->addLayout(buttonLayout);

    // === Connect Signals ===
    connect(m_dateFrom, &QDateEdit::dateChanged, this, &FinanceWidget::onFilterChanged);
    connect(m_dateTo, &QDateEdit::dateChanged, this, &FinanceWidget::onFilterChanged);
    connect(m_typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FinanceWidget::onFilterChanged);
    connect(m_accountFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FinanceWidget::onFilterChanged);
    connect(m_searchBox, &QLineEdit::textChanged,
            this, &FinanceWidget::onSearchTextChanged);

    connect(m_addButton, &QPushButton::clicked,
            this, &FinanceWidget::onAddTransactionClicked);
    connect(m_editButton, &QPushButton::clicked,
            this, &FinanceWidget::onEditTransactionClicked);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &FinanceWidget::onDeleteTransactionClicked);
    connect(m_tableView, &QTableView::doubleClicked,
            this, &FinanceWidget::onTransactionDoubleClicked);

    return ledgerTab;
}

void FinanceWidget::loadTransactions()
{
    // Get transactions from database
    QDate fromDate = m_dateFrom->date();
    QDate toDate = m_dateTo->date();

    m_transactions = m_database->getTransactionsByDateRange(fromDate, toDate);

    // Clear existing data
    m_model->removeRows(0, m_model->rowCount());

    double totalIncome = 0;
    double totalExpense = 0;

    // Populate table
    for (const auto &trans : m_transactions) {
        QList<QStandardItem*> row;

        // Date
        QStandardItem *dateItem = new QStandardItem(trans.date.toString("yyyy-MM-dd"));
        row.append(dateItem);

        // Type
        QString typeStr;
        switch (trans.type) {
        case Frontier::TransactionType::Sale: typeStr = "Sale"; break;
        case Frontier::TransactionType::Purchase: typeStr = "Purchase"; break;
        case Frontier::TransactionType::Transfer: typeStr = "Transfer"; break;
        case Frontier::TransactionType::Fuel: typeStr = "Fuel"; break;
        }
        QStandardItem *typeItem = new QStandardItem(typeStr);
        if (trans.type == Frontier::TransactionType::Sale) {
            typeItem->setForeground(QBrush(Qt::darkGreen));
        } else if (trans.type == Frontier::TransactionType::Purchase ||
                   trans.type == Frontier::TransactionType::Fuel) {
            typeItem->setForeground(QBrush(Qt::red));
        }
        row.append(typeItem);

        // Account
        QString accountStr = (trans.account == Frontier::AccountType::Company) ? "Company" : "Personal";
        row.append(new QStandardItem(accountStr));

        // Item
        row.append(new QStandardItem(trans.item));

        // Category
        row.append(new QStandardItem(trans.category));

        // Quantity
        QStandardItem *qtyItem = new QStandardItem();
        qtyItem->setData(trans.quantity, Qt::DisplayRole);
        qtyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(qtyItem);

        // Unit Price
        QStandardItem *unitItem = new QStandardItem();
        unitItem->setData(QString("$%L1").arg(trans.unitPrice, 0, 'f', 2), Qt::DisplayRole);
        unitItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(unitItem);

        // Total
        QStandardItem *totalItem = new QStandardItem();
        totalItem->setData(QString("$%L1").arg(trans.totalAmount, 0, 'f', 2), Qt::DisplayRole);
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (trans.isIncome()) {
            totalItem->setForeground(QBrush(Qt::darkGreen));
            totalIncome += trans.totalAmount;
        } else {
            totalItem->setForeground(QBrush(Qt::red));
            totalExpense += std::abs(trans.totalAmount);
        }
        row.append(totalItem);

        // Store transaction ID
        row[0]->setData(trans.id.value_or(-1), Qt::UserRole);

        m_model->appendRow(row);
    }

    // Update summary
    m_transactionCountLabel->setText(QString("Transactions: %1").arg(m_transactions.size()));
    m_incomeLabel->setText(QString("Income: $%L1").arg(totalIncome, 0, 'f', 2));
    m_expenseLabel->setText(QString("Expenses: $%L1").arg(totalExpense, 0, 'f', 2));

    // Resize columns
    m_tableView->resizeColumnsToContents();
}

void FinanceWidget::refreshData()
{
    loadTransactions();
}

void FinanceWidget::updateSummary()
{
    // Recalculate from visible rows if needed
}

void FinanceWidget::onFilterChanged()
{
    loadTransactions();
}

void FinanceWidget::onSearchTextChanged(const QString &text)
{
    m_proxyModel->setFilterRegularExpression(
        QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
}

void FinanceWidget::onAddTransactionClicked()
{
    AddTransactionDialog dialog(this);

    // Get items for dropdown
    auto items = m_database->getAllItems();
    dialog.setItems(items);

    if (dialog.exec() == QDialog::Accepted) {
        Frontier::Transaction newTrans = dialog.getTransaction();

        if (m_database->addTransaction(newTrans)) {
            loadTransactions();
            QMessageBox::information(this, "Success", "Transaction added successfully!");
        } else {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to add transaction: %1").arg(m_database->lastError()));
        }
    }
}

void FinanceWidget::onEditTransactionClicked()
{
    QModelIndex currentIndex = m_tableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Edit Transaction", "Please select a transaction to edit.");
        return;
    }

    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    int transId = m_model->item(sourceIndex.row(), 0)->data(Qt::UserRole).toInt();

    // Find the transaction
    auto transOpt = m_database->getTransaction(transId);
    if (!transOpt.has_value()) {
        QMessageBox::warning(this, "Error", "Could not load transaction.");
        return;
    }

    AddTransactionDialog dialog(this);

    // Get items for dropdown
    auto items = m_database->getAllItems();
    dialog.setItems(items);
    dialog.setTransaction(transOpt.value());

    if (dialog.exec() == QDialog::Accepted) {
        Frontier::Transaction updatedTrans = dialog.getTransaction();

        if (m_database->updateTransaction(updatedTrans)) {
            loadTransactions();
            QMessageBox::information(this, "Success", "Transaction updated successfully!");
        } else {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to update transaction: %1").arg(m_database->lastError()));
        }
    }
}

void FinanceWidget::onDeleteTransactionClicked()
{
    QModelIndex currentIndex = m_tableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Delete Transaction", "Please select a transaction to delete.");
        return;
    }

    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    int transId = m_model->item(sourceIndex.row(), 0)->data(Qt::UserRole).toInt();
    QString itemName = m_model->item(sourceIndex.row(), 3)->text();

    int result = QMessageBox::question(this, "Delete Transaction",
                                       QString("Are you sure you want to delete this transaction for '%1'?").arg(itemName),
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        if (m_database->deleteTransaction(transId)) {
            loadTransactions();
        } else {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to delete transaction: %1").arg(m_database->lastError()));
        }
    }
}

void FinanceWidget::onTransactionDoubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    onEditTransactionClicked();
}
