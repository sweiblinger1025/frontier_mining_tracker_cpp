/**
 * @file productionlogtab.cpp
 * @brief Production Log implementation
 */

#include "productionlogtab.h"
#include "core/database.h"
#include "inventorytab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>
#include <QSplitter>

// =============================================================================
// Constructor & Setup
// =============================================================================

ProductionLogTab::ProductionLogTab(Frontier::Database *database,
                                   InventoryTab *inventoryTab,
                                   QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_inventoryTab(inventoryTab)
{
    setupUi();
    refreshData();
}

void ProductionLogTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Top section: Input + Preview
    auto *topSplitter = new QSplitter(Qt::Horizontal);
    topSplitter->addWidget(createInputPanel());

    // Preview panel
    auto *previewGroup = new QGroupBox(tr("Production Preview"));
    auto *previewLayout = new QVBoxLayout(previewGroup);

    auto *costRow = new QHBoxLayout();
    costRow->addWidget(new QLabel(tr("Input Cost:")));
    m_previewInputCost = new QLabel("$0");
    m_previewInputCost->setStyleSheet("font-weight: bold; color: #c62828;");
    costRow->addWidget(m_previewInputCost);
    costRow->addStretch();
    previewLayout->addLayout(costRow);

    auto *valueRow = new QHBoxLayout();
    valueRow->addWidget(new QLabel(tr("Output Value:")));
    m_previewOutputValue = new QLabel("$0");
    m_previewOutputValue->setStyleSheet("font-weight: bold; color: #2e7d32;");
    valueRow->addWidget(m_previewOutputValue);
    valueRow->addStretch();
    previewLayout->addLayout(valueRow);

    auto *profitRow = new QHBoxLayout();
    profitRow->addWidget(new QLabel(tr("Profit:")));
    m_previewProfit = new QLabel("$0");
    m_previewProfit->setStyleSheet("font-weight: bold; font-size: 14px;");
    profitRow->addWidget(m_previewProfit);
    profitRow->addStretch();
    previewLayout->addLayout(profitRow);

    // Separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    previewLayout->addWidget(sep);

    previewLayout->addWidget(new QLabel(tr("Required Ingredients:")));
    m_previewIngredients = new QLabel("-");
    m_previewIngredients->setWordWrap(true);
    m_previewIngredients->setStyleSheet("color: #666;");
    previewLayout->addWidget(m_previewIngredients);

    previewLayout->addStretch();

    topSplitter->addWidget(previewGroup);
    topSplitter->setSizes({500, 300});

    mainLayout->addWidget(topSplitter);

    // Summary panel
    mainLayout->addWidget(createSummaryPanel());

    // History table
    auto *historyGroup = new QGroupBox(tr("Production History"));
    auto *historyLayout = new QVBoxLayout(historyGroup);

    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(8);
    m_historyTable->setHorizontalHeaderLabels({
        tr("Date/Time"), tr("Recipe"), tr("Building"), tr("Runs"),
        tr("Output"), tr("Value"), tr("Inv. Updated"), tr("Notes")
    });
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setSortingEnabled(true);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->verticalHeader()->setVisible(false);

    auto *header = m_historyTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Date
    header->setSectionResizeMode(1, QHeaderView::Stretch);           // Recipe
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Building
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // Runs
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);  // Output
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);  // Value
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);  // Inv Updated
    header->setSectionResizeMode(7, QHeaderView::Stretch);           // Notes

    connect(m_historyTable, &QTableWidget::itemSelectionChanged,
            this, &ProductionLogTab::onSelectionChanged);

    historyLayout->addWidget(m_historyTable);

    // History buttons
    auto *historyBtnLayout = new QHBoxLayout();
    historyBtnLayout->addStretch();
    m_deleteBtn = new QPushButton(tr("Delete Selected"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ProductionLogTab::onDeleteRun);
    historyBtnLayout->addWidget(m_deleteBtn);
    historyLayout->addLayout(historyBtnLayout);

    mainLayout->addWidget(historyGroup, 1);
}

