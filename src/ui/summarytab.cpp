/**
 * @file summarytab.cpp
 * @brief Income & Expense Summary implementation
 */

#include "summarytab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QHeaderView>
#include <QSplitter>
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

SummaryTab::SummaryTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void SummaryTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Period selection
    auto *periodGroup = new QGroupBox(tr("Report Period"));
    auto *periodLayout = new QHBoxLayout(periodGroup);

    periodLayout->addWidget(new QLabel(tr("Period:")));
    m_periodCombo = new QComboBox();
    m_periodCombo->addItem(tr("This Month"), "this_month");
    m_periodCombo->addItem(tr("Last Month"), "last_month");
    m_periodCombo->addItem(tr("This Year"), "this_year");
    m_periodCombo->addItem(tr("Last 30 Days"), "last_30");
    m_periodCombo->addItem(tr("Last 90 Days"), "last_90");
    m_periodCombo->addItem(tr("Custom Range"), "custom");
    connect(m_periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SummaryTab::onPeriodChanged);
    periodLayout->addWidget(m_periodCombo);

    periodLayout->addSpacing(20);

    periodLayout->addWidget(new QLabel(tr("From:")));
    m_fromDateEdit = new QDateEdit(QDate::currentDate().addMonths(-1));
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_fromDateEdit->setEnabled(false);
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &SummaryTab::updateSummary);
    periodLayout->addWidget(m_fromDateEdit);

    periodLayout->addWidget(new QLabel(tr("To:")));
    m_toDateEdit = new QDateEdit(QDate::currentDate());
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_toDateEdit->setEnabled(false);
    connect(m_toDateEdit, &QDateEdit::dateChanged, this, &SummaryTab::updateSummary);
    periodLayout->addWidget(m_toDateEdit);

    periodLayout->addStretch();

    m_refreshBtn = new QPushButton(tr("Refresh"));
    connect(m_refreshBtn, &QPushButton::clicked, this, &SummaryTab::refreshData);
    periodLayout->addWidget(m_refreshBtn);

    mainLayout->addWidget(periodGroup);

    // Overview panel
    mainLayout->addWidget(createOverviewPanel());

    // Breakdown panel
    mainLayout->addWidget(createBreakdownPanel(), 1);
}

QWidget* SummaryTab::createOverviewPanel()
{
    auto *group = new QGroupBox(tr("Financial Overview"));
    auto *layout = new QHBoxLayout(group);
    layout->setSpacing(30);

    // Total Income
    auto *incomeLayout = new QVBoxLayout();
    incomeLayout->addWidget(new QLabel(tr("Total Income")));
    m_totalIncomeLabel = new QLabel("$0");
    m_totalIncomeLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #2e7d32;");
    incomeLayout->addWidget(m_totalIncomeLabel);
    layout->addLayout(incomeLayout);

    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::VLine);
    layout->addWidget(sep1);

    // Total Expenses
    auto *expenseLayout = new QVBoxLayout();
    expenseLayout->addWidget(new QLabel(tr("Total Expenses")));
    m_totalExpensesLabel = new QLabel("$0");
    m_totalExpensesLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #c62828;");
    expenseLayout->addWidget(m_totalExpensesLabel);
    layout->addLayout(expenseLayout);

    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::VLine);
    layout->addWidget(sep2);

    // Net Profit
    auto *netLayout = new QVBoxLayout();
    netLayout->addWidget(new QLabel(tr("Net Profit")));
    m_netProfitLabel = new QLabel("$0");
    m_netProfitLabel->setStyleSheet("font-size: 28px; font-weight: bold;");
    netLayout->addWidget(m_netProfitLabel);
    layout->addLayout(netLayout);

    auto *sep3 = new QFrame();
    sep3->setFrameShape(QFrame::VLine);
    layout->addWidget(sep3);

    // Profit Margin
    auto *marginLayout = new QVBoxLayout();
    marginLayout->addWidget(new QLabel(tr("Profit Margin")));
    m_profitMarginLabel = new QLabel("0%");
    m_profitMarginLabel->setStyleSheet("font-size: 28px; font-weight: bold;");
    marginLayout->addWidget(m_profitMarginLabel);
    layout->addLayout(marginLayout);

    layout->addStretch();

    return group;
}

QWidget* SummaryTab::createBreakdownPanel()
{
    auto *splitter = new QSplitter(Qt::Horizontal);

    // Income breakdown
    auto *incomeGroup = new QGroupBox(tr("Income by Category"));
    auto *incomeLayout = new QVBoxLayout(incomeGroup);

    m_incomeTable = new QTableWidget();
    m_incomeTable->setColumnCount(3);
    m_incomeTable->setHorizontalHeaderLabels({tr("Category"), tr("Amount"), tr("% of Total")});
    m_incomeTable->setAlternatingRowColors(true);
    m_incomeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_incomeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_incomeTable->verticalHeader()->setVisible(false);
    m_incomeTable->setSortingEnabled(true);

    auto *incomeHeader = m_incomeTable->horizontalHeader();
    incomeHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    incomeHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    incomeHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    incomeLayout->addWidget(m_incomeTable);
    splitter->addWidget(incomeGroup);

    // Expense breakdown
    auto *expenseGroup = new QGroupBox(tr("Expenses by Category"));
    auto *expenseLayout = new QVBoxLayout(expenseGroup);

    m_expenseTable = new QTableWidget();
    m_expenseTable->setColumnCount(3);
    m_expenseTable->setHorizontalHeaderLabels({tr("Category"), tr("Amount"), tr("% of Total")});
    m_expenseTable->setAlternatingRowColors(true);
    m_expenseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_expenseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_expenseTable->verticalHeader()->setVisible(false);
    m_expenseTable->setSortingEnabled(true);

    auto *expenseHeader = m_expenseTable->horizontalHeader();
    expenseHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    expenseHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    expenseHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    expenseLayout->addWidget(m_expenseTable);
    splitter->addWidget(expenseGroup);

    return splitter;
}

