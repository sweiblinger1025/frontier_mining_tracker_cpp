/**
 * @file productioncalculatortab.cpp
 * @brief Production Calculator implementation
 */

#include "productioncalculatortab.h"
#include "core/database.h"
#include "inventorytab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>

// =============================================================================
// Constructor & Setup
// =============================================================================

ProductionCalculatorTab::ProductionCalculatorTab(Frontier::Database *database,
                                                 InventoryTab *inventoryTab,
                                                 QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_inventoryTab(inventoryTab)
{
    setupUi();
    refreshData();
}

void ProductionCalculatorTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Input panel
    mainLayout->addWidget(createInputPanel());

    // Summary panel
    mainLayout->addWidget(createSummaryPanel());

    // Results tree
    auto *treeGroup = new QGroupBox(tr("Material Requirements"));
    auto *treeLayout = new QVBoxLayout(treeGroup);

    m_treeWidget = new QTreeWidget();
    m_treeWidget->setColumnCount(5);
    m_treeWidget->setHeaderLabels({
        tr("Item"), tr("Needed"), tr("In Stock"), tr("Shortfall"), tr("Cost")
    });
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setRootIsDecorated(true);

    auto *header = m_treeWidget->header();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    treeLayout->addWidget(m_treeWidget);
    mainLayout->addWidget(treeGroup, 1);

    // Legend
    auto *legendLayout = new QHBoxLayout();

    auto *greenLabel = new QLabel(tr("● Sufficient stock"));
    greenLabel->setStyleSheet("color: #2e7d32;");
    legendLayout->addWidget(greenLabel);

    auto *yellowLabel = new QLabel(tr("● Partial stock"));
    yellowLabel->setStyleSheet("color: #f57c00;");
    legendLayout->addWidget(yellowLabel);

    auto *redLabel = new QLabel(tr("● No stock (shortfall)"));
    redLabel->setStyleSheet("color: #c62828;");
    legendLayout->addWidget(redLabel);

    auto *blueLabel = new QLabel(tr("● Craftable (expandable)"));
    blueLabel->setStyleSheet("color: #1976d2;");
    legendLayout->addWidget(blueLabel);

    legendLayout->addStretch();
    mainLayout->addLayout(legendLayout);
}

QWidget* ProductionCalculatorTab::createInputPanel()
{
    auto *group = new QGroupBox(tr("Production Setup"));
    auto *layout = new QHBoxLayout(group);

    // Recipe selection
    layout->addWidget(new QLabel(tr("Recipe:")));
    m_recipeCombo = new QComboBox();
    m_recipeCombo->setMinimumWidth(250);
    connect(m_recipeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProductionCalculatorTab::onRecipeChanged);
    layout->addWidget(m_recipeCombo);

    // Quantity
    layout->addWidget(new QLabel(tr("Quantity:")));
    m_quantitySpin = new QSpinBox();
    m_quantitySpin->setRange(1, 9999);
    m_quantitySpin->setValue(1);
    layout->addWidget(m_quantitySpin);

    // Full chain checkbox
    m_fullChainCheck = new QCheckBox(tr("Show full production chain"));
    m_fullChainCheck->setChecked(true);
    m_fullChainCheck->setToolTip(tr("Recursively expand craftable ingredients into their components"));
    layout->addWidget(m_fullChainCheck);

    layout->addStretch();

    // Calculate button
    m_calculateBtn = new QPushButton(tr("Calculate"));
    m_calculateBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold; padding: 8px 20px;");
    connect(m_calculateBtn, &QPushButton::clicked, this, &ProductionCalculatorTab::onCalculate);
    layout->addWidget(m_calculateBtn);

    return group;
}