QWidget* ProductionLogTab::createInputPanel()
{
    auto *group = new QGroupBox(tr("Log Production Run"));
    auto *layout = new QVBoxLayout(group);

    auto *formLayout = new QFormLayout();

    // Workbench/Building
    m_workbenchCombo = new QComboBox();
    connect(m_workbenchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProductionLogTab::onWorkbenchChanged);
    formLayout->addRow(tr("Building:"), m_workbenchCombo);

    // Recipe
    m_recipeCombo = new QComboBox();
    m_recipeCombo->setMinimumWidth(250);
    connect(m_recipeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProductionLogTab::onRecipeChanged);
    formLayout->addRow(tr("Recipe:"), m_recipeCombo);

    // Quantity (number of runs)
    m_quantitySpin = new QSpinBox();
    m_quantitySpin->setRange(1, 9999);
    m_quantitySpin->setValue(1);
    connect(m_quantitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ProductionLogTab::updatePreview);
    formLayout->addRow(tr("Runs:"), m_quantitySpin);

    // Date/Time
    m_dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    m_dateTimeEdit->setCalendarPopup(true);
    m_dateTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
    formLayout->addRow(tr("Date/Time:"), m_dateTimeEdit);

    // Notes
    m_notesEdit = new QLineEdit();
    m_notesEdit->setPlaceholderText(tr("Optional notes..."));
    formLayout->addRow(tr("Notes:"), m_notesEdit);

    layout->addLayout(formLayout);

    // Inventory options
    auto *invGroup = new QGroupBox(tr("Inventory Integration"));
    auto *invLayout = new QVBoxLayout(invGroup);

    m_deductInputsCheck = new QCheckBox(tr("Deduct inputs from Inventory"));
    m_deductInputsCheck->setChecked(true);
    m_deductInputsCheck->setToolTip(tr("Subtract ingredient quantities from inventory"));
    invLayout->addWidget(m_deductInputsCheck);

    m_addOutputsCheck = new QCheckBox(tr("Add outputs to Inventory"));
    m_addOutputsCheck->setChecked(true);
    m_addOutputsCheck->setToolTip(tr("Add produced items to inventory"));
    invLayout->addWidget(m_addOutputsCheck);

    layout->addWidget(invGroup);

    // Log button
    m_logBtn = new QPushButton(tr("Log Production"));
    m_logBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold; padding: 10px;");
    connect(m_logBtn, &QPushButton::clicked, this, &ProductionLogTab::onLogProduction);
    layout->addWidget(m_logBtn);

    layout->addStretch();

    return group;
}

QWidget* ProductionLogTab::createSummaryPanel()
{
    auto *group = new QGroupBox(tr("Production Summary"));
    auto *layout = new QHBoxLayout(group);

    // Total Runs
    auto *runsLayout = new QVBoxLayout();
    runsLayout->addWidget(new QLabel(tr("Total Runs")));
    m_totalRunsLabel = new QLabel("0");
    m_totalRunsLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    runsLayout->addWidget(m_totalRunsLabel);
    layout->addLayout(runsLayout);

    // Separator
    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::VLine);
    layout->addWidget(sep1);

    // Total Output
    auto *outputLayout = new QVBoxLayout();
    outputLayout->addWidget(new QLabel(tr("Total Items Produced")));
    m_totalOutputLabel = new QLabel("0");
    m_totalOutputLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    outputLayout->addWidget(m_totalOutputLabel);
    layout->addLayout(outputLayout);

    // Separator
    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::VLine);
    layout->addWidget(sep2);

    // Total Value Created
    auto *valueLayout = new QVBoxLayout();
    valueLayout->addWidget(new QLabel(tr("Total Value Created")));
    m_totalValueLabel = new QLabel("$0");
    m_totalValueLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2e7d32;");
    valueLayout->addWidget(m_totalValueLabel);
    layout->addLayout(valueLayout);

    layout->addStretch();

    return group;
}

// =============================================================================
// Data Loading
// =============================================================================

void ProductionLogTab::refreshData()
{
    populateWorkbenches();
    loadHistory();
    updateSummary();
}

