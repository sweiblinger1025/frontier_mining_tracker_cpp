/**
 * @file accountstab.cpp
 * @brief Accounts implementation - Company vs Personal balance tracking
 */

#include "accountstab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QHeaderView>
#include <QMessageBox>
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

AccountsTab::AccountsTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void AccountsTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Balance cards
    auto *balanceLayout = new QHBoxLayout();
    balanceLayout->setSpacing(20);

    balanceLayout->addWidget(createBalanceCard(tr("Company Account"), &m_companyBalanceLabel, &m_companyPercentLabel, "#1976d2"));
    balanceLayout->addWidget(createBalanceCard(tr("Personal Wallet"), &m_personalBalanceLabel, &m_personalPercentLabel, "#7b1fa2"));

    // Total card
    auto *totalGroup = new QGroupBox(tr("Total Balance"));
    totalGroup->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto *totalLayout = new QVBoxLayout(totalGroup);
    m_totalBalanceLabel = new QLabel("$0");
    m_totalBalanceLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #2e7d32;");
    m_totalBalanceLabel->setAlignment(Qt::AlignCenter);
    totalLayout->addWidget(m_totalBalanceLabel);
    balanceLayout->addWidget(totalGroup);

    mainLayout->addLayout(balanceLayout);

    // Split ratio visualization
    auto *splitGroup = new QGroupBox(tr("Account Distribution"));
    auto *splitLayout = new QVBoxLayout(splitGroup);

    m_splitBar = new QProgressBar();
    m_splitBar->setRange(0, 100);
    m_splitBar->setTextVisible(false);
    m_splitBar->setMinimumHeight(30);
    m_splitBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #ccc;
            border-radius: 5px;
            background-color: #7b1fa2;
        }
        QProgressBar::chunk {
            background-color: #1976d2;
            border-radius: 4px;
        }
    )");
    splitLayout->addWidget(m_splitBar);

    m_splitLabel = new QLabel(tr("Company: 0% | Personal: 0%"));
    m_splitLabel->setAlignment(Qt::AlignCenter);
    splitLayout->addWidget(m_splitLabel);

    mainLayout->addWidget(splitGroup);

    // Transfer section
    auto *transferGroup = new QGroupBox(tr("Transfer Between Accounts"));
    auto *transferLayout = new QHBoxLayout(transferGroup);

    transferLayout->addWidget(new QLabel(tr("Amount:")));
    m_transferAmountSpin = new QDoubleSpinBox();
    m_transferAmountSpin->setRange(0.01, 9999999);
    m_transferAmountSpin->setDecimals(2);
    m_transferAmountSpin->setPrefix("$");
    m_transferAmountSpin->setValue(100);
    transferLayout->addWidget(m_transferAmountSpin);

    transferLayout->addWidget(new QLabel(tr("From:")));
    m_fromAccountCombo = new QComboBox();
    m_fromAccountCombo->addItem(tr("Company"), "Company");
    m_fromAccountCombo->addItem(tr("Personal"), "Personal");
    transferLayout->addWidget(m_fromAccountCombo);

    transferLayout->addWidget(new QLabel(tr("→")));

    transferLayout->addWidget(new QLabel(tr("To:")));
    m_toAccountCombo = new QComboBox();
    m_toAccountCombo->addItem(tr("Personal"), "Personal");
    m_toAccountCombo->addItem(tr("Company"), "Company");
    transferLayout->addWidget(m_toAccountCombo);

    transferLayout->addStretch();

    m_transferBtn = new QPushButton(tr("Transfer"));
    m_transferBtn->setStyleSheet("background-color: #7b1fa2; color: white; font-weight: bold; padding: 8px 20px;");
    connect(m_transferBtn, &QPushButton::clicked, this, &AccountsTab::onTransfer);
    transferLayout->addWidget(m_transferBtn);

    mainLayout->addWidget(transferGroup);

    // Recent transfers
    auto *recentGroup = new QGroupBox(tr("Recent Transfers"));
    auto *recentLayout = new QVBoxLayout(recentGroup);

    m_recentTable = new QTableWidget();
    m_recentTable->setColumnCount(5);
    m_recentTable->setHorizontalHeaderLabels({
        tr("Date"), tr("From"), tr("To"), tr("Amount"), tr("Notes")
    });
    m_recentTable->setAlternatingRowColors(true);
    m_recentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentTable->verticalHeader()->setVisible(false);
    m_recentTable->setMaximumHeight(200);

    auto *header = m_recentTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::Stretch);

    recentLayout->addWidget(m_recentTable);

    mainLayout->addWidget(recentGroup);

    mainLayout->addStretch();
}

QWidget* AccountsTab::createBalanceCard(const QString &title, QLabel **valueLabel, QLabel **percentLabel, const QString &color)
{
    auto *group = new QGroupBox(title);
    group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto *layout = new QVBoxLayout(group);

    *valueLabel = new QLabel("$0");
    (*valueLabel)->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;").arg(color));
    (*valueLabel)->setAlignment(Qt::AlignCenter);
    layout->addWidget(*valueLabel);

    *percentLabel = new QLabel("0% of total");
    (*percentLabel)->setStyleSheet("color: #666;");
    (*percentLabel)->setAlignment(Qt::AlignCenter);
    layout->addWidget(*percentLabel);

    return group;
}

