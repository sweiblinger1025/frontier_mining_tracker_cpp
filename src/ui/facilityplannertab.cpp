/**
 * @file facilityplannertab.cpp
 * @brief Facility Planner implementation
 */

#include "facilityplannertab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QLocale>

namespace {
QString formatCurrency(double amount)
{
    QLocale locale(QLocale::English);
    return "$" + locale.toString(static_cast<qint64>(qRound(amount)));
}

QString formatPower(double kw)
{
    if (kw == 0) return "-";
    QLocale locale(QLocale::English);
    return locale.toString(static_cast<qint64>(qRound(kw))) + " kW";
}
}

FacilityPlannerTab::FacilityPlannerTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void FacilityPlannerTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // =========================================================================
    // Add Building Section
    // =========================================================================
    auto *addGroup = new QGroupBox(tr("Add Building/Equipment"));
    auto *addLayout = new QHBoxLayout(addGroup);

    addLayout->addWidget(new QLabel(tr("Category:")));
    m_categoryFilterCombo = new QComboBox();
    m_categoryFilterCombo->addItem(tr("All Categories"), "");
    m_categoryFilterCombo->addItem(tr("Conveyors"), "Factory - Conveyors");
    m_categoryFilterCombo->addItem(tr("Pipeline"), "Factory - Pipeline");
    m_categoryFilterCombo->addItem(tr("Power"), "Factory - Power");
    m_categoryFilterCombo->addItem(tr("Production"), "Factory - Production");
    connect(m_categoryFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FacilityPlannerTab::onCategoryFilterChanged);
    addLayout->addWidget(m_categoryFilterCombo);

    addLayout->addWidget(new QLabel(tr("Building:")));
    m_buildingCombo = new QComboBox();
    m_buildingCombo->setEditable(true);
    m_buildingCombo->setMinimumWidth(200);
    connect(m_buildingCombo, &QComboBox::currentTextChanged,
            this, &FacilityPlannerTab::onBuildingSelected);
    addLayout->addWidget(m_buildingCombo);

    addLayout->addWidget(new QLabel(tr("Qty:")));
    m_quantitySpin = new QSpinBox();
    m_quantitySpin->setMinimum(1);
    m_quantitySpin->setMaximum(999);
    m_quantitySpin->setValue(1);
    connect(m_quantitySpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int) {
        onBuildingSelected(m_buildingCombo->currentText());
    });
    addLayout->addWidget(m_quantitySpin);

    m_unitPriceLabel = new QLabel("$0");
    m_unitPriceLabel->setMinimumWidth(70);
    addLayout->addWidget(m_unitPriceLabel);

    m_powerLabel = new QLabel("0 kW");
    m_powerLabel->setMinimumWidth(70);
    addLayout->addWidget(m_powerLabel);

    addLayout->addWidget(new QLabel(tr("Total:")));
    m_lineTotalLabel = new QLabel("$0");
    m_lineTotalLabel->setStyleSheet("font-weight: bold;");
    m_lineTotalLabel->setMinimumWidth(80);
    addLayout->addWidget(m_lineTotalLabel);

    addLayout->addStretch();

    m_addBtn = new QPushButton(tr("+ Add to Plan"));
    m_addBtn->setStyleSheet("background-color: #4caf50; color: white; padding: 5px 15px;");
    connect(m_addBtn, &QPushButton::clicked, this, &FacilityPlannerTab::onAddClicked);
    addLayout->addWidget(m_addBtn);

    mainLayout->addWidget(addGroup);

    // =========================================================================
    // Plan Table
    // =========================================================================
    auto *planGroup = new QGroupBox(tr("Facility Plan"));
    auto *planLayout = new QVBoxLayout(planGroup);

    m_table = new QTableWidget();
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        tr("Building"), tr("Category"), tr("Qty"),
        tr("Power (kW)"), tr("Generated (kW)"), tr("Unit Price"), tr("Total Cost")
    });
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);

    auto *header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    planLayout->addWidget(m_table);

    // Action buttons and summary
    auto *actionLayout = new QHBoxLayout();

    m_removeBtn = new QPushButton(tr("Remove Selected"));
    m_removeBtn->setEnabled(false);
    connect(m_removeBtn, &QPushButton::clicked, this, &FacilityPlannerTab::onRemoveClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, [this]() {
        m_removeBtn->setEnabled(m_table->currentRow() >= 0);
    });
    actionLayout->addWidget(m_removeBtn);

    m_clearAllBtn = new QPushButton(tr("Clear All"));
    m_clearAllBtn->setStyleSheet("color: #c62828;");
    connect(m_clearAllBtn, &QPushButton::clicked, this, &FacilityPlannerTab::onClearAllClicked);
    actionLayout->addWidget(m_clearAllBtn);

    actionLayout->addStretch();

    // Summary labels
    m_itemCountLabel = new QLabel(tr("Items: 0"));
    actionLayout->addWidget(m_itemCountLabel);

    actionLayout->addWidget(new QLabel(" | "));

    m_totalPowerLabel = new QLabel(tr("Power: 0 kW"));
    m_totalPowerLabel->setStyleSheet("color: #c62828;");
    actionLayout->addWidget(m_totalPowerLabel);

    actionLayout->addWidget(new QLabel(" | "));

    m_totalGeneratedLabel = new QLabel(tr("Generated: 0 kW"));
    m_totalGeneratedLabel->setStyleSheet("color: #2e7d32;");
    actionLayout->addWidget(m_totalGeneratedLabel);

    actionLayout->addWidget(new QLabel(" | "));

    m_powerBalanceLabel = new QLabel(tr("Balance: 0 kW"));
    m_powerBalanceLabel->setStyleSheet("font-weight: bold;");
    actionLayout->addWidget(m_powerBalanceLabel);

    actionLayout->addWidget(new QLabel(" | "));

    m_totalCostLabel = new QLabel(tr("Total: $0"));
    m_totalCostLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ff9800;");
    actionLayout->addWidget(m_totalCostLabel);

    planLayout->addLayout(actionLayout);

    mainLayout->addWidget(planGroup, 1);
}