QWidget* ProductionCalculatorTab::createSummaryPanel()
{
    auto *group = new QGroupBox(tr("Production Summary"));
    auto *layout = new QHBoxLayout(group);

    // Total Material Types
    auto *typesLayout = new QVBoxLayout();
    typesLayout->addWidget(new QLabel(tr("Material Types")));
    m_totalMaterialsLabel = new QLabel("0");
    m_totalMaterialsLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    typesLayout->addWidget(m_totalMaterialsLabel);
    layout->addLayout(typesLayout);

    // Separator
    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::VLine);
    layout->addWidget(sep1);

    // Total Units
    auto *unitsLayout = new QVBoxLayout();
    unitsLayout->addWidget(new QLabel(tr("Total Units")));
    m_totalUnitsLabel = new QLabel("0");
    m_totalUnitsLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    unitsLayout->addWidget(m_totalUnitsLabel);
    layout->addLayout(unitsLayout);

    // Separator
    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::VLine);
    layout->addWidget(sep2);

    // Input Cost
    auto *inputLayout = new QVBoxLayout();
    inputLayout->addWidget(new QLabel(tr("Input Cost")));
    m_inputCostLabel = new QLabel("$0");
    m_inputCostLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #c62828;");
    inputLayout->addWidget(m_inputCostLabel);
    layout->addLayout(inputLayout);

    // Separator
    auto *sep3 = new QFrame();
    sep3->setFrameShape(QFrame::VLine);
    layout->addWidget(sep3);

    // Output Value
    auto *outputLayout = new QVBoxLayout();
    outputLayout->addWidget(new QLabel(tr("Output Value")));
    m_outputValueLabel = new QLabel("$0");
    m_outputValueLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2e7d32;");
    outputLayout->addWidget(m_outputValueLabel);
    layout->addLayout(outputLayout);

    // Separator
    auto *sep4 = new QFrame();
    sep4->setFrameShape(QFrame::VLine);
    layout->addWidget(sep4);

    // Profit
    auto *profitLayout = new QVBoxLayout();
    profitLayout->addWidget(new QLabel(tr("Profit")));
    m_profitLabel = new QLabel("$0");
    m_profitLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    profitLayout->addWidget(m_profitLabel);
    layout->addLayout(profitLayout);

    // Separator
    auto *sep5 = new QFrame();
    sep5->setFrameShape(QFrame::VLine);
    layout->addWidget(sep5);

    // Margin
    auto *marginLayout = new QVBoxLayout();
    marginLayout->addWidget(new QLabel(tr("Margin")));
    m_marginLabel = new QLabel("0%");
    m_marginLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    marginLayout->addWidget(m_marginLabel);
    layout->addLayout(marginLayout);

    layout->addStretch();

    return group;
}

// =============================================================================
// Data Loading
// =============================================================================

void ProductionCalculatorTab::refreshData()
{
    populateRecipeCombo();
}

void ProductionCalculatorTab::populateRecipeCombo()
{
    m_recipeCombo->blockSignals(true);
    m_recipeCombo->clear();

    // Load all recipes
    m_recipes = m_database->getAllRecipes();
    m_recipesByOutput.clear();

    // Build lookup map and populate combo
    for (const auto &recipe : m_recipes) {
        QString displayName = recipe.outputItem;
        if (recipe.outputQty > 1) {
            displayName += QString(" (×%1)").arg(recipe.outputQty);
        }
        if (!recipe.notes.isEmpty()) {
            displayName += QString(" - %1").arg(recipe.notes);
        }
        displayName += QString(" [%1]").arg(recipe.workbenchName);

        m_recipeCombo->addItem(displayName, recipe.id.value_or(0));

        // Store in lookup (use first recipe if multiple exist for same output)
        if (!m_recipesByOutput.contains(recipe.outputItem)) {
            m_recipesByOutput[recipe.outputItem] = recipe;
        }
    }

    m_recipeCombo->blockSignals(false);

    clearResults();
}

// =============================================================================
// Calculation Logic
// =============================================================================

void ProductionCalculatorTab::onRecipeChanged()
{
    clearResults();
}

void ProductionCalculatorTab::onCalculate()
{
    calculate();
}