void ProductionLogTab::populateWorkbenches()
{
    m_workbenchCombo->blockSignals(true);
    m_workbenchCombo->clear();

    m_workbenches = m_database->getAllWorkbenches();
    m_workbenchCombo->addItem(tr("All Buildings"));
    for (const auto &wb : m_workbenches) {
        m_workbenchCombo->addItem(wb.name, wb.id.value_or(0));
    }

    m_workbenchCombo->blockSignals(false);

    // Load all recipes
    m_recipes = m_database->getAllRecipes();
    populateRecipes();
}

void ProductionLogTab::populateRecipes()
{
    m_recipeCombo->blockSignals(true);
    m_recipeCombo->clear();

    int selectedWorkbenchId = m_workbenchCombo->currentData().toInt();
    m_filteredRecipes.clear();

    for (const auto &recipe : m_recipes) {
        // Filter by workbench if selected
        if (selectedWorkbenchId > 0 && recipe.workbenchId != selectedWorkbenchId) {
            continue;
        }

        m_filteredRecipes.append(recipe);

        QString displayName = recipe.outputItem;
        if (recipe.outputQty > 1) {
            displayName += QString(" (×%1)").arg(recipe.outputQty);
        }
        if (!recipe.notes.isEmpty()) {
            displayName += QString(" - %1").arg(recipe.notes);
        }

        m_recipeCombo->addItem(displayName, recipe.id.value_or(0));
    }

    m_recipeCombo->blockSignals(false);
    updatePreview();
}

void ProductionLogTab::loadHistory()
{
    m_runs = m_database->getAllProductionRuns();

    m_historyTable->setSortingEnabled(false);
    m_historyTable->setRowCount(m_runs.size());

    for (int row = 0; row < m_runs.size(); ++row) {
        const auto &run = m_runs[row];

        // Get recipe for cost calculation
        auto recipe = m_database->getRecipe(run.recipeId);
        double inputCost = 0, outputValue = 0;
        if (recipe.has_value()) {
            inputCost = calculateInputCost(recipe.value()) * run.quantity;
            outputValue = calculateOutputValue(recipe.value()) * run.quantity;
        }

        // Date/Time
        auto *dateItem = new QTableWidgetItem(run.timestamp.toString("yyyy-MM-dd hh:mm"));
        dateItem->setData(Qt::UserRole, run.id.value_or(0));
        m_historyTable->setItem(row, 0, dateItem);

        // Recipe
        m_historyTable->setItem(row, 1, new QTableWidgetItem(run.recipeName));

        // Building
        m_historyTable->setItem(row, 2, new QTableWidgetItem(run.workbenchName));

        // Runs
        auto *runsItem = new QTableWidgetItem(QString::number(run.quantity));
        runsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_historyTable->setItem(row, 3, runsItem);

        // Output
        int totalOutput = run.outputQty * run.quantity;
        auto *outputItem = new QTableWidgetItem(QString("%L1").arg(totalOutput));
        outputItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_historyTable->setItem(row, 4, outputItem);

        // Value
        auto *valueItem = new QTableWidgetItem(QString("$%L1").arg(outputValue, 0, 'f', 2));
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valueItem->setForeground(QColor("#2e7d32"));
        m_historyTable->setItem(row, 5, valueItem);

        // Inventory Updated
        QString invStatus;
        if (run.deductedInputs && run.addedOutputs) {
            invStatus = tr("✓ Both");
        } else if (run.deductedInputs) {
            invStatus = tr("↓ Inputs");
        } else if (run.addedOutputs) {
            invStatus = tr("↑ Outputs");
        } else {
            invStatus = tr("—");
        }
        auto *invItem = new QTableWidgetItem(invStatus);
        invItem->setTextAlignment(Qt::AlignCenter);
        m_historyTable->setItem(row, 6, invItem);

        // Notes
        m_historyTable->setItem(row, 7, new QTableWidgetItem(run.notes));
    }

    m_historyTable->setSortingEnabled(true);
}