// =============================================================================
// Data Loading
// =============================================================================

void SummaryTab::refreshData()
{
    onPeriodChanged();
}

void SummaryTab::onPeriodChanged()
{
    QString period = m_periodCombo->currentData().toString();
    QDate today = QDate::currentDate();
    QDate from, to;

    if (period == "this_month") {
        from = QDate(today.year(), today.month(), 1);
        to = today;
        m_fromDateEdit->setEnabled(false);
        m_toDateEdit->setEnabled(false);
    } else if (period == "last_month") {
        QDate lastMonth = today.addMonths(-1);
        from = QDate(lastMonth.year(), lastMonth.month(), 1);
        to = QDate(lastMonth.year(), lastMonth.month(), lastMonth.daysInMonth());
        m_fromDateEdit->setEnabled(false);
        m_toDateEdit->setEnabled(false);
    } else if (period == "this_year") {
        from = QDate(today.year(), 1, 1);
        to = today;
        m_fromDateEdit->setEnabled(false);
        m_toDateEdit->setEnabled(false);
    } else if (period == "last_30") {
        from = today.addDays(-30);
        to = today;
        m_fromDateEdit->setEnabled(false);
        m_toDateEdit->setEnabled(false);
    } else if (period == "last_90") {
        from = today.addDays(-90);
        to = today;
        m_fromDateEdit->setEnabled(false);
        m_toDateEdit->setEnabled(false);
    } else {  // custom
        from = m_fromDateEdit->date();
        to = m_toDateEdit->date();
        m_fromDateEdit->setEnabled(true);
        m_toDateEdit->setEnabled(true);
    }

    m_fromDateEdit->blockSignals(true);
    m_toDateEdit->blockSignals(true);
    m_fromDateEdit->setDate(from);
    m_toDateEdit->setDate(to);
    m_fromDateEdit->blockSignals(false);
    m_toDateEdit->blockSignals(false);

    updateSummary();
}

void SummaryTab::updateSummary()
{
    QDate from = m_fromDateEdit->date();
    QDate to = m_toDateEdit->date();

    Frontier::FinanceSummary summary = m_database->getFinanceSummary(from, to);

    // Update overview
    m_totalIncomeLabel->setText(formatCurrency(summary.totalIncome));
    m_totalExpensesLabel->setText(formatCurrency(summary.totalExpenses));
    m_netProfitLabel->setText(formatCurrency(summary.netProfit));

    // Profit margin
    double margin = 0;
    if (summary.totalIncome > 0) {
        margin = (summary.netProfit / summary.totalIncome) * 100;
    }
    m_profitMarginLabel->setText(QString("%1%").arg(margin, 0, 'f', 1));

    // Color net profit
    if (summary.netProfit >= 0) {
        m_netProfitLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #2e7d32;");
        m_profitMarginLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #2e7d32;");
    } else {
        m_netProfitLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #c62828;");
        m_profitMarginLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #c62828;");
    }

    // Income breakdown
    m_incomeTable->setSortingEnabled(false);
    m_incomeTable->setRowCount(summary.incomeByCategory.size());
    int row = 0;
    for (auto it = summary.incomeByCategory.begin(); it != summary.incomeByCategory.end(); ++it, ++row) {
        m_incomeTable->setItem(row, 0, new QTableWidgetItem(it.key()));

        auto *amountItem = new QTableWidgetItem(formatCurrency(it.value()));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        amountItem->setForeground(QColor("#2e7d32"));
        amountItem->setData(Qt::UserRole, it.value());
        m_incomeTable->setItem(row, 1, amountItem);

        double percent = (summary.totalIncome > 0) ? (it.value() / summary.totalIncome) * 100 : 0;
        auto *percentItem = new QTableWidgetItem(QString("%1%").arg(percent, 0, 'f', 1));
        percentItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_incomeTable->setItem(row, 2, percentItem);
    }
    m_incomeTable->setSortingEnabled(true);

    // Expense breakdown
    m_expenseTable->setSortingEnabled(false);
    m_expenseTable->setRowCount(summary.expensesByCategory.size());
    row = 0;
    for (auto it = summary.expensesByCategory.begin(); it != summary.expensesByCategory.end(); ++it, ++row) {
        m_expenseTable->setItem(row, 0, new QTableWidgetItem(it.key()));

        auto *amountItem = new QTableWidgetItem(formatCurrency(it.value()));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        amountItem->setForeground(QColor("#c62828"));
        amountItem->setData(Qt::UserRole, it.value());
        m_expenseTable->setItem(row, 1, amountItem);

        double percent = (summary.totalExpenses > 0) ? (it.value() / summary.totalExpenses) * 100 : 0;
        auto *percentItem = new QTableWidgetItem(QString("%1%").arg(percent, 0, 'f', 1));
        percentItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_expenseTable->setItem(row, 2, percentItem);
    }
    m_expenseTable->setSortingEnabled(true);
}