void ProductionCalculatorTab::calculate()
{
    int recipeIdx = m_recipeCombo->currentIndex();
    if (recipeIdx < 0 || recipeIdx >= m_recipes.size()) {
        return;
    }

    const auto &recipe = m_recipes[recipeIdx];
    int quantity = m_quantitySpin->value();
    bool expandChain = m_fullChainCheck->isChecked();

    // Build the production tree
    m_rootNode = ProductionNode();
    m_rootNode.itemName = recipe.outputItem;
    m_rootNode.quantityNeeded = recipe.outputQty * quantity;
    m_rootNode.quantityInInventory = getInventoryQuantity(recipe.outputItem);
    m_rootNode.isCraftable = true;

    // Build children from recipe ingredients
    QSet<QString> visited;
    visited.insert(recipe.outputItem);  // Prevent infinite recursion

    for (const auto &ing : recipe.ingredients) {
        ProductionNode childNode;
        childNode.itemName = ing.itemName;
        childNode.quantityNeeded = ing.quantity * quantity;
        childNode.quantityInInventory = getInventoryQuantity(ing.itemName);

        // Check if this ingredient is craftable
        auto childRecipe = findRecipeForItem(ing.itemName);
        childNode.isCraftable = childRecipe.has_value();

        if (expandChain && childNode.isCraftable && !visited.contains(ing.itemName)) {
            // Recursively expand
            buildProductionTree(childNode, ing.quantity * quantity, expandChain, visited);
        } else {
            // Raw material or not expanding
            childNode.isRawMaterial = true;
            auto item = m_database->getItemByName(ing.itemName);
            if (item.has_value()) {
                childNode.unitCost = item->buyPriceInternal;
            }
        }

        m_rootNode.children.append(childNode);
    }

    // Collect raw materials for summary
    QMap<QString, int> rawMaterials;
    collectRawMaterials(m_rootNode, rawMaterials);

    // Calculate summary
    m_summary = ProductionSummary();
    m_summary.totalMaterialTypes = rawMaterials.size();
    m_summary.rawMaterials = rawMaterials;

    for (auto it = rawMaterials.begin(); it != rawMaterials.end(); ++it) {
        m_summary.totalMaterialUnits += it.value();

        auto item = m_database->getItemByName(it.key());
        if (item.has_value()) {
            m_summary.inputCost += item->buyPriceInternal * it.value();
        }

        int inStock = getInventoryQuantity(it.key());
        if (it.value() > inStock) {
            m_summary.shortfalls[it.key()] = it.value() - inStock;
        }
    }

    // Output value
    auto outputItem = m_database->getItemByName(recipe.outputItem);
    if (outputItem.has_value()) {
        m_summary.outputValue = outputItem->sellPriceInternal * recipe.outputQty * quantity;
    }

    m_summary.profit = m_summary.outputValue - m_summary.inputCost;
    if (m_summary.inputCost > 0) {
        m_summary.marginPercent = (m_summary.profit / m_summary.inputCost) * 100.0;
    }

    // Update UI
    updateSummary(m_summary);

    m_treeWidget->clear();
    populateTreeWidget(m_rootNode);
    m_treeWidget->expandAll();
}

void ProductionCalculatorTab::buildProductionTree(ProductionNode &node, int quantity,
                                                  bool expandChain, QSet<QString> &visited)
{
    auto recipe = findRecipeForItem(node.itemName);
    if (!recipe.has_value()) {
        node.isRawMaterial = true;
        auto item = m_database->getItemByName(node.itemName);
        if (item.has_value()) {
            node.unitCost = item->buyPriceInternal;
        }
        return;
    }

    visited.insert(node.itemName);

    // Calculate how many times we need to run this recipe
    int runsNeeded = (quantity + recipe->outputQty - 1) / recipe->outputQty;  // Ceiling division

    for (const auto &ing : recipe->ingredients) {
        ProductionNode childNode;
        childNode.itemName = ing.itemName;
        childNode.quantityNeeded = ing.quantity * runsNeeded;
        childNode.quantityInInventory = getInventoryQuantity(ing.itemName);

        auto childRecipe = findRecipeForItem(ing.itemName);
        childNode.isCraftable = childRecipe.has_value();

        if (expandChain && childNode.isCraftable && !visited.contains(ing.itemName)) {
            buildProductionTree(childNode, childNode.quantityNeeded, expandChain, visited);
        } else {
            childNode.isRawMaterial = true;
            auto item = m_database->getItemByName(ing.itemName);
            if (item.has_value()) {
                childNode.unitCost = item->buyPriceInternal;
            }
        }

        node.children.append(childNode);
    }

    visited.remove(node.itemName);  // Allow this item in other branches
}

void ProductionCalculatorTab::collectRawMaterials(const ProductionNode &node,
                                                  QMap<QString, int> &materials)
{
    if (node.isRawMaterial || node.children.isEmpty()) {
        // This is a leaf node - add to raw materials
        materials[node.itemName] = materials.value(node.itemName, 0) + node.quantityNeeded;
    } else {
        // Recurse into children
        for (const auto &child : node.children) {
            collectRawMaterials(child, materials);
        }
    }
}

