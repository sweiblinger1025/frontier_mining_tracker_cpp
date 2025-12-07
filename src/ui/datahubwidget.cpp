/**
 * @file datahubwidget.cpp
 * @brief Implementation of Data Hub tab
 */

#include "datahubwidget.h"
#include "additemdialog.h"
#include "vehiclespecstab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

DataHubWidget::DataHubWidget(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_model(nullptr)
    , m_proxyModel(nullptr)
    , m_vehicleSpecsTab(nullptr)
{
    setupUi();
    loadCategories();
    loadItems();
}

DataHubWidget::~DataHubWidget()
{
}

void DataHubWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Create subtabs
    m_subTabs = new QTabWidget();

    // Add tabs
    m_subTabs->addTab(createItemsTab(), "Items");
    
    // Vehicle Specs tab (now functional!)
    m_vehicleSpecsTab = new VehicleSpecsTab(m_database, this);
    m_subTabs->addTab(m_vehicleSpecsTab, "Vehicle Specs");
    
    // Placeholder tabs for future
    m_subTabs->addTab(new QLabel("Factory Buildings - Coming Soon"), "Factory - Buildings");
    m_subTabs->addTab(new QLabel("Recipes - Coming Soon"), "Recipes");
    m_subTabs->addTab(new QLabel("Locations - Coming Soon"), "Locations");

    mainLayout->addWidget(m_subTabs);
}

QWidget* DataHubWidget::createItemsTab()
{
    QWidget *itemsTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(itemsTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // === Filter Bar ===
    QHBoxLayout *filterLayout = new QHBoxLayout();

    // Category filter
    QLabel *categoryLabel = new QLabel("Category:");
    m_categoryFilter = new QComboBox();
    m_categoryFilter->setMinimumWidth(200);

    // Search box
    QLabel *searchLabel = new QLabel("Search:");
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Type to search...");
    m_searchBox->setClearButtonEnabled(true);

    filterLayout->addWidget(categoryLabel);
    filterLayout->addWidget(m_categoryFilter);
    filterLayout->addSpacing(20);
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
        "Name", "Category", "Buy Price", "Sell Price", "Margin", "ROI %",
        "Can Buy", "Can Sell", "Craftable"
    });

    // Create proxy model for filtering/sorting
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);

    m_tableView->setModel(m_proxyModel);

    mainLayout->addWidget(m_tableView, 1);

    // === Bottom Bar ===
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    m_itemCountLabel = new QLabel("Items: 0");

    m_addButton = new QPushButton("Add Item");
    m_deleteButton = new QPushButton("Delete");

    bottomLayout->addWidget(m_itemCountLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_addButton);
    bottomLayout->addWidget(m_deleteButton);

    mainLayout->addLayout(bottomLayout);

    // === Connect Signals ===
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataHubWidget::onCategoryFilterChanged);

    connect(m_searchBox, &QLineEdit::textChanged,
            this, &DataHubWidget::onSearchTextChanged);

    connect(m_addButton, &QPushButton::clicked,
            this, &DataHubWidget::onAddItemClicked);

    connect(m_deleteButton, &QPushButton::clicked,
            this, &DataHubWidget::onDeleteItemClicked);

    connect(m_tableView, &QTableView::doubleClicked,
            this, &DataHubWidget::onItemDoubleClicked);

    return itemsTab;
}

void DataHubWidget::loadCategories()
{
    m_categoryFilter->clear();
    m_categoryFilter->addItem("All Categories", "");

    // Get unique main categories from items
    QSet<QString> categories;
    auto items = m_database->getAllItems();

    for (const auto &item : items) {
        categories.insert(item.categoryMain);
    }

    // Sort and add to combo box
    QStringList sortedCategories = categories.values();
    sortedCategories.sort();

    for (const QString &category : sortedCategories) {
        m_categoryFilter->addItem(category, category);
    }
}

