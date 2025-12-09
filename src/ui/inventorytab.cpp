/**
 * @file inventorytab.cpp
 * @brief Inventory management tab implementation
 */

#include "inventorytab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSet>

// =============================================================================
// Constructor & Setup
// =============================================================================

InventoryTab::InventoryTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void InventoryTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Summary panel
    mainLayout->addWidget(createSummaryPanel());

    // Filter panel
    mainLayout->addWidget(createFilterPanel());

    // Main content: splitter with table and details
    auto *splitter = new QSplitter(Qt::Horizontal);

    // Left: Table and buttons
    auto *tableWidget = new QWidget();
    auto *tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget();
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        tr("Item Name"), tr("Category"), tr("Location"),
        tr("Quantity"), tr("Unit Price"), tr("Total Value"), tr("Status")
    });
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);

    auto *header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i < 7; ++i) {
        header->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &InventoryTab::onSelectionChanged);

    tableLayout->addWidget(m_table);

    // Buttons
    auto *btnLayout = new QHBoxLayout();

    m_addBtn = new QPushButton(tr("+ Add Item"));
    connect(m_addBtn, &QPushButton::clicked, this, &InventoryTab::onAddItem);
    btnLayout->addWidget(m_addBtn);

    m_editBtn = new QPushButton(tr("Edit"));
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &InventoryTab::onEditItem);
    btnLayout->addWidget(m_editBtn);

    m_adjustBtn = new QPushButton(tr("Adjust Qty"));
    m_adjustBtn->setEnabled(false);
    connect(m_adjustBtn, &QPushButton::clicked, this, &InventoryTab::onAdjustQuantity);
    btnLayout->addWidget(m_adjustBtn);

    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &InventoryTab::onDeleteItem);
    btnLayout->addWidget(m_deleteBtn);

    btnLayout->addStretch();

    m_syncBtn = new QPushButton(tr("Sync from Ledger"));
    m_syncBtn->setToolTip(tr("Scan Ledger transactions and update inventory quantities"));
    m_syncBtn->setStyleSheet("background-color: #e3f2fd;");
    connect(m_syncBtn, &QPushButton::clicked, this, &InventoryTab::onSyncFromLedger);
    btnLayout->addWidget(m_syncBtn);

    tableLayout->addLayout(btnLayout);
    splitter->addWidget(tableWidget);

    // Right: Details panel
    splitter->addWidget(createDetailsPanel());
    splitter->setSizes({600, 400});

    mainLayout->addWidget(splitter, 1);

    // Oil tracker at bottom
    mainLayout->addWidget(createOilTracker());
}

QWidget* InventoryTab::createSummaryPanel()
{
    auto *group = new QGroupBox(tr("Inventory Summary"));
    auto *layout = new QHBoxLayout(group);

    // Total Value
    auto *valueLayout = new QVBoxLayout();
    valueLayout->addWidget(new QLabel(tr("Total Value")));
    m_totalValueLabel = new QLabel("$0");
    m_totalValueLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2e7d32;");
    valueLayout->addWidget(m_totalValueLabel);
    layout->addLayout(valueLayout);

    layout->addWidget(createSeparator());

    // Items with Stock
    auto *itemsLayout = new QVBoxLayout();
    itemsLayout->addWidget(new QLabel(tr("Items with Stock")));
    m_itemsCountLabel = new QLabel("0");
    m_itemsCountLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    itemsLayout->addWidget(m_itemsCountLabel);
    layout->addLayout(itemsLayout);

    layout->addWidget(createSeparator());

    // Active Locations
    auto *locLayout = new QVBoxLayout();
    locLayout->addWidget(new QLabel(tr("Active Locations")));
    m_locationsCountLabel = new QLabel("0");
    m_locationsCountLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    locLayout->addWidget(m_locationsCountLabel);
    layout->addLayout(locLayout);

    layout->addWidget(createSeparator());

    // Low Stock
    auto *alertLayout = new QVBoxLayout();
    alertLayout->addWidget(new QLabel(tr("Low Stock")));
    m_lowStockLabel = new QLabel("0");
    m_lowStockLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f57c00;");
    alertLayout->addWidget(m_lowStockLabel);
    layout->addLayout(alertLayout);

    layout->addWidget(createSeparator());

    // Oil Cap Status
    auto *oilLayout = new QVBoxLayout();
    oilLayout->addWidget(new QLabel(tr("Oil Cap")));
    m_oilStatusLabel = new QLabel("--");
    m_oilStatusLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    oilLayout->addWidget(m_oilStatusLabel);
    layout->addLayout(oilLayout);

    layout->addStretch();

    return group;
}