void ProductionCalculatorTab::populateTreeWidget(const ProductionNode &node,
                                                 QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item;
    if (parent) {
        item = new QTreeWidgetItem(parent);
    } else {
        item = new QTreeWidgetItem(m_treeWidget);
    }

    // Item name
    QString displayName = node.itemName;
    if (node.isCraftable && !node.isRawMaterial) {
        displayName += " [Craftable]";
    }
    item->setText(0, displayName);

    // Quantity needed
    item->setText(1, QString("%L1").arg(node.quantityNeeded));
    item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);

    // In stock
    item->setText(2, QString("%L1").arg(node.quantityInInventory));
    item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);

    // Shortfall
    int shortfall = node.shortfall();
    if (shortfall > 0) {
        item->setText(3, QString("-%L1").arg(shortfall));
    } else {
        item->setText(3, "-");
    }
    item->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);

    // Cost (only for raw materials)
    if (node.isRawMaterial && node.unitCost > 0) {
        double totalCost = node.unitCost * node.quantityNeeded;
        item->setText(4, QString("$%L1").arg(totalCost, 0, 'f', 2));
    } else {
        item->setText(4, "-");
    }
    item->setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);

    // Color coding
    QColor textColor;
    if (node.isCraftable && !node.isRawMaterial) {
        textColor = QColor("#1976d2");  // Blue for craftable
    } else if (node.quantityInInventory >= node.quantityNeeded) {
        textColor = QColor("#2e7d32");  // Green - sufficient
    } else if (node.quantityInInventory > 0) {
        textColor = QColor("#f57c00");  // Orange - partial
    } else {
        textColor = QColor("#c62828");  // Red - no stock
    }

    for (int col = 0; col < 5; ++col) {
        item->setForeground(col, textColor);
    }

    // Highlight shortfall column in red if there is one
    if (shortfall > 0) {
        item->setForeground(3, QColor("#c62828"));
        QFont font = item->font(3);
        font.setBold(true);
        item->setFont(3, font);
    }

    // Add children
    for (const auto &child : node.children) {
        populateTreeWidget(child, item);
    }
}

void ProductionCalculatorTab::updateSummary(const ProductionSummary &summary)
{
    m_totalMaterialsLabel->setText(QString::number(summary.totalMaterialTypes));
    m_totalUnitsLabel->setText(QString("%L1").arg(summary.totalMaterialUnits));
    m_inputCostLabel->setText(QString("$%L1").arg(summary.inputCost, 0, 'f', 2));
    m_outputValueLabel->setText(QString("$%L1").arg(summary.outputValue, 0, 'f', 2));
    m_profitLabel->setText(QString("$%L1").arg(summary.profit, 0, 'f', 2));
    m_marginLabel->setText(QString("%1%").arg(summary.marginPercent, 0, 'f', 1));

    // Color profit based on value
    if (summary.profit > 0) {
        m_profitLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2e7d32;");
    } else if (summary.profit < 0) {
        m_profitLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #c62828;");
    } else {
        m_profitLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    }

    // Color margin based on value
    if (summary.marginPercent > 50) {
        m_marginLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2e7d32;");
    } else if (summary.marginPercent > 0) {
        m_marginLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #f57c00;");
    } else {
        m_marginLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #c62828;");
    }
}

void ProductionCalculatorTab::clearResults()
{
    m_treeWidget->clear();
    m_rootNode = ProductionNode();
    m_summary = ProductionSummary();

    m_totalMaterialsLabel->setText("0");
    m_totalUnitsLabel->setText("0");
    m_inputCostLabel->setText("$0");
    m_outputValueLabel->setText("$0");
    m_profitLabel->setText("$0");
    m_profitLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    m_marginLabel->setText("0%");
    m_marginLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
}

// =============================================================================
// Helper Methods
// =============================================================================

int ProductionCalculatorTab::getInventoryQuantity(const QString &itemName) const
{
    if (m_inventoryTab) {
        return m_inventoryTab->getItemQuantity(itemName);
    }
    return 0;
}

std::optional<Frontier::Recipe> ProductionCalculatorTab::findRecipeForItem(const QString &itemName) const
{
    auto it = m_recipesByOutput.find(itemName);
    if (it != m_recipesByOutput.end()) {
        return it.value();
    }
    return std::nullopt;
}