// =============================================================================
// Data Loading
// =============================================================================

void AccountsTab::refreshData()
{
    updateBalances();

    // Load recent transfers
    auto allTransactions = m_database->getAllTransactions();
    QVector<Frontier::Transaction> transfers;

    for (const auto &t : allTransactions) {
        if (t.type == Frontier::TransactionType::Transfer) {
            transfers.append(t);
            if (transfers.size() >= 10) break;  // Last 10 transfers
        }
    }

    m_recentTable->setRowCount(transfers.size());
    for (int row = 0; row < transfers.size(); ++row) {
        const auto &t = transfers[row];

        m_recentTable->setItem(row, 0, new QTableWidgetItem(t.date.toString("yyyy-MM-dd")));

        // For transfers, we need to determine direction from notes or amount
        // Positive amount = into this account, Negative = out of this account
        QString fromAccount = (t.totalAmount < 0) ? Frontier::accountTypeToString(t.account) : 
                              (t.account == Frontier::AccountType::Company ? "Personal" : "Company");
        QString toAccount = (t.totalAmount >= 0) ? Frontier::accountTypeToString(t.account) :
                            (t.account == Frontier::AccountType::Company ? "Personal" : "Company");

        m_recentTable->setItem(row, 1, new QTableWidgetItem(Frontier::accountTypeToString(t.account)));
        m_recentTable->setItem(row, 2, new QTableWidgetItem(toAccount));

        auto *amountItem = new QTableWidgetItem(formatCurrency(qAbs(t.totalAmount)));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_recentTable->setItem(row, 3, amountItem);

        m_recentTable->setItem(row, 4, new QTableWidgetItem(t.notes));
    }
}

void AccountsTab::updateBalances()
{
    Frontier::AccountBalance balance = m_database->calculateBalances();

    m_companyBalanceLabel->setText(formatCurrency(balance.companyBalance));
    m_personalBalanceLabel->setText(formatCurrency(balance.personalBalance));
    m_totalBalanceLabel->setText(formatCurrency(balance.total()));

    // Calculate percentages
    double total = balance.total();
    int companyPercent = 0;
    int personalPercent = 0;

    if (total > 0) {
        companyPercent = static_cast<int>((balance.companyBalance / total) * 100);
        personalPercent = 100 - companyPercent;
    }

    m_companyPercentLabel->setText(QString("%1% of total").arg(companyPercent));
    m_personalPercentLabel->setText(QString("%1% of total").arg(personalPercent));

    m_splitBar->setValue(companyPercent);
    m_splitLabel->setText(QString(tr("Company: %1% | Personal: %2%")).arg(companyPercent).arg(personalPercent));

    // Color total based on positive/negative
    if (total >= 0) {
        m_totalBalanceLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #2e7d32;");
    } else {
        m_totalBalanceLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #c62828;");
    }
}

// =============================================================================
// Slots
// =============================================================================

void AccountsTab::onTransfer()
{
    double amount = m_transferAmountSpin->value();
    QString fromAccount = m_fromAccountCombo->currentData().toString();
    QString toAccount = m_toAccountCombo->currentData().toString();

    if (fromAccount == toAccount) {
        QMessageBox::warning(this, tr("Invalid Transfer"), tr("Cannot transfer to the same account."));
        return;
    }

    if (amount <= 0) {
        QMessageBox::warning(this, tr("Invalid Amount"), tr("Please enter a positive amount."));
        return;
    }

    // Create two transactions: one out, one in
    Frontier::Transaction outTransaction;
    outTransaction.date = QDate::currentDate();
    outTransaction.type = Frontier::TransactionType::Transfer;
    outTransaction.account = Frontier::stringToAccountType(fromAccount);
    outTransaction.item = QString("Transfer to %1").arg(toAccount);
    outTransaction.category = "Transfer";
    outTransaction.quantity = 1;
    outTransaction.unitPrice = amount;
    outTransaction.totalAmount = -amount;  // Negative = out
    outTransaction.notes = QString("Transfer to %1").arg(toAccount);

    Frontier::Transaction inTransaction;
    inTransaction.date = QDate::currentDate();
    inTransaction.type = Frontier::TransactionType::Transfer;
    inTransaction.account = Frontier::stringToAccountType(toAccount);
    inTransaction.item = QString("Transfer from %1").arg(fromAccount);
    inTransaction.category = "Transfer";
    inTransaction.quantity = 1;
    inTransaction.unitPrice = amount;
    inTransaction.totalAmount = amount;  // Positive = in
    inTransaction.notes = QString("Transfer from %1").arg(fromAccount);

    bool success = m_database->addTransaction(outTransaction) && m_database->addTransaction(inTransaction);

    if (success) {
        refreshData();
        emit transferCompleted();
        QMessageBox::information(this, tr("Transfer Complete"),
            QString(tr("Transferred %1 from %2 to %3."))
                .arg(formatCurrency(amount))
                .arg(fromAccount)
                .arg(toAccount));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to complete transfer."));
    }
}