QFrame* InventoryTab::createSeparator()
{
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    return sep;
}

QWidget* InventoryTab::createFilterPanel()
{
    auto *group = new QGroupBox(tr("Filters"));
    auto *layout = new QHBoxLayout(group);

    // Search
    layout->addWidget(new QLabel(tr("Search:")));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Item name..."));
    m_searchEdit->setMaximumWidth(150);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &InventoryTab::onFilterChanged);
    layout->addWidget(m_searchEdit);

    // Category
    layout->addWidget(new QLabel(tr("Category:")));
    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItem(tr("All Categories"));
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InventoryTab::onFilterChanged);
    layout->addWidget(m_categoryCombo);

    // Location
    layout->addWidget(new QLabel(tr("Location:")));
    m_locationCombo = new QComboBox();
    m_locationCombo->addItem(tr("All Locations"));
    connect(m_locationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InventoryTab::onFilterChanged);
    layout->addWidget(m_locationCombo);

    // Status
    layout->addWidget(new QLabel(tr("Status:")));
    m_statusCombo = new QComboBox();
    m_statusCombo->addItems({tr("All"), tr("Empty"), tr("Low"), tr("Good"), tr("High")});
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InventoryTab::onFilterChanged);
    layout->addWidget(m_statusCombo);

    // Show Zero Stock
    m_showZeroCheck = new QCheckBox(tr("Show Zero Stock"));
    m_showZeroCheck->setChecked(true);
    connect(m_showZeroCheck, &QCheckBox::stateChanged, this, &InventoryTab::onFilterChanged);
    layout->addWidget(m_showZeroCheck);

    layout->addStretch();

    // Refresh button
    auto *refreshBtn = new QPushButton(tr("Refresh"));
    connect(refreshBtn, &QPushButton::clicked, this, &InventoryTab::refreshData);
    layout->addWidget(refreshBtn);

    return group;
}

QWidget* InventoryTab::createDetailsPanel()
{
    auto *group = new QGroupBox(tr("Item Details"));
    auto *layout = new QVBoxLayout(group);

    m_detailsText = new QTextEdit();
    m_detailsText->setReadOnly(true);
    m_detailsText->setHtml("<p style='color: #666;'>Select an item to view details</p>");
    layout->addWidget(m_detailsText);

    return group;
}

