/**
 * @file equipmentplannertab.cpp
 * @brief Equipment Planner implementation
 */

#include "equipmentplannertab.h"
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
}

EquipmentPlannerTab::EquipmentPlannerTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void EquipmentPlannerTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // =========================================================================
    // Add Item Section
    // =========================================================================
    auto *addGroup = new QGroupBox(tr("Add Equipment"));
    auto *addLayout = new QHBoxLayout(addGroup);

    addLayout->addWidget(new QLabel(tr("Item:")));
    m_itemCombo = new QComboBox();
    m_itemCombo->setEditable(true);
    m_itemCombo->setMinimumWidth(250);
    connect(m_itemCombo, &QComboBox::currentTextChanged,
            this, &EquipmentPlannerTab::onItemSelected);
    addLayout->addWidget(m_itemCombo);

    addLayout->addWidget(new QLabel(tr("Qty:")));
    m_quantitySpin = new QSpinBox();
    m_quantitySpin->setMinimum(1);
    m_quantitySpin->setMaximum(9999);
    m_quantitySpin->setValue(1);
    connect(m_quantitySpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int) {
        onItemSelected(m_itemCombo->currentText());
    });
    addLayout->addWidget(m_quantitySpin);

    addLayout->addWidget(new QLabel(tr("Unit Price:")));
    m_unitPriceLabel = new QLabel("$0");
    m_unitPriceLabel->setMinimumWidth(80);
    addLayout->addWidget(m_unitPriceLabel);

    addLayout->addWidget(new QLabel(tr("Line Total:")));
    m_lineTotalLabel = new QLabel("$0");
    m_lineTotalLabel->setStyleSheet("font-weight: bold;");
    m_lineTotalLabel->setMinimumWidth(100);
    addLayout->addWidget(m_lineTotalLabel);

    addLayout->addStretch();

    m_addBtn = new QPushButton(tr("+ Add to Plan"));
    m_addBtn->setStyleSheet("background-color: #4caf50; color: white; padding: 5px 15px;");
    connect(m_addBtn, &QPushButton::clicked, this, &EquipmentPlannerTab::onAddClicked);
    addLayout->addWidget(m_addBtn);

    mainLayout->addWidget(addGroup);

    // =========================================================================
    // Plan Table
    // =========================================================================
    auto *planGroup = new QGroupBox(tr("Equipment Plan"));
    auto *planLayout = new QVBoxLayout(planGroup);

    m_table = new QTableWidget();
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        tr("Item"), tr("Category"), tr("Quantity"), tr("Unit Price"), tr("Total Cost")
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

    planLayout->addWidget(m_table);

    // Action buttons
    auto *actionLayout = new QHBoxLayout();

    m_removeBtn = new QPushButton(tr("Remove Selected"));
    m_removeBtn->setEnabled(false);
    connect(m_removeBtn, &QPushButton::clicked, this, &EquipmentPlannerTab::onRemoveClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, [this]() {
        m_removeBtn->setEnabled(m_table->currentRow() >= 0);
    });
    actionLayout->addWidget(m_removeBtn);

    m_clearAllBtn = new QPushButton(tr("Clear All"));
    m_clearAllBtn->setStyleSheet("color: #c62828;");
    connect(m_clearAllBtn, &QPushButton::clicked, this, &EquipmentPlannerTab::onClearAllClicked);
    actionLayout->addWidget(m_clearAllBtn);

    actionLayout->addStretch();

    // Summary
    m_itemCountLabel = new QLabel(tr("Items: 0"));
    actionLayout->addWidget(m_itemCountLabel);

    actionLayout->addWidget(new QLabel(" | "));

    m_totalCostLabel = new QLabel(tr("Total: $0"));
    m_totalCostLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ff9800;");
    actionLayout->addWidget(m_totalCostLabel);

    planLayout->addLayout(actionLayout);

    mainLayout->addWidget(planGroup, 1);
}

void EquipmentPlannerTab::refreshData()
{
    // Populate item combo
    m_itemCombo->clear();
    auto items = m_database->getAllItems();
    for (const auto &item : items) {
        // Only include purchasable items (those with buy price)
        if (item.buyPriceInternal > 0) {
            m_itemCombo->addItem(item.name, item.id.value_or(0));
        }
    }

    loadPlan();
}

