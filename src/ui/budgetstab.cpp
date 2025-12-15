/**
 * @file budgetstab.cpp
 * @brief Budgets & Forecasting implementation
 */

#include "budgetstab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFrame>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
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

BudgetsTab::BudgetsTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void BudgetsTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Month selection
    auto *monthGroup = new QGroupBox(tr("Budget Period"));
    auto *monthLayout = new QHBoxLayout(monthGroup);

    m_prevMonthBtn = new QPushButton(tr("◄"));
    m_prevMonthBtn->setMaximumWidth(40);
    connect(m_prevMonthBtn, &QPushButton::clicked, [this]() {
        int month = m_monthCombo->currentIndex();
        int year = m_yearSpin->value();
        if (month == 0) {
            month = 11;
            year--;
        } else {
            month--;
        }
        m_yearSpin->setValue(year);
        m_monthCombo->setCurrentIndex(month);
    });
    monthLayout->addWidget(m_prevMonthBtn);

    m_monthCombo = new QComboBox();
    m_monthCombo->addItems({
        tr("January"), tr("February"), tr("March"), tr("April"),
        tr("May"), tr("June"), tr("July"), tr("August"),
        tr("September"), tr("October"), tr("November"), tr("December")
    });
    m_monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BudgetsTab::onMonthChanged);
    monthLayout->addWidget(m_monthCombo);

    m_yearSpin = new QSpinBox();
    m_yearSpin->setRange(2020, 2100);
    m_yearSpin->setValue(QDate::currentDate().year());
    connect(m_yearSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &BudgetsTab::onMonthChanged);
    monthLayout->addWidget(m_yearSpin);

    m_nextMonthBtn = new QPushButton(tr("►"));
    m_nextMonthBtn->setMaximumWidth(40);
    connect(m_nextMonthBtn, &QPushButton::clicked, [this]() {
        int month = m_monthCombo->currentIndex();
        int year = m_yearSpin->value();
        if (month == 11) {
            month = 0;
            year++;
        } else {
            month++;
        }
        m_yearSpin->setValue(year);
        m_monthCombo->setCurrentIndex(month);
    });
    monthLayout->addWidget(m_nextMonthBtn);

    monthLayout->addStretch();

    mainLayout->addWidget(monthGroup);

    // Summary
    auto *summaryGroup = new QGroupBox(tr("Budget Summary"));
    auto *summaryLayout = new QHBoxLayout(summaryGroup);

    auto *budgetLayout = new QVBoxLayout();
    budgetLayout->addWidget(new QLabel(tr("Total Budget")));
    m_totalBudgetLabel = new QLabel("$0.00");
    m_totalBudgetLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    budgetLayout->addWidget(m_totalBudgetLabel);
    summaryLayout->addLayout(budgetLayout);

    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::VLine);
    summaryLayout->addWidget(sep1);

    auto *actualLayout = new QVBoxLayout();
    actualLayout->addWidget(new QLabel(tr("Actual Spent")));
    m_totalActualLabel = new QLabel("$0.00");
    m_totalActualLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    actualLayout->addWidget(m_totalActualLabel);
    summaryLayout->addLayout(actualLayout);

    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::VLine);
    summaryLayout->addWidget(sep2);

    auto *remainingLayout = new QVBoxLayout();
    remainingLayout->addWidget(new QLabel(tr("Remaining")));
    m_remainingLabel = new QLabel("$0.00");
    m_remainingLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    remainingLayout->addWidget(m_remainingLabel);
    summaryLayout->addLayout(remainingLayout);

    summaryLayout->addSpacing(30);

    m_overallProgress = new QProgressBar();
    m_overallProgress->setRange(0, 100);
    m_overallProgress->setMinimumWidth(200);
    m_overallProgress->setMinimumHeight(25);
    summaryLayout->addWidget(m_overallProgress);

    summaryLayout->addStretch();

    mainLayout->addWidget(summaryGroup);

    // Budget table
    auto *tableGroup = new QGroupBox(tr("Category Budgets"));
    auto *tableLayout = new QVBoxLayout(tableGroup);

    m_budgetTable = new QTableWidget();
    m_budgetTable->setColumnCount(5);
    m_budgetTable->setHorizontalHeaderLabels({
        tr("Category"), tr("Budget"), tr("Actual"), tr("Remaining"), tr("Progress")
    });
    m_budgetTable->setAlternatingRowColors(true);
    m_budgetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_budgetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_budgetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_budgetTable->verticalHeader()->setVisible(false);

    auto *header = m_budgetTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::Fixed);
    header->resizeSection(4, 150);

    connect(m_budgetTable, &QTableWidget::itemSelectionChanged, this, &BudgetsTab::onSelectionChanged);

    tableLayout->addWidget(m_budgetTable);

    // Buttons
    auto *btnLayout = new QHBoxLayout();

    m_addBtn = new QPushButton(tr("+ Add Budget"));
    m_addBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold;");
    connect(m_addBtn, &QPushButton::clicked, this, &BudgetsTab::onAddBudget);
    btnLayout->addWidget(m_addBtn);

    btnLayout->addStretch();

    m_editBtn = new QPushButton(tr("Edit"));
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &BudgetsTab::onEditBudget);
    btnLayout->addWidget(m_editBtn);

    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &BudgetsTab::onDeleteBudget);
    btnLayout->addWidget(m_deleteBtn);

    tableLayout->addLayout(btnLayout);

    mainLayout->addWidget(tableGroup, 1);

    // Forecast section
    auto *forecastGroup = new QGroupBox(tr("Forecast"));
    auto *forecastLayout = new QVBoxLayout(forecastGroup);

    m_forecastLabel = new QLabel(tr("Based on current spending patterns..."));
    m_forecastLabel->setWordWrap(true);
    m_forecastLabel->setStyleSheet("padding: 10px; background-color: #e3f2fd; border-radius: 5px;");
    forecastLayout->addWidget(m_forecastLabel);

    mainLayout->addWidget(forecastGroup);
}