QWidget* InventoryTab::createOilTracker()
{
    auto *group = new QGroupBox(tr("Oil Lifetime Cap Tracker"));
    auto *layout = new QVBoxLayout(group);

    // Warning label
    m_oilWarningLabel = new QLabel(tr("Oil cap in good standing"));
    m_oilWarningLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
    layout->addWidget(m_oilWarningLabel);

    // Stats row
    auto *statsLayout = new QHBoxLayout();

    // Current inventory
    auto *invLayout = new QFormLayout();
    m_oilInventoryLabel = new QLabel("0");
    m_oilInventoryLabel->setStyleSheet("font-weight: bold;");
    invLayout->addRow(tr("Current Inventory:"), m_oilInventoryLabel);
    statsLayout->addLayout(invLayout);

    // Total sold
    auto *soldLayout = new QFormLayout();
    m_oilSoldLabel = new QLabel("0");
    m_oilSoldLabel->setStyleSheet("font-weight: bold;");
    soldLayout->addRow(tr("Total Sold (Lifetime):"), m_oilSoldLabel);
    statsLayout->addLayout(soldLayout);

    // Remaining
    auto *remainingLayout = new QFormLayout();
    m_oilRemainingLabel = new QLabel("0");
    m_oilRemainingLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");
    remainingLayout->addRow(tr("Remaining Cap:"), m_oilRemainingLabel);
    statsLayout->addLayout(remainingLayout);

    // Cap setting
    auto *capLayout = new QFormLayout();
    m_oilCapSpin = new QSpinBox();
    m_oilCapSpin->setRange(0, 9999999);
    m_oilCapSpin->setValue(10000);
    connect(m_oilCapSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &InventoryTab::onOilCapChanged);
    capLayout->addRow(tr("Cap Limit:"), m_oilCapSpin);
    statsLayout->addLayout(capLayout);

    statsLayout->addStretch();
    layout->addLayout(statsLayout);

    // Progress bar
    auto *progressLayout = new QHBoxLayout();
    progressLayout->addWidget(new QLabel(tr("Progress:")));
    m_oilProgress = new QProgressBar();
    m_oilProgress->setRange(0, 100);
    m_oilProgress->setValue(0);
    m_oilProgress->setFormat("%p% used");
    progressLayout->addWidget(m_oilProgress, 1);
    m_oilProgressText = new QLabel("0 / 10,000");
    progressLayout->addWidget(m_oilProgressText);
    layout->addLayout(progressLayout);

    // Revenue potential
    auto *revenueLayout = new QHBoxLayout();
    revenueLayout->addWidget(new QLabel(tr("Revenue if all remaining sold:")));
    m_oilRevenueLabel = new QLabel("$0");
    m_oilRevenueLabel->setStyleSheet("font-weight: bold;");
    revenueLayout->addWidget(m_oilRevenueLabel);
    revenueLayout->addWidget(new QLabel(tr("(Company:")));
    m_oilCompanyLabel = new QLabel("$0");
    revenueLayout->addWidget(m_oilCompanyLabel);
    revenueLayout->addWidget(new QLabel(tr("Personal:")));
    m_oilPersonalLabel = new QLabel("$0");
    revenueLayout->addWidget(m_oilPersonalLabel);
    revenueLayout->addWidget(new QLabel(")"));
    revenueLayout->addStretch();

    auto *resetBtn = new QPushButton(tr("Reset Lifetime Counter"));
    resetBtn->setToolTip(tr("Use when starting a new playthrough"));
    connect(resetBtn, &QPushButton::clicked, this, &InventoryTab::onResetOilCounter);
    revenueLayout->addWidget(resetBtn);

    layout->addLayout(revenueLayout);

    return group;
}

// =============================================================================
// Data Loading
// =============================================================================

void InventoryTab::refreshData()
{
    // Load oil tracking first
    m_oilTracking = m_database->getOilTracking();
    m_oilCapSpin->blockSignals(true);
    m_oilCapSpin->setValue(m_oilTracking.oilCap);
    m_oilCapSpin->blockSignals(false);

    // Populate filter combos
    m_categoryCombo->blockSignals(true);
    m_locationCombo->blockSignals(true);

    QString currentCategory = m_categoryCombo->currentText();
    QString currentLocation = m_locationCombo->currentText();

    m_categoryCombo->clear();
    m_categoryCombo->addItem(tr("All Categories"));

    m_locationCombo->clear();
    m_locationCombo->addItem(tr("All Locations"));

    // Get unique categories from items
    auto categories = m_database->getAllCategories();
    for (const auto &cat : categories) {
        m_categoryCombo->addItem(cat);
    }

    // Get locations
    auto locations = m_database->getAllLocations();
    for (const auto &loc : locations) {
        m_locationCombo->addItem(loc.name, loc.id.value_or(0));
    }

    // Restore selections
    int catIdx = m_categoryCombo->findText(currentCategory);
    if (catIdx >= 0) m_categoryCombo->setCurrentIndex(catIdx);
    int locIdx = m_locationCombo->findText(currentLocation);
    if (locIdx >= 0) m_locationCombo->setCurrentIndex(locIdx);

    m_categoryCombo->blockSignals(false);
    m_locationCombo->blockSignals(false);

    loadInventory();
}