void EquipmentPlannerTab::loadPlan()
{
    m_table->setRowCount(0);

    auto plan = m_database->getEquipmentPlan();

    for (const auto &item : plan) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        // Item name
        auto *nameItem = new QTableWidgetItem(item.itemName);
        nameItem->setData(Qt::UserRole, item.id.value_or(0));
        m_table->setItem(row, 0, nameItem);

        // Category
        m_table->setItem(row, 1, new QTableWidgetItem(item.category));

        // Quantity (editable spinbox)
        auto *qtySpin = new QSpinBox();
        qtySpin->setMinimum(1);
        qtySpin->setMaximum(9999);
        qtySpin->setValue(item.quantity);
        connect(qtySpin, QOverload<int>::of(&QSpinBox::valueChanged), [this, row](int newQty) {
            onQuantityChanged(row, newQty);
        });
        m_table->setCellWidget(row, 2, qtySpin);

        // Unit Price
        auto *priceItem = new QTableWidgetItem(formatCurrency(item.unitPrice));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        priceItem->setData(Qt::UserRole, item.unitPrice);
        m_table->setItem(row, 3, priceItem);

        // Total Cost
        auto *totalItem = new QTableWidgetItem(formatCurrency(item.totalCost));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setForeground(QColor("#ff9800"));
        m_table->setItem(row, 4, totalItem);
    }

    updateSummary();
}

void EquipmentPlannerTab::updateSummary()
{
    int itemCount = m_table->rowCount();
    double totalCost = 0;

    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto *priceItem = m_table->item(row, 3);
        auto *qtySpin = qobject_cast<QSpinBox*>(m_table->cellWidget(row, 2));
        if (priceItem && qtySpin) {
            double unitPrice = priceItem->data(Qt::UserRole).toDouble();
            totalCost += unitPrice * qtySpin->value();
        }
    }

    m_itemCountLabel->setText(QString(tr("Items: %1")).arg(itemCount));
    m_totalCostLabel->setText(QString(tr("Total: %1")).arg(formatCurrency(totalCost)));
}

void EquipmentPlannerTab::onItemSelected(const QString &itemName)
{
    auto item = m_database->getItemByName(itemName);
    if (item.has_value()) {
        double unitPrice = item->buyPriceInternal;
        int qty = m_quantitySpin->value();

        m_unitPriceLabel->setText(formatCurrency(unitPrice));
        m_lineTotalLabel->setText(formatCurrency(unitPrice * qty));
    } else {
        m_unitPriceLabel->setText("$0");
        m_lineTotalLabel->setText("$0");
    }
}

void EquipmentPlannerTab::onAddClicked()
{
    QString itemName = m_itemCombo->currentText();
    if (itemName.isEmpty()) {
        return;
    }

    auto item = m_database->getItemByName(itemName);
    if (!item.has_value()) {
        QMessageBox::warning(this, tr("Error"), tr("Item not found in database."));
        return;
    }

    Frontier::EquipmentPlanItem planItem;
    planItem.itemId = item->id.value_or(0);
    planItem.itemName = item->name;
    planItem.category = item->categoryMain;
    planItem.quantity = m_quantitySpin->value();
    planItem.unitPrice = item->buyPriceInternal;
    planItem.totalCost = planItem.quantity * planItem.unitPrice;

    // Check if already in plan - if so, update quantity
    auto existing = m_database->getEquipmentPlanItemByItemId(planItem.itemId);
    if (existing.has_value()) {
        existing->quantity += planItem.quantity;
        existing->totalCost = existing->quantity * existing->unitPrice;
        m_database->updateEquipmentPlanItem(*existing);
    } else {
        m_database->addEquipmentPlanItem(planItem);
    }

    loadPlan();
    m_quantitySpin->setValue(1);
    emit planChanged();
}

void EquipmentPlannerTab::onRemoveClicked()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    auto *nameItem = m_table->item(row, 0);
    if (!nameItem) return;

    int planId = nameItem->data(Qt::UserRole).toInt();
    m_database->deleteEquipmentPlanItem(planId);

    loadPlan();
    emit planChanged();
}

void EquipmentPlannerTab::onClearAllClicked()
{
    if (m_table->rowCount() == 0) return;

    auto result = QMessageBox::question(this, tr("Clear All"),
                                        tr("Are you sure you want to clear all items from the equipment plan?"),
                                        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_database->clearEquipmentPlan();
        loadPlan();
        emit planChanged();
    }
}

void EquipmentPlannerTab::onQuantityChanged(int row, int newQty)
{
    auto *nameItem = m_table->item(row, 0);
    auto *priceItem = m_table->item(row, 3);
    if (!nameItem || !priceItem) return;

    int planId = nameItem->data(Qt::UserRole).toInt();
    double unitPrice = priceItem->data(Qt::UserRole).toDouble();
    double newTotal = unitPrice * newQty;

    // Update display
    m_table->item(row, 4)->setText(formatCurrency(newTotal));

    // Update database
    auto planItem = m_database->getEquipmentPlanItem(planId);
    if (planItem.has_value()) {
        planItem->quantity = newQty;
        planItem->totalCost = newTotal;
        m_database->updateEquipmentPlanItem(*planItem);
    }

    updateSummary();
    emit planChanged();
}