// =============================================================================
// Data Loading
// =============================================================================

void BudgetsTab::refreshData()
{
    loadBudgets();
}

void BudgetsTab::onMonthChanged()
{
    loadBudgets();
}

void BudgetsTab::loadBudgets()
{
    int year = m_yearSpin->value();
    int month = m_monthCombo->currentIndex() + 1;

    // Get budgets for selected month
    auto budgets = m_database->getBudgetsForMonth(year, month);

    // Get actual spending for comparison
    QDate from(year, month, 1);
    QDate to(year, month, QDate(year, month, 1).daysInMonth());
    auto summary = m_database->getFinanceSummary(from, to);

    double totalBudget = 0;
    double totalActual = 0;

    m_budgetTable->setRowCount(budgets.size());

    for (int row = 0; row < budgets.size(); ++row) {
        const auto &budget = budgets[row];
        totalBudget += budget.monthlyAmount;

        // Get actual for this category
        double actual = summary.expensesByCategory.value(budget.category, 0);
        totalActual += actual;
        double remaining = budget.monthlyAmount - actual;
        int percent = (budget.monthlyAmount > 0) ? static_cast<int>((actual / budget.monthlyAmount) * 100) : 0;

        // Category
        auto *catItem = new QTableWidgetItem(budget.category);
        catItem->setData(Qt::UserRole, budget.id.value_or(0));
        m_budgetTable->setItem(row, 0, catItem);

        // Budget
        auto *budgetItem = new QTableWidgetItem(formatCurrency(budget.monthlyAmount));
        budgetItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_budgetTable->setItem(row, 1, budgetItem);

        // Actual
        auto *actualItem = new QTableWidgetItem(formatCurrency(actual));
        actualItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (actual > budget.monthlyAmount) {
            actualItem->setForeground(QColor("#c62828"));
        }
        m_budgetTable->setItem(row, 2, actualItem);

        // Remaining
        auto *remainItem = new QTableWidgetItem(formatCurrency(remaining));
        remainItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (remaining < 0) {
            remainItem->setForeground(QColor("#c62828"));
        } else {
            remainItem->setForeground(QColor("#2e7d32"));
        }
        m_budgetTable->setItem(row, 3, remainItem);

        // Progress bar
        auto *progress = new QProgressBar();
        progress->setRange(0, 100);
        progress->setValue(qMin(percent, 100));
        progress->setFormat(QString("%1%").arg(percent));

        if (percent > 100) {
            progress->setStyleSheet("QProgressBar::chunk { background-color: #c62828; }");
        } else if (percent > 80) {
            progress->setStyleSheet("QProgressBar::chunk { background-color: #f57c00; }");
        } else {
            progress->setStyleSheet("QProgressBar::chunk { background-color: #2e7d32; }");
        }

        m_budgetTable->setCellWidget(row, 4, progress);
    }

    // Update summary
    double remaining = totalBudget - totalActual;
    int overallPercent = (totalBudget > 0) ? static_cast<int>((totalActual / totalBudget) * 100) : 0;

    m_totalBudgetLabel->setText(formatCurrency(totalBudget));
    m_totalActualLabel->setText(formatCurrency(totalActual));
    m_remainingLabel->setText(formatCurrency(remaining));

    if (remaining < 0) {
        m_remainingLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #c62828;");
    } else {
        m_remainingLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2e7d32;");
    }

    m_overallProgress->setValue(qMin(overallPercent, 100));
    m_overallProgress->setFormat(QString("%1% of budget used").arg(overallPercent));

    if (overallPercent > 100) {
        m_overallProgress->setStyleSheet("QProgressBar::chunk { background-color: #c62828; }");
    } else if (overallPercent > 80) {
        m_overallProgress->setStyleSheet("QProgressBar::chunk { background-color: #f57c00; }");
    } else {
        m_overallProgress->setStyleSheet("QProgressBar::chunk { background-color: #2e7d32; }");
    }

    // Update forecast
    int dayOfMonth = QDate::currentDate().day();
    int daysInMonth = QDate(year, month, 1).daysInMonth();
    double dailyRate = (dayOfMonth > 0) ? totalActual / dayOfMonth : 0;
    double projectedTotal = dailyRate * daysInMonth;

    QString forecast;
    if (year == QDate::currentDate().year() && month == QDate::currentDate().month()) {
        forecast = QString(tr("At current pace, you'll spend approximately %1 this month.\n"))
                       .arg(formatCurrency(projectedTotal));

        if (projectedTotal > totalBudget && totalBudget > 0) {
            double over = projectedTotal - totalBudget;
            forecast += QString(tr("⚠ This is %1 OVER your budget. Consider reducing spending.")).arg(formatCurrency(over));
        } else if (totalBudget > 0) {
            double under = totalBudget - projectedTotal;
            forecast += QString(tr("✓ This is %1 under budget. You're on track!")).arg(formatCurrency(under));
        }
    } else {
        if (totalActual > totalBudget && totalBudget > 0) {
            forecast = QString(tr("⚠ This month was %1 over budget.")).arg(formatCurrency(totalActual - totalBudget));
        } else if (totalBudget > 0) {
            forecast = QString(tr("✓ This month was %1 under budget.")).arg(formatCurrency(totalBudget - totalActual));
        } else {
            forecast = tr("No budgets set for this period.");
        }
    }

    m_forecastLabel->setText(forecast);
}