void InventoryTab::loadInventory()
{
    m_allItems = m_database->getAllInventory();
    applyFilters();
    updateSummary();
    updateOilTracker();
}

void InventoryTab::applyFilters()
{
    QString searchText = m_searchEdit->text().toLower();
    QString categoryFilter = m_categoryCombo->currentText();
    QString locationFilter = m_locationCombo->currentText();
    QString statusFilter = m_statusCombo->currentText();
    bool showZero = m_showZeroCheck->isChecked();

    m_filteredItems.clear();

    for (const auto &item : m_allItems) {
        // Search filter
        if (!searchText.isEmpty() && !item.itemName.toLower().contains(searchText)) {
            continue;
        }

        // Category filter
        if (categoryFilter != tr("All Categories") && item.category != categoryFilter) {
            continue;
        }

        // Location filter
        if (locationFilter != tr("All Locations") && item.locationName != locationFilter) {
            continue;
        }

        // Status filter
        QString status = getStockStatus(item.quantity);
        if (statusFilter != tr("All") && status != statusFilter) {
            continue;
        }

        // Zero stock filter
        if (!showZero && item.quantity == 0) {
            continue;
        }

        m_filteredItems.append(item);
    }

    populateTable();
}

void InventoryTab::populateTable()
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(m_filteredItems.size());

    for (int row = 0; row < m_filteredItems.size(); ++row) {
        const auto &item = m_filteredItems[row];

        // Item Name
        auto *nameItem = new QTableWidgetItem(item.itemName);
        nameItem->setData(Qt::UserRole, item.id.value_or(0));
        m_table->setItem(row, 0, nameItem);

        // Category
        m_table->setItem(row, 1, new QTableWidgetItem(item.category));

        // Location
        m_table->setItem(row, 2, new QTableWidgetItem(item.locationName));

        // Quantity
        auto *qtyItem = new QTableWidgetItem(QString("%L1").arg(item.quantity));
        qtyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        qtyItem->setData(Qt::UserRole, item.quantity);
        m_table->setItem(row, 3, qtyItem);

        // Unit Price
        auto *priceItem = new QTableWidgetItem(QString("$%L1").arg(item.unitPrice, 0, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 4, priceItem);

        // Total Value
        auto *totalItem = new QTableWidgetItem(QString("$%L1").arg(item.totalValue(), 0, 'f', 0));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setData(Qt::UserRole, item.totalValue());
        m_table->setItem(row, 5, totalItem);

        // Status
        QString status = getStockStatus(item.quantity);
        auto *statusItem = new QTableWidgetItem(status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 6, statusItem);

        // Row color
        QColor color = getStatusColor(status);
        for (int col = 0; col < 7; ++col) {
            if (auto *tableItem = m_table->item(row, col)) {
                tableItem->setBackground(color);
            }
        }
    }

    m_table->setSortingEnabled(true);
}

QString InventoryTab::getStockStatus(int quantity) const
{
    if (quantity == 0) return tr("Empty");
    if (quantity <= STOCK_LOW) return tr("Low");
    if (quantity <= STOCK_GOOD) return tr("Good");
    return tr("High");
}

QColor InventoryTab::getStatusColor(const QString &status) const
{
    if (status == tr("Empty")) return QColor(255, 205, 210);  // Light red
    if (status == tr("Low")) return QColor(255, 249, 196);    // Light yellow
    if (status == tr("Good")) return QColor(200, 230, 201);   // Light green
    if (status == tr("High")) return QColor(187, 222, 251);   // Light blue
    return Qt::white;
}