void FacilityPlannerTab::refreshData()
{
    populateBuildingCombo();
    loadPlan();
}

void FacilityPlannerTab::populateBuildingCombo()
{
    m_buildingCombo->clear();

    QString categoryFilter = m_categoryFilterCombo->currentData().toString();

    QVector<Frontier::FactoryBuilding> buildings;
    if (categoryFilter.isEmpty()) {
        buildings = m_database->getAllFactoryBuildings();
    } else {
        buildings = m_database->getFactoryBuildingsByCategory(categoryFilter);
    }

    for (const auto &b : buildings) {
        m_buildingCombo->addItem(b.name, b.id.value_or(0));
    }
}

void FacilityPlannerTab::loadPlan()
{
    m_table->setRowCount(0);

    auto plan = m_database->getFacilityPlan();

    for (const auto &item : plan) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        // Building name
        auto *nameItem = new QTableWidgetItem(item.buildingName);
        nameItem->setData(Qt::UserRole, item.id.value_or(0));
        m_table->setItem(row, 0, nameItem);

        // Category (shortened)
        QString shortCat = item.category;
        shortCat.replace("Factory - ", "");
        m_table->setItem(row, 1, new QTableWidgetItem(shortCat));

        // Quantity (editable spinbox)
        auto *qtySpin = new QSpinBox();
        qtySpin->setMinimum(1);
        qtySpin->setMaximum(999);
        qtySpin->setValue(item.quantity);
        connect(qtySpin, QOverload<int>::of(&QSpinBox::valueChanged), [this, row](int newQty) {
            onQuantityChanged(row, newQty);
        });
        m_table->setCellWidget(row, 2, qtySpin);

        // Power (kW)
        auto *powerItem = new QTableWidgetItem(formatPower(item.totalPowerKw));
        powerItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        powerItem->setData(Qt::UserRole, item.unitPowerKw);
        if (item.totalPowerKw > 0) {
            powerItem->setForeground(QColor("#c62828"));
        }
        m_table->setItem(row, 3, powerItem);

        // Generated (kW)
        auto *genItem = new QTableWidgetItem(formatPower(item.totalGeneratedKw));
        genItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        genItem->setData(Qt::UserRole, item.unitGeneratedKw);
        if (item.totalGeneratedKw > 0) {
            genItem->setForeground(QColor("#2e7d32"));
        }
        m_table->setItem(row, 4, genItem);

        // Unit Price
        auto *priceItem = new QTableWidgetItem(formatCurrency(item.unitPrice));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        priceItem->setData(Qt::UserRole, item.unitPrice);
        m_table->setItem(row, 5, priceItem);

        // Total Cost
        auto *totalItem = new QTableWidgetItem(formatCurrency(item.totalCost));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setForeground(QColor("#ff9800"));
        m_table->setItem(row, 6, totalItem);
    }

    updateSummary();
}

