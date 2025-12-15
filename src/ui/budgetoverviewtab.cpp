/**
 * @file budgetoverviewtab.cpp
 * @brief Budget Overview implementation
 */

#include "budgetoverviewtab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QFrame>
#include <QLocale>

namespace {
QString formatCurrency(double amount)
{
    QLocale locale(QLocale::English);
    return "$" + locale.toString(static_cast<qint64>(qRound(amount)));
}

QString formatPower(double kw)
{
    QLocale locale(QLocale::English);
    return locale.toString(static_cast<qint64>(qRound(kw))) + " kW";
}
}

BudgetOverviewTab::BudgetOverviewTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void BudgetOverviewTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // =========================================================================
    // Available Funds Section
    // =========================================================================
    auto *fundsGroup = new QGroupBox(tr("Available Funds"));
    auto *fundsLayout = new QHBoxLayout(fundsGroup);

    // Company Balance
    auto *companyBox = new QFrame();
    companyBox->setFrameStyle(QFrame::StyledPanel);
    auto *companyLayout = new QVBoxLayout(companyBox);
    companyLayout->addWidget(new QLabel(tr("Company Account")));
    m_companyBalanceLabel = new QLabel("$0");
    m_companyBalanceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #1976d2;");
    companyLayout->addWidget(m_companyBalanceLabel);
    fundsLayout->addWidget(companyBox);

    // Personal Balance
    auto *personalBox = new QFrame();
    personalBox->setFrameStyle(QFrame::StyledPanel);
    auto *personalLayout = new QVBoxLayout(personalBox);
    personalLayout->addWidget(new QLabel(tr("Personal Wallet")));
    m_personalBalanceLabel = new QLabel("$0");
    m_personalBalanceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #7b1fa2;");
    personalLayout->addWidget(m_personalBalanceLabel);
    fundsLayout->addWidget(personalBox);

    // Total Available
    auto *totalBox = new QFrame();
    totalBox->setFrameStyle(QFrame::StyledPanel);
    auto *totalLayout = new QVBoxLayout(totalBox);
    totalLayout->addWidget(new QLabel(tr("Total Available")));
    m_totalAvailableLabel = new QLabel("$0");
    m_totalAvailableLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2e7d32;");
    totalLayout->addWidget(m_totalAvailableLabel);
    fundsLayout->addWidget(totalBox);

    mainLayout->addWidget(fundsGroup);

    // =========================================================================
    // Planned Costs Section
    // =========================================================================
    auto *costsGroup = new QGroupBox(tr("Planned Capital Expenditure"));
    auto *costsLayout = new QHBoxLayout(costsGroup);

    // Equipment Total
    auto *equipBox = new QFrame();
    equipBox->setFrameStyle(QFrame::StyledPanel);
    auto *equipLayout = new QVBoxLayout(equipBox);
    equipLayout->addWidget(new QLabel(tr("Equipment Total")));
    m_equipmentTotalLabel = new QLabel("$0");
    m_equipmentTotalLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ff9800;");
    equipLayout->addWidget(m_equipmentTotalLabel);
    costsLayout->addWidget(equipBox);

    // Facility Total
    auto *facilityBox = new QFrame();
    facilityBox->setFrameStyle(QFrame::StyledPanel);
    auto *facilityLayout = new QVBoxLayout(facilityBox);
    facilityLayout->addWidget(new QLabel(tr("Facility Total")));
    m_facilityTotalLabel = new QLabel("$0");
    m_facilityTotalLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ff9800;");
    facilityLayout->addWidget(m_facilityTotalLabel);
    costsLayout->addWidget(facilityBox);

    // Grand Total
    auto *grandBox = new QFrame();
    grandBox->setFrameStyle(QFrame::StyledPanel);
    auto *grandLayout = new QVBoxLayout(grandBox);
    grandLayout->addWidget(new QLabel(tr("Grand Total")));
    m_grandTotalLabel = new QLabel("$0");
    m_grandTotalLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #c62828;");
    grandLayout->addWidget(m_grandTotalLabel);
    costsLayout->addWidget(grandBox);

    mainLayout->addWidget(costsGroup);

    // =========================================================================
    // Affordability Section
    // =========================================================================
    auto *affordGroup = new QGroupBox(tr("Affordability"));
    auto *affordLayout = new QVBoxLayout(affordGroup);

    auto *remainingLayout = new QHBoxLayout();
    remainingLayout->addWidget(new QLabel(tr("Remaining After Purchase:")));
    m_remainingLabel = new QLabel("$0");
    m_remainingLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    remainingLayout->addWidget(m_remainingLabel);
    remainingLayout->addStretch();
    affordLayout->addLayout(remainingLayout);

    m_affordabilityBar = new QProgressBar();
    m_affordabilityBar->setMinimum(0);
    m_affordabilityBar->setMaximum(100);
    m_affordabilityBar->setTextVisible(true);
    m_affordabilityBar->setFixedHeight(30);
    affordLayout->addWidget(m_affordabilityBar);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    affordLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(affordGroup);

    // =========================================================================
    // Power Summary Section
    // =========================================================================
    auto *powerGroup = new QGroupBox(tr("Power Requirements (Facility Plan)"));
    auto *powerLayout = new QVBoxLayout(powerGroup);

    auto *powerStatsLayout = new QHBoxLayout();

    // Power Required
    auto *reqBox = new QFrame();
    reqBox->setFrameStyle(QFrame::StyledPanel);
    auto *reqLayout = new QVBoxLayout(reqBox);
    reqLayout->addWidget(new QLabel(tr("Power Required")));
    m_powerRequiredLabel = new QLabel("0 kW");
    m_powerRequiredLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #c62828;");
    reqLayout->addWidget(m_powerRequiredLabel);
    powerStatsLayout->addWidget(reqBox);

    // Power Generated
    auto *genBox = new QFrame();
    genBox->setFrameStyle(QFrame::StyledPanel);
    auto *genLayout = new QVBoxLayout(genBox);
    genLayout->addWidget(new QLabel(tr("Power Generated")));
    m_powerGeneratedLabel = new QLabel("0 kW");
    m_powerGeneratedLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2e7d32;");
    genLayout->addWidget(m_powerGeneratedLabel);
    powerStatsLayout->addWidget(genBox);

    // Power Balance
    auto *balBox = new QFrame();
    balBox->setFrameStyle(QFrame::StyledPanel);
    auto *balLayout = new QVBoxLayout(balBox);
    balLayout->addWidget(new QLabel(tr("Power Balance")));
    m_powerBalanceLabel = new QLabel("0 kW");
    m_powerBalanceLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    balLayout->addWidget(m_powerBalanceLabel);
    powerStatsLayout->addWidget(balBox);

    powerLayout->addLayout(powerStatsLayout);

    m_powerBar = new QProgressBar();
    m_powerBar->setMinimum(0);
    m_powerBar->setMaximum(100);
    m_powerBar->setTextVisible(true);
    m_powerBar->setFixedHeight(25);
    powerLayout->addWidget(m_powerBar);

    m_powerStatusLabel = new QLabel();
    m_powerStatusLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_powerStatusLabel->setAlignment(Qt::AlignCenter);
    powerLayout->addWidget(m_powerStatusLabel);

    mainLayout->addWidget(powerGroup);

    mainLayout->addStretch();
}