void InventoryTab::updateSummary()
{
    double totalValue = 0;
    int itemsWithStock = 0;
    QSet<QString> activeLocations;
    int lowStockCount = 0;

    for (const auto &item : m_allItems) {
        totalValue += item.totalValue();

        if (item.quantity > 0) {
            itemsWithStock++;
            if (!item.locationName.isEmpty()) {
                activeLocations.insert(item.locationName);
            }
        }

        if (item.quantity > 0 && item.quantity <= STOCK_LOW) {
            lowStockCount++;
        }
    }

    m_totalValueLabel->setText(QString("$%L1").arg(totalValue, 0, 'f', 0));
    m_itemsCountLabel->setText(QString::number(itemsWithStock));
    m_locationsCountLabel->setText(QString::number(activeLocations.size()));
    m_lowStockLabel->setText(QString::number(lowStockCount));

    // Oil quick status
    int remaining = m_oilTracking.remaining();
    double pct = m_oilTracking.oilCap > 0 ?
                     (remaining * 100.0 / m_oilTracking.oilCap) : 0;
    m_oilStatusLabel->setText(QString("%L1 left (%2%)").arg(remaining).arg(pct, 0, 'f', 0));
}

void InventoryTab::updateOilTracker()
{
    // Find oil inventory
    int oilInventory = 0;
    for (const auto &item : m_allItems) {
        if (item.itemName == "Oil") {
            oilInventory = item.quantity;
            break;
        }
    }

    int remaining = m_oilTracking.remaining();
    double pctUsed = m_oilTracking.percentUsed();

    m_oilInventoryLabel->setText(QString("%L1").arg(oilInventory));
    m_oilSoldLabel->setText(QString("%L1").arg(m_oilTracking.totalOilSold));
    m_oilRemainingLabel->setText(QString("%L1").arg(remaining));

    m_oilProgress->setValue(static_cast<int>(pctUsed));
    m_oilProgressText->setText(QString("%L1 / %L2")
                                   .arg(m_oilTracking.totalOilSold)
                                   .arg(m_oilTracking.oilCap));

    // Get oil price from items
    double oilPrice = 67.00;  // Default
    auto oilItem = m_database->getItemByName("Oil");
    if (oilItem.has_value()) {
        oilPrice = oilItem->sellPriceDisplay;
    }

    double revenue = remaining * oilPrice;
    double company = revenue * COMPANY_SPLIT;
    double personal = revenue * PERSONAL_SPLIT;

    m_oilRevenueLabel->setText(QString("$%L1").arg(revenue, 0, 'f', 0));
    m_oilCompanyLabel->setText(QString("$%L1").arg(company, 0, 'f', 0));
    m_oilPersonalLabel->setText(QString("$%L1").arg(personal, 0, 'f', 0));

    // Update warning
    if (pctUsed >= 90) {
        m_oilWarningLabel->setStyleSheet("color: #c62828; font-weight: bold;");
        m_oilWarningLabel->setText(tr("CRITICAL: Less than 10% cap remaining!"));
    } else if (pctUsed >= 75) {
        m_oilWarningLabel->setStyleSheet("color: #f57c00; font-weight: bold;");
        m_oilWarningLabel->setText(tr("WARNING: Less than 25% cap remaining"));
    } else {
        m_oilWarningLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
        m_oilWarningLabel->setText(tr("Oil cap in good standing"));
    }
}