void FacilityPlannerTab::updateSummary()
{
    int itemCount = m_table->rowCount();
    double totalCost = 0;
    double totalPower = 0;
    double totalGenerated = 0;

    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto *priceItem = m_table->item(row, 5);
        auto *powerItem = m_table->item(row, 3);
        auto *genItem = m_table->item(row, 4);
        auto *qtySpin = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 2));

        if (priceItem && qtySpin) {
            int qty = qtySpin->value();
            double unitPrice = priceItem->data(Qt::UserRole).toDouble();
            double unitPower = powerItem->data(Qt::UserRole).toDouble();
            double unitGen = genItem->data(Qt::UserRole).toDouble();

            totalCost += unitPrice * qty;
            totalPower += unitPower * qty;
            totalGenerated += unitGen * qty;
        }
    }

    double powerBalance = totalGenerated - totalPower;

    m_itemCountLabel->setText(QString(tr("Items: %1")).arg(itemCount));
    m_totalCostLabel->setText(QString(tr("Total: %1")).arg(formatCurrency(totalCost)));
    m_totalPowerLabel->setText(QString(tr("Power: %1")).arg(formatPower(totalPower)));
    m_totalGeneratedLabel->setText(QString(tr("Generated: %1")).arg(formatPower(totalGenerated)));

    QString balanceStr = (powerBalance >= 0 ? "+" : "") + formatPower(powerBalance);
    m_powerBalanceLabel->setText(QString(tr("Balance: %1")).arg(balanceStr));

    if (powerBalance >= 0) {
        m_powerBalanceLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");
    } else {
        m_powerBalanceLabel->setStyleSheet("font-weight: bold; color: #c62828;");
    }
}

void FacilityPlannerTab::onCategoryFilterChanged()
{
    populateBuildingCombo();
}

void FacilityPlannerTab::onBuildingSelected(const QString &buildingName)
{
    auto building = m_database->getFactoryBuildingByName(buildingName);
    if (building.has_value()) {
        int qty = m_quantitySpin->value();

        m_unitPriceLabel->setText(formatCurrency(building->price));

        // Show power info
        if (building->generatedKw > 0) {
            m_powerLabel->setText("+" + formatPower(building->generatedKw * qty));
            m_powerLabel->setStyleSheet("color: #2e7d32;");
        } else if (building->powerKw > 0) {
            m_powerLabel->setText("-" + formatPower(building->powerKw * qty));
            m_powerLabel->setStyleSheet("color: #c62828;");
        } else {
            m_powerLabel->setText("-");
            m_powerLabel->setStyleSheet("");
        }

        m_lineTotalLabel->setText(formatCurrency(building->price * qty));
    } else {
        m_unitPriceLabel->setText("$0");
        m_powerLabel->setText("-");
        m_lineTotalLabel->setText("$0");
    }
}