void ProductionLogTab::updatePreview()
{
    int recipeIdx = m_recipeCombo->currentIndex();
    if (recipeIdx < 0 || recipeIdx >= m_filteredRecipes.size()) {
        m_previewInputCost->setText("$0");
        m_previewOutputValue->setText("$0");
        m_previewProfit->setText("$0");
        m_previewIngredients->setText("-");
        return;
    }

    const auto &recipe = m_filteredRecipes[recipeIdx];
    int runs = m_quantitySpin->value();

    double inputCost = calculateInputCost(recipe) * runs;
    double outputValue = calculateOutputValue(recipe) * runs;
    double profit = outputValue - inputCost;

    m_previewInputCost->setText(QString("$%L1").arg(inputCost, 0, 'f', 2));
    m_previewOutputValue->setText(QString("$%L1").arg(outputValue, 0, 'f', 2));
    m_previewProfit->setText(QString("$%L1").arg(profit, 0, 'f', 2));

    if (profit > 0) {
        m_previewProfit->setStyleSheet("font-weight: bold; font-size: 14px; color: #2e7d32;");
    } else if (profit < 0) {
        m_previewProfit->setStyleSheet("font-weight: bold; font-size: 14px; color: #c62828;");
    } else {
        m_previewProfit->setStyleSheet("font-weight: bold; font-size: 14px;");
    }

    // Build ingredients list
    QStringList ingredients;
    for (const auto &ing : recipe.ingredients) {
        int needed = ing.quantity * runs;
        int inStock = m_inventoryTab ? m_inventoryTab->getItemQuantity(ing.itemName) : 0;

        QString status;
        if (inStock >= needed) {
            status = QString("✓ %1 × %2 (have %3)").arg(ing.itemName).arg(needed).arg(inStock);
        } else if (inStock > 0) {
            status = QString("⚠ %1 × %2 (have %3, need %4 more)")
                         .arg(ing.itemName).arg(needed).arg(inStock).arg(needed - inStock);
        } else {
            status = QString("✗ %1 × %2 (none in stock)").arg(ing.itemName).arg(needed);
        }
        ingredients << status;
    }
    m_previewIngredients->setText(ingredients.join("\n"));
}

void ProductionLogTab::updateSummary()
{
    int totalRuns = 0;
    int totalOutput = 0;
    double totalValue = 0;

    for (const auto &run : m_runs) {
        totalRuns += run.quantity;
        totalOutput += run.outputQty * run.quantity;

        auto recipe = m_database->getRecipe(run.recipeId);
        if (recipe.has_value()) {
            totalValue += calculateOutputValue(recipe.value()) * run.quantity;
        }
    }

    m_totalRunsLabel->setText(QString("%L1").arg(totalRuns));
    m_totalOutputLabel->setText(QString("%L1").arg(totalOutput));
    m_totalValueLabel->setText(QString("$%L1").arg(totalValue, 0, 'f', 2));
}

// =============================================================================
// Slots
// =============================================================================

void ProductionLogTab::onWorkbenchChanged()
{
    populateRecipes();
}

void ProductionLogTab::onRecipeChanged()
{
    updatePreview();
}