void InventoryTab::updateDetails()
{
    auto selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        m_detailsText->setHtml("<p style='color: #666;'>Select an item to view details</p>");
        return;
    }

    int row = selected.first()->row();
    if (row < 0 || row >= m_filteredItems.size()) {
        return;
    }

    const auto &item = m_filteredItems[row];
    QString status = getStockStatus(item.quantity);

    double gross = item.totalValue();
    double company = gross * COMPANY_SPLIT;
    double personal = gross * PERSONAL_SPLIT;

    QString html = QString(R"(
        <style>
            body { font-family: sans-serif; font-size: 12px; }
            h2 { margin: 5px 0; color: #1976d2; }
            h3 { margin: 10px 0 5px 0; color: #388e3c; }
            .label { color: #666; }
            .value { font-weight: bold; }
            .section { margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 5px; }
        </style>

        <h2>%1</h2>
        <p><span class="label">Category:</span> <span class="value">%2</span></p>
        <p><span class="label">Location:</span> <span class="value">%3</span></p>
        <p><span class="label">Status:</span> <span class="value">%4</span></p>

        <div class="section">
            <h3>Quantities</h3>
            <p><span class="label">Current Stock:</span> <span class="value">%L5</span></p>
        </div>

        <div class="section">
            <h3>Values</h3>
            <p><span class="label">Unit Price:</span> <span class="value">$%6</span></p>
            <p><span class="label">Total Value:</span> <span class="value">$%L7</span></p>
        </div>

        <div class="section">
            <h3>Revenue Potential (if sold)</h3>
            <p><span class="label">Gross:</span> <span class="value">$%L8</span></p>
            <p><span class="label">Company (90%):</span> <span class="value">$%L9</span></p>
            <p><span class="label">Personal (10%):</span> <span class="value">$%L10</span></p>
        </div>
    )")
                       .arg(item.itemName)
                       .arg(item.category)
                       .arg(item.locationName.isEmpty() ? tr("Not set") : item.locationName)
                       .arg(status)
                       .arg(item.quantity)
                       .arg(item.unitPrice, 0, 'f', 2)
                       .arg(item.totalValue(), 0, 'f', 0)
                       .arg(gross, 0, 'f', 0)
                       .arg(company, 0, 'f', 0)
                       .arg(personal, 0, 'f', 0);

    // Special info for Oil
    if (item.itemName == "Oil") {
        int remaining = m_oilTracking.remaining();
        html += QString(R"(
            <div class="section" style="background: #fff3e0;">
                <h3>Lifetime Cap Info</h3>
                <p><span class="label">Lifetime Cap:</span> <span class="value">%L1</span></p>
                <p><span class="label">Total Sold:</span> <span class="value">%L2</span></p>
                <p><span class="label">Remaining:</span> <span class="value">%L3</span></p>
            </div>
        )")
                    .arg(m_oilTracking.oilCap)
                    .arg(m_oilTracking.totalOilSold)
                    .arg(remaining);
    }

    m_detailsText->setHtml(html);
}

// =============================================================================
// Slots
// =============================================================================

void InventoryTab::onFilterChanged()
{
    applyFilters();
}

void InventoryTab::onSelectionChanged()
{
    bool hasSelection = !m_table->selectedItems().isEmpty();
    m_editBtn->setEnabled(hasSelection);
    m_adjustBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    updateDetails();
}

void InventoryTab::onOilCapChanged(int value)
{
    m_oilTracking.oilCap = value;
    m_database->saveOilTracking(m_oilTracking);
    updateSummary();
    updateOilTracker();
    emit dataChanged();
}

void InventoryTab::onResetOilCounter()
{
    auto result = QMessageBox::question(this, tr("Reset Oil Counter"),
                                        tr("Reset the oil lifetime sold counter to 0?\n\nUse this when starting a new playthrough."));

    if (result == QMessageBox::Yes) {
        m_database->resetOilTracking();
        m_oilTracking = m_database->getOilTracking();
        updateSummary();
        updateOilTracker();
        emit dataChanged();
        QMessageBox::information(this, tr("Reset"), tr("Oil lifetime counter has been reset."));
    }
}

void InventoryTab::onAddItem()
{
    // Get all items for selection
    auto items = m_database->getAllItems();
    if (items.isEmpty()) {
        QMessageBox::warning(this, tr("No Items"),
                             tr("Please import items first (File → Import → Items)."));
        return;
    }

    // Build item name list
    QStringList itemNames;
    for (const auto &item : items) {
        itemNames << item.name;
    }

    bool ok;
    QString selectedName = QInputDialog::getItem(this, tr("Add Inventory Item"),
                                                 tr("Select Item:"), itemNames, 0, false, &ok);
    if (!ok) return;

    // Find the item
    const Frontier::Item* selectedItem = nullptr;
    for (const auto &item : items) {
        if (item.name == selectedName) {
            selectedItem = &item;
            break;
        }
    }
    if (!selectedItem) return;

    // Check if already in inventory
    auto existing = m_database->getInventoryByItemId(selectedItem->id.value_or(0));
    if (existing.has_value()) {
        QMessageBox::information(this, tr("Already Exists"),
                                 tr("'%1' is already in inventory. Use Adjust Qty to change the quantity.")
                                     .arg(selectedName));
        return;
    }

    // Get quantity
    int quantity = QInputDialog::getInt(this, tr("Add Inventory Item"),
                                        tr("Quantity:"), 0, 0, 9999999, 1, &ok);
    if (!ok) return;

    // Create inventory item
    Frontier::InventoryItem invItem;
    invItem.itemId = selectedItem->id.value_or(0);
    invItem.quantity = quantity;

    if (m_database->addInventoryItem(invItem) > 0) {
        loadInventory();
        emit dataChanged();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to add inventory item."));
    }
}

void InventoryTab::onEditItem()
{
    auto selected = m_table->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    if (row < 0 || row >= m_filteredItems.size()) return;

    auto item = m_filteredItems[row];

    // Get locations for selection
    auto locations = m_database->getAllLocations();
    QStringList locNames;
    locNames << tr("(None)");
    for (const auto &loc : locations) {
        locNames << loc.name;
    }

    bool ok;
    QString selectedLoc = QInputDialog::getItem(this, tr("Edit Location"),
                                                tr("Storage Location:"), locNames,
                                                item.locationId.has_value() ? locNames.indexOf(item.locationName) : 0,
                                                false, &ok);

    if (!ok) return;

    // Find location ID
    std::optional<int> locId;
    if (selectedLoc != tr("(None)")) {
        for (const auto &loc : locations) {
            if (loc.name == selectedLoc) {
                locId = loc.id;
                break;
            }
        }
    }

    item.locationId = locId;
    if (m_database->updateInventoryItem(item)) {
        loadInventory();
        emit dataChanged();
    }
}

void InventoryTab::onAdjustQuantity()
{
    auto selected = m_table->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    if (row < 0 || row >= m_filteredItems.size()) return;

    const auto &item = m_filteredItems[row];

    // Create adjustment dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Adjust Quantity"));
    dialog.setMinimumWidth(350);

    auto *layout = new QVBoxLayout(&dialog);

    // Item info
    auto *infoGroup = new QGroupBox(tr("Item"));
    auto *infoLayout = new QFormLayout(infoGroup);
    infoLayout->addRow(tr("Name:"), new QLabel(item.itemName));
    infoLayout->addRow(tr("Current Qty:"), new QLabel(QString("%L1").arg(item.quantity)));
    layout->addWidget(infoGroup);

    // Adjustment type
    auto *typeGroup = new QGroupBox(tr("Adjustment Type"));
    auto *typeLayout = new QVBoxLayout(typeGroup);

    auto *buttonGroup = new QButtonGroup(&dialog);
    auto *addRadio = new QRadioButton(tr("Add to inventory"));
    addRadio->setChecked(true);
    buttonGroup->addButton(addRadio, 0);
    typeLayout->addWidget(addRadio);

    auto *removeRadio = new QRadioButton(tr("Remove from inventory"));
    buttonGroup->addButton(removeRadio, 1);
    typeLayout->addWidget(removeRadio);

    auto *setRadio = new QRadioButton(tr("Set exact quantity"));
    buttonGroup->addButton(setRadio, 2);
    typeLayout->addWidget(setRadio);

    layout->addWidget(typeGroup);

    // Amount
    auto *amountLayout = new QHBoxLayout();
    amountLayout->addWidget(new QLabel(tr("Amount:")));
    auto *amountSpin = new QSpinBox();
    amountSpin->setRange(0, 9999999);
    amountLayout->addWidget(amountSpin);
    layout->addLayout(amountLayout);

    // Preview
    auto *previewLayout = new QHBoxLayout();
    previewLayout->addWidget(new QLabel(tr("New Qty:")));
    auto *previewLabel = new QLabel(QString("%L1").arg(item.quantity));
    previewLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    previewLayout->addWidget(previewLabel);
    previewLayout->addStretch();
    layout->addLayout(previewLayout);

    // Update preview
    auto updatePreview = [&]() {
        int newQty = item.quantity;
        int amount = amountSpin->value();
        if (addRadio->isChecked()) {
            newQty = item.quantity + amount;
        } else if (removeRadio->isChecked()) {
            newQty = qMax(0, item.quantity - amount);
        } else {
            newQty = amount;
        }
        previewLabel->setText(QString("%L1").arg(newQty));
        if (newQty < 0) {
            previewLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: red;");
        } else {
            previewLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2e7d32;");
        }
    };

    connect(amountSpin, QOverload<int>::of(&QSpinBox::valueChanged), updatePreview);
    connect(addRadio, &QRadioButton::toggled, updatePreview);
    connect(removeRadio, &QRadioButton::toggled, updatePreview);
    connect(setRadio, &QRadioButton::toggled, updatePreview);

    // Buttons
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) return;

    // Calculate new quantity
    int newQty = item.quantity;
    int amount = amountSpin->value();
    if (addRadio->isChecked()) {
        newQty = item.quantity + amount;
    } else if (removeRadio->isChecked()) {
        newQty = qMax(0, item.quantity - amount);
    } else {
        newQty = amount;
    }

    // Track oil sales
    if (item.itemName == "Oil" && newQty < item.quantity) {
        int sold = item.quantity - newQty;
        m_database->addOilSold(sold);
        m_oilTracking = m_database->getOilTracking();
    }

    // Update quantity
    if (m_database->updateInventoryQuantity(item.id.value_or(0), newQty)) {
        loadInventory();
        emit dataChanged();
    }
}

void InventoryTab::onDeleteItem()
{
    auto selected = m_table->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    if (row < 0 || row >= m_filteredItems.size()) return;

    const auto &item = m_filteredItems[row];

    auto result = QMessageBox::question(this, tr("Delete Item"),
                                        tr("Delete '%1' from inventory?").arg(item.itemName));

    if (result == QMessageBox::Yes) {
        if (m_database->deleteInventoryItem(item.id.value_or(0))) {
            loadInventory();
            emit dataChanged();
        }
    }
}

void InventoryTab::onSyncFromLedger()
{
    QMessageBox::information(this, tr("Sync from Ledger"),
                             tr("This feature syncs inventory from Ledger transactions.\n\n"
                                "It will be implemented when the Ledger integration is complete."));
    // TODO: Implement when Ledger tab is available
}

// =============================================================================
// Public API
// =============================================================================

bool InventoryTab::adjustItemQuantity(const QString &itemName, int delta, bool trackOil)
{
    auto invItem = m_database->getInventoryByItemName(itemName);
    if (!invItem.has_value()) {
        return false;
    }

    int newQty = qMax(0, invItem->quantity + delta);

    // Track oil sales
    if (trackOil && itemName == "Oil" && delta < 0) {
        m_database->addOilSold(qAbs(delta));
        m_oilTracking = m_database->getOilTracking();
    }

    bool success = m_database->updateInventoryQuantity(invItem->id.value_or(0), newQty);
    if (success) {
        loadInventory();
    }
    return success;
}

bool InventoryTab::addOrUpdateItem(int itemId, int quantity, std::optional<int> locationId)
{
    auto existing = m_database->getInventoryByItemId(itemId);
    if (existing.has_value()) {
        // Update existing
        int newQty = existing->quantity + quantity;
        bool success = m_database->updateInventoryQuantity(existing->id.value_or(0), newQty);
        if (success) loadInventory();
        return success;
    } else {
        // Add new
        Frontier::InventoryItem item;
        item.itemId = itemId;
        item.quantity = quantity;
        item.locationId = locationId;
        bool success = m_database->addInventoryItem(item) > 0;
        if (success) loadInventory();
        return success;
    }
}

int InventoryTab::getItemQuantity(const QString &itemName) const
{
    for (const auto &item : m_allItems) {
        if (item.itemName == itemName) {
            return item.quantity;
        }
    }
    return 0;
}