// =============================================================================
// Slots
// =============================================================================

void BudgetsTab::onSelectionChanged()
{
    bool hasSelection = !m_budgetTable->selectedItems().isEmpty();
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
}

void BudgetsTab::showBudgetDialog(bool isEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? tr("Edit Budget") : tr("Add Budget"));
    dialog.setMinimumWidth(350);

    auto *layout = new QFormLayout(&dialog);

    // Category
    auto *categoryCombo = new QComboBox();
    categoryCombo->setEditable(true);

    // Populate with existing categories from transactions
    auto allTransactions = m_database->getAllTransactions();
    QSet<QString> categories;
    for (const auto &t : allTransactions) {
        if (!t.category.isEmpty()) {
            categories.insert(t.category);
        }
    }
    for (const auto &cat : categories) {
        categoryCombo->addItem(cat);
    }
    layout->addRow(tr("Category:"), categoryCombo);

    // Amount
    auto *amountSpin = new QDoubleSpinBox();
    amountSpin->setRange(0, 9999999);
    amountSpin->setDecimals(2);
    amountSpin->setPrefix("$");
    layout->addRow(tr("Monthly Budget:"), amountSpin);

    // Notes
    auto *notesEdit = new QLineEdit();
    layout->addRow(tr("Notes:"), notesEdit);

    // Populate if editing
    Frontier::Budget budget;
    budget.year = m_yearSpin->value();
    budget.month = m_monthCombo->currentIndex() + 1;

    if (isEdit) {
        auto selected = m_budgetTable->selectedItems();
        if (!selected.isEmpty()) {
            int row = selected.first()->row();
            int budgetId = m_budgetTable->item(row, 0)->data(Qt::UserRole).toInt();
            auto existing = m_database->getBudget(budgetId);
            if (existing.has_value()) {
                budget = existing.value();
                categoryCombo->setCurrentText(budget.category);
                amountSpin->setValue(budget.monthlyAmount);
                notesEdit->setText(budget.notes);
            }
        }
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        budget.category = categoryCombo->currentText().trimmed();
        budget.monthlyAmount = amountSpin->value();
        budget.notes = notesEdit->text();

        if (budget.category.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Category"), tr("Please enter a category name."));
            return;
        }

        bool success = false;
        if (isEdit && budget.id.has_value()) {
            success = m_database->updateBudget(budget);
        } else {
            success = m_database->addBudget(budget) > 0;
        }

        if (success) {
            loadBudgets();
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to save budget."));
        }
    }
}

void BudgetsTab::onAddBudget()
{
    showBudgetDialog(false);
}

void BudgetsTab::onEditBudget()
{
    showBudgetDialog(true);
}

void BudgetsTab::onDeleteBudget()
{
    auto selected = m_budgetTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    int budgetId = m_budgetTable->item(row, 0)->data(Qt::UserRole).toInt();

    auto result = QMessageBox::question(this, tr("Delete Budget"),
        tr("Delete this budget entry?"));

    if (result == QMessageBox::Yes) {
        if (m_database->deleteBudget(budgetId)) {
            loadBudgets();
        }
    }
}