void FacilityPlannerTab::onAddClicked()
{
    QString buildingName = m_buildingCombo->currentText();
    if (buildingName.isEmpty()) {
        return;
    }

    auto building = m_database->getFactoryBuildingByName(buildingName);
    if (!building.has_value()) {
        QMessageBox::warning(this, tr("Error"), tr("Building not found in database."));
        return;
    }

    Frontier::FacilityPlanItem planItem;
    planItem.buildingId = building->id.value_or(0);
    planItem.buildingName = building->name;
    planItem.category = building->category;
    planItem.quantity = m_quantitySpin->value();
    planItem.unitPrice = building->price;
    planItem.unitPowerKw = building->powerKw;
    planItem.unitGeneratedKw = building->generatedKw;
    planItem.totalCost = planItem.quantity * planItem.unitPrice;
    planItem.totalPowerKw = planItem.quantity * planItem.unitPowerKw;
    planItem.totalGeneratedKw = planItem.quantity * planItem.unitGeneratedKw;

    // Check if already in plan - if so, update quantity
    auto existing = m_database->getFacilityPlanItemByBuildingId(planItem.buildingId);
    if (existing.has_value()) {
        existing->quantity += planItem.quantity;
        existing->totalCost = existing->quantity * existing->unitPrice;
        existing->totalPowerKw = existing->quantity * existing->unitPowerKw;
        existing->totalGeneratedKw = existing->quantity * existing->unitGeneratedKw;
        m_database->updateFacilityPlanItem(*existing);
    } else {
        m_database->addFacilityPlanItem(planItem);
    }

    loadPlan();
    m_quantitySpin->setValue(1);
    emit planChanged();
}

void FacilityPlannerTab::onRemoveClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    auto *nameItem = m_table->item(row, 0);
    if (!nameItem) return;

    int planId = nameItem->data(Qt::UserRole).toInt();
    m_database->deleteFacilityPlanItem(planId);

    loadPlan();
    emit planChanged();
}

void FacilityPlannerTab::onClearAllClicked()
{
    if (m_table->rowCount() == 0) return;

    auto result = QMessageBox::question(this, tr("Clear All"),
                                        tr("Are you sure you want to clear all items from the facility plan?"),
                                        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_database->clearFacilityPlan();
        loadPlan();
        emit planChanged();
    }
}

void FacilityPlannerTab::onQuantityChanged(int row, int newQty)
{
    auto *nameItem = m_table->item(row, 0);
    auto *priceItem = m_table->item(row, 5);
    auto *powerItem = m_table->item(row, 3);
    auto *genItem = m_table->item(row, 4);

    if (!nameItem || !priceItem || !powerItem || !genItem) return;

    int planId = nameItem->data(Qt::UserRole).toInt();
    double unitPrice = priceItem->data(Qt::UserRole).toDouble();
    double unitPower = powerItem->data(Qt::UserRole).toDouble();
    double unitGen = genItem->data(Qt::UserRole).toDouble();

    double newTotal = unitPrice * newQty;
    double newPower = unitPower * newQty;
    double newGen = unitGen * newQty;

    // Update display
    m_table->item(row, 3)->setText(formatPower(newPower));
    m_table->item(row, 4)->setText(formatPower(newGen));
    m_table->item(row, 6)->setText(formatCurrency(newTotal));

    // Update database
    auto planItem = m_database->getFacilityPlanItem(planId);
    if (planItem.has_value()) {
        planItem->quantity = newQty;
        planItem->totalCost = newTotal;
        planItem->totalPowerKw = newPower;
        planItem->totalGeneratedKw = newGen;
        m_database->updateFacilityPlanItem(*planItem);
    }

    updateSummary();
    emit planChanged();
}