void DataHubWidget::loadItems()
{
    // Get items from database
    m_items = m_database->getAllItems();

    // Clear existing data
    m_model->removeRows(0, m_model->rowCount());

    // Populate table
    for (const auto &item : m_items) {
        QList<QStandardItem*> row;

        // Name
        row.append(new QStandardItem(item.name));

        // Category (display format)
        row.append(new QStandardItem(item.displayCategory()));

        // Buy Price (formatted - fixed notation for large numbers)
        QStandardItem *buyItem = new QStandardItem();
        buyItem->setData(QString("$%L1").arg(item.buyPriceDisplay, 0, 'f', 0), Qt::DisplayRole);
        buyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(buyItem);

        // Sell Price (formatted - fixed notation for large numbers)
        QStandardItem *sellItem = new QStandardItem();
        sellItem->setData(QString("$%L1").arg(item.sellPriceDisplay, 0, 'f', 0), Qt::DisplayRole);
        sellItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row.append(sellItem);

        // Margin (calculated)
        double margin = item.sellPriceInternal - item.buyPriceInternal;
        QStandardItem *marginItem = new QStandardItem();
        marginItem->setData(QString("$%L1").arg(margin, 0, 'f', 0), Qt::DisplayRole);
        marginItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (margin < 0) {
            marginItem->setForeground(QBrush(Qt::red));
        } else if (margin > 0) {
            marginItem->setForeground(QBrush(Qt::darkGreen));
        }
        row.append(marginItem);

        // ROI %
        double roi = item.roiPercent();
        QStandardItem *roiItem = new QStandardItem();
        roiItem->setData(QString("%1%").arg(roi, 0, 'f', 2), Qt::DisplayRole);
        roiItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (roi < 0) {
            roiItem->setForeground(QBrush(Qt::red));
        } else if (roi > 0) {
            roiItem->setForeground(QBrush(Qt::darkGreen));
        }
        row.append(roiItem);

        // Can Buy
        QStandardItem *canBuyItem = new QStandardItem();
        canBuyItem->setData(item.isPurchasable ? "✓" : "✗", Qt::DisplayRole);
        canBuyItem->setTextAlignment(Qt::AlignCenter);
        if (!item.isPurchasable) {
            canBuyItem->setForeground(QBrush(Qt::gray));
        }
        row.append(canBuyItem);

        // Can Sell
        QStandardItem *canSellItem = new QStandardItem();
        canSellItem->setData(item.isSellable ? "✓" : "✗", Qt::DisplayRole);
        canSellItem->setTextAlignment(Qt::AlignCenter);
        if (!item.isSellable) {
            canSellItem->setForeground(QBrush(Qt::gray));
        }
        row.append(canSellItem);

        // Craftable
        QStandardItem *craftableItem = new QStandardItem();
        craftableItem->setData(item.isCraftable ? "✓" : "✗", Qt::DisplayRole);
        craftableItem->setTextAlignment(Qt::AlignCenter);
        if (!item.isCraftable) {
            craftableItem->setForeground(QBrush(Qt::gray));
        }
        row.append(craftableItem);

        // Store item ID in first column for later retrieval
        row[0]->setData(item.id.value_or(-1), Qt::UserRole);

        m_model->appendRow(row);
    }

    m_itemCountLabel->setText(QString("Items: %1").arg(m_items.size()));
    m_tableView->resizeColumnsToContents();
}

void DataHubWidget::refreshData()
{
    loadCategories();
    loadItems();
    
    // Also refresh Vehicle Specs tab
    if (m_vehicleSpecsTab) {
        m_vehicleSpecsTab->refreshData();
    }
}

void DataHubWidget::applyFilters()
{
    QString selectedCategory = m_categoryFilter->currentData().toString();
    QString searchText = m_searchBox->text();

    // Build filter regex
    if (selectedCategory.isEmpty()) {
        // Just use search text
        m_proxyModel->setFilterRegularExpression(
            QRegularExpression(searchText, QRegularExpression::CaseInsensitiveOption));
    } else {
        // Filter by category first, then search
        // This is a simplified approach - we filter by search text
        // and let the category filter work through the visible items
        m_proxyModel->setFilterRegularExpression(
            QRegularExpression(searchText, QRegularExpression::CaseInsensitiveOption));
    }

    // Update visible count
    m_itemCountLabel->setText(QString("Items: %1 / %2")
                                  .arg(m_proxyModel->rowCount())
                                  .arg(m_items.size()));
}

