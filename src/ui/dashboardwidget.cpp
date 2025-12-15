/**
 * @file dashboardwidget.cpp
 * @brief Dashboard implementation
 */

#include "dashboardwidget.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QScrollArea>
#include <QHeaderView>
#include <QLocale>
#include <QDate>
#include <QSettings>
#include <algorithm>

namespace {
QString formatCurrency(double amount)
{
    QLocale locale(QLocale::English);
    return "$" + locale.toString(static_cast<qint64>(qRound(amount)));
}

QString formatPower(double kw)
{
    QLocale locale(QLocale::English);
    QString sign = kw >= 0 ? "+" : "";
    return sign + locale.toString(static_cast<qint64>(qRound(kw))) + " kW";
}
}

// =============================================================================
// Constructor & Setup
// =============================================================================

DashboardWidget::DashboardWidget(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_currentJournalDate(QDate::currentDate())
{
    setupUi();
    refreshData();
}

void DashboardWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Create scrollable area for dashboard content
    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *contentWidget = new QWidget();
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(15);

    // Add sections
    contentLayout->addWidget(createStatusBanner());
    contentLayout->addWidget(createFinancialSummary());
    contentLayout->addWidget(createCapitalPlanSummary());
    contentLayout->addWidget(createQuickActions());
    contentLayout->addWidget(createDailyJournal());
    contentLayout->addWidget(createRecentActivity());
    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

// =============================================================================
// Status Banner
// =============================================================================

QWidget* DashboardWidget::createStatusBanner()
{
    auto *banner = new QFrame();
    banner->setStyleSheet(
        "QFrame { background-color: #c8e6c9; border-radius: 8px; padding: 10px; }"
        );

    auto *layout = new QHBoxLayout(banner);

    // Status icon and text
    m_statusLabel = new QLabel(tr("✓ ALL SYSTEMS OPERATIONAL"));
    m_statusLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2e7d32;");
    layout->addWidget(m_statusLabel);

    layout->addStretch();

    // Day counter
    m_dayLabel = new QLabel(tr("Day: 1"));
    m_dayLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2e7d32;");
    layout->addWidget(m_dayLabel);

    layout->addSpacing(20);

    // Refresh button
    m_refreshBtn = new QPushButton(tr("⟳ Refresh"));
    m_refreshBtn->setStyleSheet(
        "QPushButton { background-color: white; border: 1px solid #2e7d32; "
        "border-radius: 4px; padding: 5px 15px; color: #2e7d32; }"
        "QPushButton:hover { background-color: #e8f5e9; }"
        );
    connect(m_refreshBtn, &QPushButton::clicked, this, &DashboardWidget::refreshData);
    layout->addWidget(m_refreshBtn);

    return banner;
}

// =============================================================================
// Financial Summary
// =============================================================================

QWidget* DashboardWidget::createFinancialSummary()
{
    auto *container = new QWidget();
    auto *layout = new QHBoxLayout(container);
    layout->setSpacing(15);

    // Net Worth Card
    auto *netWorthCard = new QFrame();
    netWorthCard->setFrameStyle(QFrame::StyledPanel);
    netWorthCard->setStyleSheet("QFrame { background-color: #fff8e1; border-radius: 8px; padding: 15px; }");
    auto *nwLayout = new QVBoxLayout(netWorthCard);
    auto *nwTitle = new QLabel(tr("💰 NET WORTH"));
    nwTitle->setStyleSheet("font-size: 12px; color: #666;");
    nwLayout->addWidget(nwTitle);
    m_netWorthLabel = new QLabel("$0");
    m_netWorthLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #f9a825;");
    nwLayout->addWidget(m_netWorthLabel);
    auto *nwSub = new QLabel(tr("Combined total"));
    nwSub->setStyleSheet("font-size: 11px; color: #999;");
    nwLayout->addWidget(nwSub);
    layout->addWidget(netWorthCard);

    // Company Card
    auto *companyCard = new QFrame();
    companyCard->setFrameStyle(QFrame::StyledPanel);
    companyCard->setStyleSheet("QFrame { background-color: #e3f2fd; border-radius: 8px; padding: 15px; }");
    auto *coLayout = new QVBoxLayout(companyCard);
    auto *coTitle = new QLabel(tr("🏢 COMPANY"));
    coTitle->setStyleSheet("font-size: 12px; color: #666;");
    coLayout->addWidget(coTitle);
    m_companyLabel = new QLabel("$0");
    m_companyLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #1976d2;");
    coLayout->addWidget(m_companyLabel);
    auto *coSub = new QLabel(tr("90% of sales"));
    coSub->setStyleSheet("font-size: 11px; color: #999;");
    coLayout->addWidget(coSub);
    layout->addWidget(companyCard);

    // Personal Card
    auto *personalCard = new QFrame();
    personalCard->setFrameStyle(QFrame::StyledPanel);
    personalCard->setStyleSheet("QFrame { background-color: #f3e5f5; border-radius: 8px; padding: 15px; }");
    auto *perLayout = new QVBoxLayout(personalCard);
    auto *perTitle = new QLabel(tr("👤 PERSONAL"));
    perTitle->setStyleSheet("font-size: 12px; color: #666;");
    perLayout->addWidget(perTitle);
    m_personalLabel = new QLabel("$0");
    m_personalLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #7b1fa2;");
    perLayout->addWidget(m_personalLabel);
    auto *perSub = new QLabel(tr("10% of sales"));
    perSub->setStyleSheet("font-size: 11px; color: #999;");
    perLayout->addWidget(perSub);
    layout->addWidget(personalCard);

    // Transactions Card
    auto *txCard = new QFrame();
    txCard->setFrameStyle(QFrame::StyledPanel);
    txCard->setStyleSheet("QFrame { background-color: #ffebee; border-radius: 8px; padding: 15px; }");
    auto *txLayout = new QVBoxLayout(txCard);
    auto *txTitle = new QLabel(tr("📊 TRANSACTIONS"));
    txTitle->setStyleSheet("font-size: 12px; color: #666;");
    txLayout->addWidget(txTitle);
    m_transactionCountLabel = new QLabel("0");
    m_transactionCountLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #c62828;");
    txLayout->addWidget(m_transactionCountLabel);
    auto *txSub = new QLabel(tr("Total count"));
    txSub->setStyleSheet("font-size: 11px; color: #999;");
    txLayout->addWidget(txSub);
    layout->addWidget(txCard);

    return container;
}

// =============================================================================
// Capital Plan Summary
// =============================================================================

QWidget* DashboardWidget::createCapitalPlanSummary()
{
    auto *group = new QGroupBox(tr("📋 Capital Plan Summary"));
    auto *layout = new QHBoxLayout(group);
    layout->setSpacing(15);

    // Equipment Total
    auto *equipCard = new QFrame();
    equipCard->setFrameStyle(QFrame::StyledPanel);
    auto *equipLayout = new QVBoxLayout(equipCard);
    equipLayout->addWidget(new QLabel(tr("Equipment Plan")));
    m_equipmentTotalLabel = new QLabel("$0");
    m_equipmentTotalLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #ff9800;");
    equipLayout->addWidget(m_equipmentTotalLabel);
    layout->addWidget(equipCard);

    // Facility Total
    auto *facilityCard = new QFrame();
    facilityCard->setFrameStyle(QFrame::StyledPanel);
    auto *facilityLayout = new QVBoxLayout(facilityCard);
    facilityLayout->addWidget(new QLabel(tr("Facility Plan")));
    m_facilityTotalLabel = new QLabel("$0");
    m_facilityTotalLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #ff9800;");
    facilityLayout->addWidget(m_facilityTotalLabel);
    layout->addWidget(facilityCard);

    // Power Balance
    auto *powerCard = new QFrame();
    powerCard->setFrameStyle(QFrame::StyledPanel);
    auto *powerLayout = new QVBoxLayout(powerCard);
    powerLayout->addWidget(new QLabel(tr("Power Balance")));
    m_powerBalanceLabel = new QLabel("0 kW");
    m_powerBalanceLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    powerLayout->addWidget(m_powerBalanceLabel);
    layout->addWidget(powerCard);

    // Affordability
    auto *affordCard = new QFrame();
    affordCard->setFrameStyle(QFrame::StyledPanel);
    auto *affordLayout = new QVBoxLayout(affordCard);
    affordLayout->addWidget(new QLabel(tr("Affordability")));
    m_affordabilityBar = new QProgressBar();
    m_affordabilityBar->setMinimum(0);
    m_affordabilityBar->setMaximum(100);
    m_affordabilityBar->setTextVisible(true);
    affordLayout->addWidget(m_affordabilityBar);
    m_affordabilityLabel = new QLabel(tr("No plan"));
    m_affordabilityLabel->setStyleSheet("font-size: 12px;");
    affordLayout->addWidget(m_affordabilityLabel);
    layout->addWidget(affordCard);

    return group;
}

// =============================================================================
// Quick Actions
// =============================================================================

QWidget* DashboardWidget::createQuickActions()
{
    auto *group = new QGroupBox(tr("⚡ Quick Actions"));
    auto *layout = new QHBoxLayout(group);

    m_addTransactionBtn = new QPushButton(tr("➕ Add Transaction"));
    m_addTransactionBtn->setStyleSheet(
        "QPushButton { background-color: #4caf50; color: white; padding: 10px 20px; "
        "border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background-color: #388e3c; }"
        );
    connect(m_addTransactionBtn, &QPushButton::clicked, this, &DashboardWidget::addTransactionRequested);
    layout->addWidget(m_addTransactionBtn);

    m_capitalPlannerBtn = new QPushButton(tr("📋 Capital Planner"));
    m_capitalPlannerBtn->setStyleSheet(
        "QPushButton { background-color: #2196f3; color: white; padding: 10px 20px; "
        "border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1976d2; }"
        );
    connect(m_capitalPlannerBtn, &QPushButton::clicked, this, &DashboardWidget::navigateToCapitalPlanner);
    layout->addWidget(m_capitalPlannerBtn);

    m_ledgerBtn = new QPushButton(tr("📒 Ledger"));
    m_ledgerBtn->setStyleSheet(
        "QPushButton { background-color: #ff9800; color: white; padding: 10px 20px; "
        "border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background-color: #f57c00; }"
        );
    connect(m_ledgerBtn, &QPushButton::clicked, this, &DashboardWidget::navigateToLedger);
    layout->addWidget(m_ledgerBtn);

    m_dataHubBtn = new QPushButton(tr("📦 Data Hub"));
    m_dataHubBtn->setStyleSheet(
        "QPushButton { background-color: #9c27b0; color: white; padding: 10px 20px; "
        "border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background-color: #7b1fa2; }"
        );
    connect(m_dataHubBtn, &QPushButton::clicked, this, &DashboardWidget::navigateToDataHub);
    layout->addWidget(m_dataHubBtn);

    layout->addStretch();

    return group;
}

// =============================================================================
// Daily Journal
// =============================================================================

QWidget* DashboardWidget::createDailyJournal()
{
    auto *group = new QGroupBox(tr("📅 Daily Journal"));
    auto *mainLayout = new QVBoxLayout(group);

    // Date navigation row
    auto *dateLayout = new QHBoxLayout();

    dateLayout->addWidget(new QLabel(tr("Date:")));

    m_journalDateEdit = new QDateEdit(QDate::currentDate());
    m_journalDateEdit->setCalendarPopup(true);
    m_journalDateEdit->setDisplayFormat("yyyy-MM-dd");
    connect(m_journalDateEdit, &QDateEdit::dateChanged, this, &DashboardWidget::onDateChanged);
    dateLayout->addWidget(m_journalDateEdit);

    auto *prevBtn = new QPushButton(tr("◄ Prev"));
    connect(prevBtn, &QPushButton::clicked, this, &DashboardWidget::onPrevDay);
    dateLayout->addWidget(prevBtn);

    auto *nextBtn = new QPushButton(tr("Next ►"));
    connect(nextBtn, &QPushButton::clicked, this, &DashboardWidget::onNextDay);
    dateLayout->addWidget(nextBtn);

    auto *todayBtn = new QPushButton(tr("Today"));
    connect(todayBtn, &QPushButton::clicked, this, &DashboardWidget::onToday);
    dateLayout->addWidget(todayBtn);

    dateLayout->addStretch();

    // Day's income/expenses summary
    m_dayIncomeLabel = new QLabel(tr("Income: $0"));
    m_dayIncomeLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
    dateLayout->addWidget(m_dayIncomeLabel);

    m_dayExpensesLabel = new QLabel(tr("Expenses: $0"));
    m_dayExpensesLabel->setStyleSheet("color: #c62828; font-weight: bold;");
    dateLayout->addWidget(m_dayExpensesLabel);

    m_dayNetLabel = new QLabel(tr("Net: $0"));
    m_dayNetLabel->setStyleSheet("font-weight: bold;");
    dateLayout->addWidget(m_dayNetLabel);

    mainLayout->addLayout(dateLayout);

    // Content: Activities table and Notes side by side
    auto *contentLayout = new QHBoxLayout();

    // Day's Activities Table
    auto *activitiesGroup = new QGroupBox(tr("Day's Activities"));
    auto *activitiesLayout = new QVBoxLayout(activitiesGroup);

    m_dayActivitiesTable = new QTableWidget();
    m_dayActivitiesTable->setColumnCount(5);
    m_dayActivitiesTable->setHorizontalHeaderLabels({
        tr("Time"), tr("Type"), tr("Description"), tr("Amount"), tr("Account")
    });
    m_dayActivitiesTable->setAlternatingRowColors(true);
    m_dayActivitiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dayActivitiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dayActivitiesTable->verticalHeader()->setVisible(false);
    m_dayActivitiesTable->setMaximumHeight(150);

    auto *actHeader = m_dayActivitiesTable->horizontalHeader();
    actHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    actHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    actHeader->setSectionResizeMode(2, QHeaderView::Stretch);
    actHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    actHeader->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    activitiesLayout->addWidget(m_dayActivitiesTable);
    contentLayout->addWidget(activitiesGroup, 2);

    // Notes
    auto *notesGroup = new QGroupBox(tr("Notes"));
    auto *notesLayout = new QVBoxLayout(notesGroup);

    m_notesEdit = new QTextEdit();
    m_notesEdit->setPlaceholderText(tr("Add notes for this day..."));
    m_notesEdit->setMaximumHeight(120);
    notesLayout->addWidget(m_notesEdit);

    m_saveNotesBtn = new QPushButton(tr("💾 Save"));
    m_saveNotesBtn->setStyleSheet("background-color: #4caf50; color: white;");
    connect(m_saveNotesBtn, &QPushButton::clicked, this, &DashboardWidget::onSaveNotes);
    notesLayout->addWidget(m_saveNotesBtn, 0, Qt::AlignRight);

    contentLayout->addWidget(notesGroup, 1);

    mainLayout->addLayout(contentLayout);

    return group;
}

// =============================================================================
// Recent Activity
// =============================================================================

QWidget* DashboardWidget::createRecentActivity()
{
    auto *group = new QGroupBox(tr("📜 Recent Activity"));
    auto *layout = new QVBoxLayout(group);

    m_recentActivityTable = new QTableWidget();
    m_recentActivityTable->setColumnCount(5);
    m_recentActivityTable->setHorizontalHeaderLabels({
        tr("Date"), tr("Description"), tr("Amount"), tr("Account"), tr("Balance")
    });
    m_recentActivityTable->setAlternatingRowColors(true);
    m_recentActivityTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentActivityTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentActivityTable->verticalHeader()->setVisible(false);
    m_recentActivityTable->setMaximumHeight(200);

    auto *header = m_recentActivityTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    layout->addWidget(m_recentActivityTable);

    // View All link
    m_viewAllBtn = new QPushButton(tr("View All Transactions →"));
    m_viewAllBtn->setFlat(true);
    m_viewAllBtn->setStyleSheet("color: #1976d2; text-decoration: underline;");
    connect(m_viewAllBtn, &QPushButton::clicked, this, &DashboardWidget::onViewAllTransactions);
    layout->addWidget(m_viewAllBtn, 0, Qt::AlignLeft);

    return group;
}

// =============================================================================
// Data Update Methods
// =============================================================================

void DashboardWidget::refreshData()
{
    updateFinancialSummary();
    updateCapitalPlanSummary();
    updateDailyJournal();
    updateRecentActivity();
}

void DashboardWidget::updateFinancialSummary()
{
    auto balances = m_database->calculateBalances();

    m_netWorthLabel->setText(formatCurrency(balances.total()));
    m_companyLabel->setText(formatCurrency(balances.companyBalance));
    m_personalLabel->setText(formatCurrency(balances.personalBalance));

    // Get transaction count
    auto transactions = m_database->getAllTransactions();
    m_transactionCountLabel->setText(QString::number(transactions.size()));
}

void DashboardWidget::updateCapitalPlanSummary()
{
    // Get equipment plan total
    double equipmentTotal = 0;
    auto equipmentPlan = m_database->getEquipmentPlan();
    for (const auto &item : equipmentPlan) {
        equipmentTotal += item.totalCost;
    }
    m_equipmentTotalLabel->setText(formatCurrency(equipmentTotal));

    // Get facility plan totals
    double facilityTotal = 0;
    double powerRequired = 0;
    double powerGenerated = 0;
    auto facilityPlan = m_database->getFacilityPlan();
    for (const auto &item : facilityPlan) {
        facilityTotal += item.totalCost;
        powerRequired += item.totalPowerKw;
        powerGenerated += item.totalGeneratedKw;
    }
    m_facilityTotalLabel->setText(formatCurrency(facilityTotal));

    // Power balance
    double powerBalance = powerGenerated - powerRequired;
    m_powerBalanceLabel->setText(formatPower(powerBalance));
    if (powerBalance >= 0) {
        m_powerBalanceLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2e7d32;");
    } else {
        m_powerBalanceLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #c62828;");
    }

    // Affordability
    double grandTotal = equipmentTotal + facilityTotal;
    auto balances = m_database->calculateBalances();
    double totalAvailable = balances.total();

    if (grandTotal == 0) {
        m_affordabilityBar->setValue(0);
        m_affordabilityBar->setFormat(tr("No plan"));
        m_affordabilityLabel->setText(tr("Add items to plan"));
        m_affordabilityLabel->setStyleSheet("font-size: 12px; color: #666;");
    } else {
        int percent = qBound(0, static_cast<int>((grandTotal / totalAvailable) * 100), 100);
        m_affordabilityBar->setValue(percent);
        m_affordabilityBar->setFormat(QString("%1%").arg(percent));

        double remaining = totalAvailable - grandTotal;
        if (remaining >= 0) {
            m_affordabilityLabel->setText(tr("✓ Can afford (%1 remaining)").arg(formatCurrency(remaining)));
            m_affordabilityLabel->setStyleSheet("font-size: 12px; color: #2e7d32; font-weight: bold;");
            m_affordabilityBar->setStyleSheet("QProgressBar::chunk { background-color: #4caf50; }");
        } else {
            m_affordabilityLabel->setText(tr("✗ Need %1 more").arg(formatCurrency(-remaining)));
            m_affordabilityLabel->setStyleSheet("font-size: 12px; color: #c62828; font-weight: bold;");
            m_affordabilityBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");
        }
    }
}

void DashboardWidget::updateDailyJournal()
{
    QDate date = m_journalDateEdit->date();

    // Get transactions for this date
    auto transactions = m_database->getTransactionsByDateRange(date, date);

    m_dayActivitiesTable->setRowCount(transactions.size());

    double dayIncome = 0;
    double dayExpenses = 0;

    for (int row = 0; row < transactions.size(); ++row) {
        const auto &t = transactions[row];

        // Time (we don't have time, just show "-")
        m_dayActivitiesTable->setItem(row, 0, new QTableWidgetItem("-"));

        // Type
        QString typeStr = Frontier::transactionTypeToString(t.type);
        auto *typeItem = new QTableWidgetItem(typeStr);
        if (t.type == Frontier::TransactionType::Sale || t.type == Frontier::TransactionType::Opening) {
            typeItem->setForeground(QColor("#2e7d32"));
            dayIncome += t.totalAmount;
        } else if (t.type == Frontier::TransactionType::Purchase || t.type == Frontier::TransactionType::Fuel) {
            typeItem->setForeground(QColor("#c62828"));
            dayExpenses += t.totalAmount;
        }
        m_dayActivitiesTable->setItem(row, 1, typeItem);

        // Description
        m_dayActivitiesTable->setItem(row, 2, new QTableWidgetItem(t.item));

        // Amount
        auto *amountItem = new QTableWidgetItem(formatCurrency(t.totalAmount));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_dayActivitiesTable->setItem(row, 3, amountItem);

        // Account
        m_dayActivitiesTable->setItem(row, 4, new QTableWidgetItem(
                                                  Frontier::accountTypeToString(t.account)));
    }

    // Update day summary
    m_dayIncomeLabel->setText(QString(tr("Income: %1")).arg(formatCurrency(dayIncome)));
    m_dayExpensesLabel->setText(QString(tr("Expenses: %1")).arg(formatCurrency(dayExpenses)));

    double dayNet = dayIncome - dayExpenses;
    m_dayNetLabel->setText(QString(tr("Net: %1%2")).arg(dayNet >= 0 ? "+" : "").arg(formatCurrency(dayNet)));
    if (dayNet >= 0) {
        m_dayNetLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");
    } else {
        m_dayNetLabel->setStyleSheet("font-weight: bold; color: #c62828;");
    }

    // Load notes for this date
    loadNotesForDate(date);
}

void DashboardWidget::updateRecentActivity()
{
    // Get recent transactions (last 10)
    auto transactions = m_database->getAllTransactions();

    // Sort by date descending, then by ID descending
    std::sort(transactions.begin(), transactions.end(),
              [](const Frontier::Transaction &a, const Frontier::Transaction &b) {
                  if (a.date != b.date) return a.date > b.date;
                  return a.id.value_or(0) > b.id.value_or(0);
              });

    int maxRows = qMin(10, static_cast<int>(transactions.size()));
    m_recentActivityTable->setRowCount(maxRows);

    // Calculate running balance (from most recent backwards)
    auto balances = m_database->calculateBalances();
    double runningBalance = balances.total();

    for (int row = 0; row < maxRows; ++row) {
        const auto &t = transactions[row];

        // Date
        m_recentActivityTable->setItem(row, 0,
                                       new QTableWidgetItem(t.date.toString("yyyy-MM-dd")));

        // Description
        m_recentActivityTable->setItem(row, 1, new QTableWidgetItem(t.item));

        // Amount
        QString amountStr;
        bool isIncome = (t.type == Frontier::TransactionType::Sale ||
                         t.type == Frontier::TransactionType::Opening);
        if (isIncome) {
            amountStr = "+" + formatCurrency(t.totalAmount);
        } else {
            amountStr = "-" + formatCurrency(t.totalAmount);
        }
        auto *amountItem = new QTableWidgetItem(amountStr);
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        amountItem->setForeground(isIncome ? QColor("#2e7d32") : QColor("#c62828"));
        m_recentActivityTable->setItem(row, 2, amountItem);

        // Account
        m_recentActivityTable->setItem(row, 3, new QTableWidgetItem(
                                                   Frontier::accountTypeToString(t.account)));

        // Balance (show current balance for first row, then we'd need historical)
        auto *balanceItem = new QTableWidgetItem(formatCurrency(runningBalance));
        balanceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_recentActivityTable->setItem(row, 4, balanceItem);

        // Adjust running balance for next row (going backwards)
        if (isIncome) {
            runningBalance -= t.totalAmount;
        } else {
            runningBalance += t.totalAmount;
        }
    }
}

// =============================================================================
// Notes Persistence
// =============================================================================

void DashboardWidget::loadNotesForDate(const QDate &date)
{
    QSettings settings;
    QString key = QString("journal_notes/%1").arg(date.toString("yyyy-MM-dd"));
    m_notesEdit->setText(settings.value(key, "").toString());
}

void DashboardWidget::saveNotesForDate(const QDate &date)
{
    QSettings settings;
    QString key = QString("journal_notes/%1").arg(date.toString("yyyy-MM-dd"));
    settings.setValue(key, m_notesEdit->toPlainText());
}

// =============================================================================
// Slots
// =============================================================================

void DashboardWidget::onPrevDay()
{
    m_journalDateEdit->setDate(m_journalDateEdit->date().addDays(-1));
}

void DashboardWidget::onNextDay()
{
    m_journalDateEdit->setDate(m_journalDateEdit->date().addDays(1));
}

void DashboardWidget::onToday()
{
    m_journalDateEdit->setDate(QDate::currentDate());
}

void DashboardWidget::onDateChanged(const QDate &date)
{
    m_currentJournalDate = date;
    updateDailyJournal();
}

void DashboardWidget::onSaveNotes()
{
    saveNotesForDate(m_journalDateEdit->date());
}

void DashboardWidget::onViewAllTransactions()
{
    emit navigateToLedger();
}