void BudgetOverviewTab::refreshData()
{
    // Get available funds from Finance
    auto balances = m_database->calculateBalances();
    double companyBalance = balances.companyBalance;
    double personalBalance = balances.personalBalance;
    double totalAvailable = balances.total();

    m_companyBalanceLabel->setText(formatCurrency(companyBalance));
    m_personalBalanceLabel->setText(formatCurrency(personalBalance));
    m_totalAvailableLabel->setText(formatCurrency(totalAvailable));

    // Get planned costs from capital_plan tables
    double equipmentTotal = 0;
    double facilityTotal = 0;
    double powerRequired = 0;
    double powerGenerated = 0;

    // Get equipment plan totals
    auto equipmentPlan = m_database->getEquipmentPlan();
    for (const auto &item : equipmentPlan) {
        equipmentTotal += item.totalCost;
    }

    // Get facility plan totals
    auto facilityPlan = m_database->getFacilityPlan();
    for (const auto &item : facilityPlan) {
        facilityTotal += item.totalCost;
        powerRequired += item.totalPowerKw;
        powerGenerated += item.totalGeneratedKw;
    }

    double grandTotal = equipmentTotal + facilityTotal;

    m_equipmentTotalLabel->setText(formatCurrency(equipmentTotal));
    m_facilityTotalLabel->setText(formatCurrency(facilityTotal));
    m_grandTotalLabel->setText(formatCurrency(grandTotal));

    // Affordability
    double remaining = totalAvailable - grandTotal;
    m_remainingLabel->setText(formatCurrency(remaining));

    if (remaining >= 0) {
        m_remainingLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2e7d32;");
    } else {
        m_remainingLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #c62828;");
    }

    // Progress bar: how much of available funds are we using?
    int usagePercent = 0;
    if (totalAvailable > 0) {
        usagePercent = qBound(0, static_cast<int>((grandTotal / totalAvailable) * 100), 100);
    }
    m_affordabilityBar->setValue(usagePercent);
    m_affordabilityBar->setFormat(QString("%1% of funds allocated").arg(usagePercent));

    // Status message
    if (grandTotal == 0) {
        m_statusLabel->setText(tr("No items in capital plan"));
        m_statusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #757575;");
        m_affordabilityBar->setStyleSheet("");
    } else if (remaining >= 0) {
        m_statusLabel->setText(tr("✓ You can afford this plan!"));
        m_statusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2e7d32;");
        m_affordabilityBar->setStyleSheet("QProgressBar::chunk { background-color: #4caf50; }");
    } else {
        m_statusLabel->setText(tr("✗ Insufficient funds - need %1 more").arg(formatCurrency(-remaining)));
        m_statusLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #c62828;");
        m_affordabilityBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");
    }

    // Power section
    m_powerRequiredLabel->setText(formatPower(powerRequired));
    m_powerGeneratedLabel->setText(formatPower(powerGenerated));

    double powerBalance = powerGenerated - powerRequired;
    m_powerBalanceLabel->setText((powerBalance >= 0 ? "+" : "") + formatPower(powerBalance));

    if (powerBalance >= 0) {
        m_powerBalanceLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2e7d32;");
    } else {
        m_powerBalanceLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #c62828;");
    }

    // Power bar: coverage percentage
    int powerPercent = 0;
    if (powerRequired > 0) {
        powerPercent = qBound(0, static_cast<int>((powerGenerated / powerRequired) * 100), 100);
    } else if (powerGenerated > 0) {
        powerPercent = 100;
    }
    m_powerBar->setValue(powerPercent);
    m_powerBar->setFormat(QString("%1% power coverage").arg(powerPercent));

    // Power status
    if (powerRequired == 0 && powerGenerated == 0) {
        m_powerStatusLabel->setText(tr("No power requirements in facility plan"));
        m_powerStatusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #757575;");
        m_powerBar->setStyleSheet("");
    } else if (powerBalance >= 0) {
        m_powerStatusLabel->setText(tr("✓ Sufficient power generation"));
        m_powerStatusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2e7d32;");
        m_powerBar->setStyleSheet("QProgressBar::chunk { background-color: #4caf50; }");
    } else {
        m_powerStatusLabel->setText(tr("✗ Need %1 more power").arg(formatPower(-powerBalance)));
        m_powerStatusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #c62828;");
        m_powerBar->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }");
    }
}