void ProductionLogTab::onLogProduction()
{
    int recipeIdx = m_recipeCombo->currentIndex();
    if (recipeIdx < 0 || recipeIdx >= m_filteredRecipes.size()) {
        QMessageBox::warning(this, tr("No Recipe"), tr("Please select a recipe."));
        return;
    }

    const auto &recipe = m_filteredRecipes[recipeIdx];
    int runs = m_quantitySpin->value();
    bool deductInputs = m_deductInputsCheck->isChecked();
    bool addOutputs = m_addOutputsCheck->isChecked();

    // Confirm if inventory operations are enabled
    if (deductInputs || addOutputs) {
        QString msg = tr("Log %1 run(s) of %2?").arg(runs).arg(recipe.outputItem);
        if (deductInputs) {
            msg += tr("\n\nIngredients will be deducted from inventory.");
        }
        if (addOutputs) {
            msg += tr("\nOutput items will be added to inventory.");
        }

        auto result = QMessageBox::question(this, tr("Confirm Production"), msg);
        if (result != QMessageBox::Yes) {
            return;
        }
    }

    // Perform inventory operations
    bool inputsDeducted = false;
    bool outputsAdded = false;

    if (deductInputs && m_inventoryTab) {
        inputsDeducted = deductInputsFromInventory(recipe, runs);
        if (!inputsDeducted) {
            QMessageBox::warning(this, tr("Inventory Error"),
                                 tr("Could not deduct all inputs from inventory.\n"
                                    "Some items may have insufficient stock."));
        }
    }

    if (addOutputs && m_inventoryTab) {
        outputsAdded = addOutputsToInventory(recipe, runs);
    }

    // Create production run record
    Frontier::ProductionRun run;
    run.recipeId = recipe.id.value_or(0);
    run.quantity = runs;
    run.timestamp = m_dateTimeEdit->dateTime();
    run.deductedInputs = inputsDeducted;
    run.addedOutputs = outputsAdded;
    run.notes = m_notesEdit->text();

    int runId = m_database->addProductionRun(run);
    if (runId > 0) {
        // Reset inputs
        m_quantitySpin->setValue(1);
        m_dateTimeEdit->setDateTime(QDateTime::currentDateTime());
        m_notesEdit->clear();

        // Refresh
        loadHistory();
        updateSummary();
        updatePreview();

        emit productionLogged();

        QMessageBox::information(this, tr("Production Logged"),
                                 tr("Successfully logged %1 run(s) of %2.\n"
                                    "Total output: %3 items")
                                     .arg(runs)
                                     .arg(recipe.outputItem)
                                     .arg(recipe.outputQty * runs));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to log production run."));
    }
}

void ProductionLogTab::onDeleteRun()
{
    auto selected = m_historyTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int row = selected.first()->row();
    int runId = m_historyTable->item(row, 0)->data(Qt::UserRole).toInt();

    auto result = QMessageBox::question(this, tr("Delete Run"),
                                        tr("Delete this production run from history?\n\n"
                                           "Note: This will NOT reverse any inventory changes that were made."));

    if (result == QMessageBox::Yes) {
        if (m_database->deleteProductionRun(runId)) {
            loadHistory();
            updateSummary();
        }
    }
}

void ProductionLogTab::onSelectionChanged()
{
    bool hasSelection = !m_historyTable->selectedItems().isEmpty();
    m_deleteBtn->setEnabled(hasSelection);
}

// =============================================================================
// Inventory Operations
// =============================================================================

bool ProductionLogTab::deductInputsFromInventory(const Frontier::Recipe &recipe, int runs)
{
    if (!m_inventoryTab) return false;

    bool allSuccess = true;
    for (const auto &ing : recipe.ingredients) {
        int needed = ing.quantity * runs;
        bool success = m_inventoryTab->adjustItemQuantity(ing.itemName, -needed, false);
        if (!success) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool ProductionLogTab::addOutputsToInventory(const Frontier::Recipe &recipe, int runs)
{
    if (!m_inventoryTab) return false;

    int outputQty = recipe.outputQty * runs;

    // Find the item to get its ID
    auto item = m_database->getItemByName(recipe.outputItem);
    if (item.has_value() && item->id.has_value()) {
        return m_inventoryTab->addOrUpdateItem(item->id.value(), outputQty);
    }
    return false;
}

double ProductionLogTab::calculateInputCost(const Frontier::Recipe &recipe) const
{
    double cost = 0;
    for (const auto &ing : recipe.ingredients) {
        auto item = m_database->getItemByName(ing.itemName);
        if (item.has_value()) {
            cost += item->buyPriceInternal * ing.quantity;
        }
    }
    return cost;
}

double ProductionLogTab::calculateOutputValue(const Frontier::Recipe &recipe) const
{
    auto item = m_database->getItemByName(recipe.outputItem);
    if (item.has_value()) {
        return item->sellPriceInternal * recipe.outputQty;
    }
    return 0;
}