void DataHubWidget::onCategoryFilterChanged(int index)
{
    Q_UNUSED(index);

    QString selectedCategory = m_categoryFilter->currentData().toString();

    if (selectedCategory.isEmpty()) {
        // Show all - reload full list
        loadItems();
    } else {
        // Filter by category
        m_items = m_database->getItemsByCategory(selectedCategory);

        m_model->removeRows(0, m_model->rowCount());

        for (const auto &item : m_items) {
            QList<QStandardItem*> row;

            row.append(new QStandardItem(item.name));
            row.append(new QStandardItem(item.displayCategory()));

            // Fixed notation for large numbers
            QStandardItem *buyItem = new QStandardItem();
            buyItem->setData(QString("$%L1").arg(item.buyPriceDisplay, 0, 'f', 0), Qt::DisplayRole);
            buyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            row.append(buyItem);

            QStandardItem *sellItem = new QStandardItem();
            sellItem->setData(QString("$%L1").arg(item.sellPriceDisplay, 0, 'f', 0), Qt::DisplayRole);
            sellItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            row.append(sellItem);

            double margin = item.sellPriceInternal - item.buyPriceInternal;
            QStandardItem *marginItem = new QStandardItem();
            marginItem->setData(QString("$%L1").arg(margin, 0, 'f', 0), Qt::DisplayRole);
            marginItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (margin < 0) {
                marginItem->setForeground(QBrush(Qt::red));
            }
            row.append(marginItem);

            double roi = item.roiPercent();
            QStandardItem *roiItem = new QStandardItem();
            roiItem->setData(QString("%1%").arg(roi, 0, 'f', 2), Qt::DisplayRole);
            roiItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (roi < 0) {
                roiItem->setForeground(QBrush(Qt::red));
            }
            row.append(roiItem);

            QStandardItem *canBuyItem = new QStandardItem();
            canBuyItem->setData(item.isPurchasable ? "✓" : "✗", Qt::DisplayRole);
            canBuyItem->setTextAlignment(Qt::AlignCenter);
            if (!item.isPurchasable) {
                canBuyItem->setForeground(QBrush(Qt::gray));
            }
            row.append(canBuyItem);

            QStandardItem *canSellItem = new QStandardItem();
            canSellItem->setData(item.isSellable ? "✓" : "✗", Qt::DisplayRole);
            canSellItem->setTextAlignment(Qt::AlignCenter);
            if (!item.isSellable) {
                canSellItem->setForeground(QBrush(Qt::gray));
            }
            row.append(canSellItem);

            QStandardItem *craftableItem = new QStandardItem();
            craftableItem->setData(item.isCraftable ? "✓" : "✗", Qt::DisplayRole);
            craftableItem->setTextAlignment(Qt::AlignCenter);
            if (!item.isCraftable) {
                craftableItem->setForeground(QBrush(Qt::gray));
            }
            row.append(craftableItem);

            row[0]->setData(item.id.value_or(-1), Qt::UserRole);

            m_model->appendRow(row);
        }

        m_itemCountLabel->setText(QString("Items: %1").arg(m_items.size()));
        m_tableView->resizeColumnsToContents();
    }
}

void DataHubWidget::onSearchTextChanged(const QString &text)
{
    m_proxyModel->setFilterRegularExpression(
        QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));

    m_itemCountLabel->setText(QString("Items: %1 / %2")
                                  .arg(m_proxyModel->rowCount())
                                  .arg(m_model->rowCount()));
}

void DataHubWidget::onAddItemClicked()
{
    AddItemDialog dialog(this);

    // Get unique categories for dropdowns
    QSet<QString> mainCats, subCats;
    for (const auto &item : m_items) {
        mainCats.insert(item.categoryMain);
        subCats.insert(item.categorySub);
    }

    QStringList mainList = mainCats.values();
    QStringList subList = subCats.values();
    mainList.sort();
    subList.sort();

    dialog.setCategories(mainList, subList);

    if (dialog.exec() == QDialog::Accepted) {
        Frontier::Item newItem = dialog.getItem();

        if (m_database->addItem(newItem)) {
            loadItems();  // Refresh table
            QMessageBox::information(this, "Success", "Item added successfully!");
        } else {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to add item: %1").arg(m_database->lastError()));
        }
    }
}

void DataHubWidget::onDeleteItemClicked()
{
    QModelIndex currentIndex = m_tableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Delete Item", "Please select an item to delete.");
        return;
    }

    // Get the source index (in case of proxy model)
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    int itemId = m_model->item(sourceIndex.row(), 0)->data(Qt::UserRole).toInt();
    QString itemName = m_model->item(sourceIndex.row(), 0)->text();

    // Confirm deletion
    int result = QMessageBox::question(this, "Delete Item",
                                       QString("Are you sure you want to delete '%1'?").arg(itemName),
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        if (m_database->deleteItem(itemId)) {
            loadItems();  // Refresh table
        } else {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to delete item: %1").arg(m_database->lastError()));
        }
    }
}

void DataHubWidget::onItemDoubleClicked(const QModelIndex &index)
{
    // Get the source index
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    int itemId = m_model->item(sourceIndex.row(), 0)->data(Qt::UserRole).toInt();

    // Find the item
    auto itemOpt = m_database->getItem(itemId);
    if (!itemOpt.has_value()) {
        QMessageBox::warning(this, "Error", "Could not load item.");
        return;
    }

    AddItemDialog dialog(this);

    // Get unique categories for dropdowns
    QSet<QString> mainCats, subCats;
    for (const auto &item : m_items) {
        mainCats.insert(item.categoryMain);
        subCats.insert(item.categorySub);
    }

    QStringList mainList = mainCats.values();
    QStringList subList = subCats.values();
    mainList.sort();
    subList.sort();

    dialog.setCategories(mainList, subList);
    dialog.setItem(itemOpt.value());

    if (dialog.exec() == QDialog::Accepted) {
        Frontier::Item updatedItem = dialog.getItem();

        if (m_database->updateItem(updatedItem)) {
            loadItems();  // Refresh table
            QMessageBox::information(this, "Success", "Item updated successfully!");
        } else {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to update item: %1").arg(m_database->lastError()));
        }
    }
}
