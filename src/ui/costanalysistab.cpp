/**
 * @file costanalysistab.cpp
 * @brief Cost Analysis tab implementation
 */

#include "costanalysistab.h"
#include "core/database.h"
#include "core/types.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFrame>
#include <algorithm>

// =============================================================================
// Constructor & Setup
// =============================================================================

CostAnalysisTab::CostAnalysisTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void CostAnalysisTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Top: Podium
    mainLayout->addWidget(createPodiumPanel());

    // Filter bar
    mainLayout->addWidget(createFilterPanel());

    // Main content: splitter with table and details
    auto *splitter = new QSplitter(Qt::Horizontal);

    // Left: Table
    auto *tableWidget = new QWidget();
    auto *tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget();
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        tr("Recipe"), tr("Building"), tr("Input Cost"),
        tr("Output Value"), tr("Profit"), tr("Margin %"), tr("Notes")
    });
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);

    auto *header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);  // Recipe
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Building
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Input Cost
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // Output Value
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);  // Profit
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);  // Margin
    header->setSectionResizeMode(6, QHeaderView::Stretch);  // Notes

    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &CostAnalysisTab::onSelectionChanged);

    tableLayout->addWidget(m_table);
    splitter->addWidget(tableWidget);

    // Right: Details panel
    splitter->addWidget(createDetailsPanel());
    splitter->setSizes({700, 300});

    mainLayout->addWidget(splitter, 1);
}

QWidget* CostAnalysisTab::createPodiumPanel()
{
    auto *group = new QGroupBox(tr("Top Performers"));
    auto *layout = new QHBoxLayout(group);
    layout->setSpacing(20);

    // Second Place (left)
    auto *secondCard = createPodiumCard(tr("Runner Up"), "#C0C0C0");
    auto *secondLayout = qobject_cast<QVBoxLayout*>(secondCard->layout());
    m_secondPlaceName = new QLabel("-");
    m_secondPlaceName->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_secondPlaceName->setAlignment(Qt::AlignCenter);
    m_secondPlaceProfit = new QLabel("$0");
    m_secondPlaceProfit->setStyleSheet("font-size: 12px; color: #2e7d32;");
    m_secondPlaceProfit->setAlignment(Qt::AlignCenter);
    m_secondPlaceMargin = new QLabel("0%");
    m_secondPlaceMargin->setAlignment(Qt::AlignCenter);
    secondLayout->addWidget(m_secondPlaceName);
    secondLayout->addWidget(m_secondPlaceProfit);
    secondLayout->addWidget(m_secondPlaceMargin);
    layout->addWidget(secondCard);

    // First Place (center, larger)
    auto *firstCard = createPodiumCard(tr("Most Profitable"), "#FFD700");
    firstCard->setMinimumHeight(140);
    auto *firstLayout = qobject_cast<QVBoxLayout*>(firstCard->layout());
    m_firstPlaceName = new QLabel("-");
    m_firstPlaceName->setStyleSheet("font-size: 16px; font-weight: bold;");
    m_firstPlaceName->setAlignment(Qt::AlignCenter);
    m_firstPlaceProfit = new QLabel("$0");
    m_firstPlaceProfit->setStyleSheet("font-size: 14px; color: #2e7d32; font-weight: bold;");
    m_firstPlaceProfit->setAlignment(Qt::AlignCenter);
    m_firstPlaceMargin = new QLabel("0%");
    m_firstPlaceMargin->setStyleSheet("font-size: 12px;");
    m_firstPlaceMargin->setAlignment(Qt::AlignCenter);
    firstLayout->addWidget(m_firstPlaceName);
    firstLayout->addWidget(m_firstPlaceProfit);
    firstLayout->addWidget(m_firstPlaceMargin);
    layout->addWidget(firstCard);

    // Third Place (right)
    auto *thirdCard = createPodiumCard(tr("Third Place"), "#CD7F32");
    auto *thirdLayout = qobject_cast<QVBoxLayout*>(thirdCard->layout());
    m_thirdPlaceName = new QLabel("-");
    m_thirdPlaceName->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_thirdPlaceName->setAlignment(Qt::AlignCenter);
    m_thirdPlaceProfit = new QLabel("$0");
    m_thirdPlaceProfit->setStyleSheet("font-size: 12px; color: #2e7d32;");
    m_thirdPlaceProfit->setAlignment(Qt::AlignCenter);
    m_thirdPlaceMargin = new QLabel("0%");
    m_thirdPlaceMargin->setAlignment(Qt::AlignCenter);
    thirdLayout->addWidget(m_thirdPlaceName);
    thirdLayout->addWidget(m_thirdPlaceProfit);
    thirdLayout->addWidget(m_thirdPlaceMargin);
    layout->addWidget(thirdCard);

    return group;
}

QFrame* CostAnalysisTab::createPodiumCard(const QString &title, const QString &color)
{
    auto *card = new QFrame();
    card->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    card->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 8px; }").arg(color));
    card->setMinimumWidth(200);
    card->setMinimumHeight(120);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 10, 10, 10);

    auto *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 11px; color: #333;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    return card;
}

QWidget* CostAnalysisTab::createFilterPanel()
{
    auto *group = new QGroupBox(tr("Filters"));
    auto *layout = new QHBoxLayout(group);

    layout->addWidget(new QLabel(tr("Building:")));
    m_workbenchCombo = new QComboBox();
    m_workbenchCombo->addItem(tr("All Buildings"));
    connect(m_workbenchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CostAnalysisTab::onFilterChanged);
    layout->addWidget(m_workbenchCombo);

    layout->addStretch();

    // Summary label
    auto *summaryLabel = new QLabel();
    summaryLabel->setObjectName("summaryLabel");
    layout->addWidget(summaryLabel);

    return group;
}

QWidget* CostAnalysisTab::createDetailsPanel()
{
    auto *group = new QGroupBox(tr("Recipe Details"));
    auto *layout = new QVBoxLayout(group);

    m_detailsTitle = new QLabel(tr("Select a recipe to view details"));
    m_detailsTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #1976d2;");
    layout->addWidget(m_detailsTitle);

    auto *formLayout = new QVBoxLayout();
    formLayout->setSpacing(8);

    // Workbench
    auto *workbenchRow = new QHBoxLayout();
    workbenchRow->addWidget(new QLabel(tr("Building:")));
    m_detailsWorkbench = new QLabel("-");
    m_detailsWorkbench->setStyleSheet("font-weight: bold;");
    workbenchRow->addWidget(m_detailsWorkbench);
    workbenchRow->addStretch();
    formLayout->addLayout(workbenchRow);

    // Input Cost
    auto *inputRow = new QHBoxLayout();
    inputRow->addWidget(new QLabel(tr("Input Cost:")));
    m_detailsInputCost = new QLabel("$0");
    m_detailsInputCost->setStyleSheet("font-weight: bold; color: #c62828;");
    inputRow->addWidget(m_detailsInputCost);
    inputRow->addStretch();
    formLayout->addLayout(inputRow);

    // Output Value
    auto *outputRow = new QHBoxLayout();
    outputRow->addWidget(new QLabel(tr("Output Value:")));
    m_detailsOutputValue = new QLabel("$0");
    m_detailsOutputValue->setStyleSheet("font-weight: bold; color: #2e7d32;");
    outputRow->addWidget(m_detailsOutputValue);
    outputRow->addStretch();
    formLayout->addLayout(outputRow);

    // Profit
    auto *profitRow = new QHBoxLayout();
    profitRow->addWidget(new QLabel(tr("Profit:")));
    m_detailsProfit = new QLabel("$0");
    m_detailsProfit->setStyleSheet("font-weight: bold; font-size: 14px;");
    profitRow->addWidget(m_detailsProfit);
    profitRow->addStretch();
    formLayout->addLayout(profitRow);

    // Margin
    auto *marginRow = new QHBoxLayout();
    marginRow->addWidget(new QLabel(tr("Margin:")));
    m_detailsMargin = new QLabel("0%");
    m_detailsMargin->setStyleSheet("font-weight: bold;");
    marginRow->addWidget(m_detailsMargin);
    marginRow->addStretch();
    formLayout->addLayout(marginRow);

    layout->addLayout(formLayout);

    // Separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep);

    // Ingredients
    auto *ingredientsLabel = new QLabel(tr("Ingredients:"));
    ingredientsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    layout->addWidget(ingredientsLabel);

    m_detailsIngredients = new QLabel("-");
    m_detailsIngredients->setWordWrap(true);
    layout->addWidget(m_detailsIngredients);

    // Notes
    auto *notesLabel = new QLabel(tr("Notes:"));
    notesLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    layout->addWidget(notesLabel);

    m_detailsNotes = new QLabel("-");
    m_detailsNotes->setWordWrap(true);
    m_detailsNotes->setStyleSheet("color: #666;");
    layout->addWidget(m_detailsNotes);

    layout->addStretch();

    return group;
}

// =============================================================================
// Data Loading
// =============================================================================

void CostAnalysisTab::refreshData()
{
    // Populate workbench filter
    m_workbenchCombo->blockSignals(true);
    QString currentWorkbench = m_workbenchCombo->currentText();
    m_workbenchCombo->clear();
    m_workbenchCombo->addItem(tr("All Buildings"));

    auto workbenches = m_database->getAllWorkbenches();
    for (const auto &wb : workbenches) {
        m_workbenchCombo->addItem(wb.name, wb.id.value_or(0));
    }

    int idx = m_workbenchCombo->findText(currentWorkbench);
    if (idx >= 0) m_workbenchCombo->setCurrentIndex(idx);
    m_workbenchCombo->blockSignals(false);

    loadRecipeProfitability();
}

void CostAnalysisTab::loadRecipeProfitability()
{
    m_allRecipes.clear();

    auto recipes = m_database->getAllRecipes();
    for (const auto &recipe : recipes) {
        RecipeProfitability prof = calculateRecipeProfitability(recipe);
        m_allRecipes.append(prof);
    }

    // Sort by profit descending
    std::sort(m_allRecipes.begin(), m_allRecipes.end(),
              [](const RecipeProfitability &a, const RecipeProfitability &b) {
                  return a.profit > b.profit;
              });

    applyFilters();
    updatePodium();
}

RecipeProfitability CostAnalysisTab::calculateRecipeProfitability(const Frontier::Recipe &recipe)
{
    RecipeProfitability prof;
    prof.recipeId = recipe.id.value_or(0);
    prof.outputItem = recipe.outputItem;
    prof.outputQty = recipe.outputQty;
    prof.workbenchName = recipe.workbenchName;
    prof.notes = recipe.notes;

    // Calculate input cost from ingredients
    prof.inputCost = 0.0;
    for (const auto &ing : recipe.ingredients) {
        auto item = m_database->getItemByName(ing.itemName);
        if (item.has_value()) {
            // Use buy price (base price) for input cost
            prof.inputCost += item->buyPriceInternal * ing.quantity;
        }
        prof.ingredients.append({ing.itemName, ing.quantity});
    }

    // Calculate output value
    auto outputItem = m_database->getItemByName(recipe.outputItem);
    if (outputItem.has_value()) {
        // Use sell price internal for output value
        prof.outputValue = outputItem->sellPriceInternal * recipe.outputQty;
    }

    // Calculate profit and margin
    prof.profit = prof.outputValue - prof.inputCost;
    if (prof.inputCost > 0) {
        prof.marginPercent = (prof.profit / prof.inputCost) * 100.0;
    }

    return prof;
}

void CostAnalysisTab::applyFilters()
{
    QString workbenchFilter = m_workbenchCombo->currentText();

    m_filteredRecipes.clear();

    for (const auto &recipe : m_allRecipes) {
        if (workbenchFilter != tr("All Buildings") && recipe.workbenchName != workbenchFilter) {
            continue;
        }
        m_filteredRecipes.append(recipe);
    }

    populateTable();

    // Update summary
    auto *summaryLabel = findChild<QLabel*>("summaryLabel");
    if (summaryLabel) {
        summaryLabel->setText(tr("Showing %1 of %2 recipes")
                                  .arg(m_filteredRecipes.size())
                                  .arg(m_allRecipes.size()));
    }
}

void CostAnalysisTab::populateTable()
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(m_filteredRecipes.size());

    for (int row = 0; row < m_filteredRecipes.size(); ++row) {
        const auto &recipe = m_filteredRecipes[row];

        // Recipe name (with qty if > 1)
        QString recipeName = recipe.outputItem;
        if (recipe.outputQty > 1) {
            recipeName += QString(" (×%1)").arg(recipe.outputQty);
        }
        auto *nameItem = new QTableWidgetItem(recipeName);
        nameItem->setData(Qt::UserRole, recipe.recipeId);
        m_table->setItem(row, 0, nameItem);

        // Building
        m_table->setItem(row, 1, new QTableWidgetItem(recipe.workbenchName));

        // Input Cost
        auto *inputItem = new QTableWidgetItem(QString("$%L1").arg(recipe.inputCost, 0, 'f', 2));
        inputItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        inputItem->setData(Qt::UserRole, recipe.inputCost);
        m_table->setItem(row, 2, inputItem);

        // Output Value
        auto *outputItem = new QTableWidgetItem(QString("$%L1").arg(recipe.outputValue, 0, 'f', 2));
        outputItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        outputItem->setData(Qt::UserRole, recipe.outputValue);
        m_table->setItem(row, 3, outputItem);

        // Profit
        auto *profitItem = new QTableWidgetItem(QString("$%L1").arg(recipe.profit, 0, 'f', 2));
        profitItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        profitItem->setData(Qt::UserRole, recipe.profit);
        if (recipe.profit > 0) {
            profitItem->setForeground(QColor("#2e7d32"));
        } else if (recipe.profit < 0) {
            profitItem->setForeground(QColor("#c62828"));
        }
        m_table->setItem(row, 4, profitItem);

        // Margin %
        auto *marginItem = new QTableWidgetItem(QString("%1%").arg(recipe.marginPercent, 0, 'f', 1));
        marginItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        marginItem->setData(Qt::UserRole, recipe.marginPercent);
        if (recipe.marginPercent > 0) {
            marginItem->setForeground(QColor("#2e7d32"));
        } else if (recipe.marginPercent < 0) {
            marginItem->setForeground(QColor("#c62828"));
        }
        m_table->setItem(row, 5, marginItem);

        // Notes
        m_table->setItem(row, 6, new QTableWidgetItem(recipe.notes));

        // Row color based on profitability
        QColor rowColor;
        if (recipe.profit > 100) {
            rowColor = QColor(200, 230, 201);  // Light green - great profit
        } else if (recipe.profit > 0) {
            rowColor = QColor(255, 249, 196);  // Light yellow - small profit
        } else if (recipe.profit < 0) {
            rowColor = QColor(255, 205, 210);  // Light red - loss
        } else {
            rowColor = Qt::white;  // Break even
        }

        for (int col = 0; col < 7; ++col) {
            if (auto *item = m_table->item(row, col)) {
                item->setBackground(rowColor);
            }
        }
    }

    m_table->setSortingEnabled(true);
}

void CostAnalysisTab::updatePodium()
{
    // Sort all recipes by profit for podium (not filtered)
    QVector<RecipeProfitability> sorted = m_allRecipes;
    std::sort(sorted.begin(), sorted.end(),
              [](const RecipeProfitability &a, const RecipeProfitability &b) {
                  return a.profit > b.profit;
              });

    // First place
    if (sorted.size() >= 1) {
        const auto &first = sorted[0];
        m_firstPlaceName->setText(first.outputItem);
        m_firstPlaceProfit->setText(QString("$%L1").arg(first.profit, 0, 'f', 2));
        m_firstPlaceMargin->setText(QString("%1% margin").arg(first.marginPercent, 0, 'f', 1));
    } else {
        m_firstPlaceName->setText("-");
        m_firstPlaceProfit->setText("$0");
        m_firstPlaceMargin->setText("0%");
    }

    // Second place
    if (sorted.size() >= 2) {
        const auto &second = sorted[1];
        m_secondPlaceName->setText(second.outputItem);
        m_secondPlaceProfit->setText(QString("$%L1").arg(second.profit, 0, 'f', 2));
        m_secondPlaceMargin->setText(QString("%1% margin").arg(second.marginPercent, 0, 'f', 1));
    } else {
        m_secondPlaceName->setText("-");
        m_secondPlaceProfit->setText("$0");
        m_secondPlaceMargin->setText("0%");
    }

    // Third place
    if (sorted.size() >= 3) {
        const auto &third = sorted[2];
        m_thirdPlaceName->setText(third.outputItem);
        m_thirdPlaceProfit->setText(QString("$%L1").arg(third.profit, 0, 'f', 2));
        m_thirdPlaceMargin->setText(QString("%1% margin").arg(third.marginPercent, 0, 'f', 1));
    } else {
        m_thirdPlaceName->setText("-");
        m_thirdPlaceProfit->setText("$0");
        m_thirdPlaceMargin->setText("0%");
    }
}

void CostAnalysisTab::updateDetails()
{
    auto selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        m_detailsTitle->setText(tr("Select a recipe to view details"));
        m_detailsWorkbench->setText("-");
        m_detailsInputCost->setText("$0");
        m_detailsOutputValue->setText("$0");
        m_detailsProfit->setText("$0");
        m_detailsMargin->setText("0%");
        m_detailsIngredients->setText("-");
        m_detailsNotes->setText("-");
        return;
    }

    int row = selected.first()->row();
    if (row < 0 || row >= m_filteredRecipes.size()) {
        return;
    }

    const auto &recipe = m_filteredRecipes[row];

    // Title
    QString title = recipe.outputItem;
    if (recipe.outputQty > 1) {
        title += QString(" (×%1)").arg(recipe.outputQty);
    }
    m_detailsTitle->setText(title);

    // Workbench
    m_detailsWorkbench->setText(recipe.workbenchName);

    // Input Cost
    m_detailsInputCost->setText(QString("$%L1").arg(recipe.inputCost, 0, 'f', 2));

    // Output Value
    m_detailsOutputValue->setText(QString("$%L1").arg(recipe.outputValue, 0, 'f', 2));

    // Profit
    m_detailsProfit->setText(QString("$%L1").arg(recipe.profit, 0, 'f', 2));
    if (recipe.profit > 0) {
        m_detailsProfit->setStyleSheet("font-weight: bold; font-size: 14px; color: #2e7d32;");
    } else if (recipe.profit < 0) {
        m_detailsProfit->setStyleSheet("font-weight: bold; font-size: 14px; color: #c62828;");
    } else {
        m_detailsProfit->setStyleSheet("font-weight: bold; font-size: 14px;");
    }

    // Margin
    m_detailsMargin->setText(QString("%1%").arg(recipe.marginPercent, 0, 'f', 1));
    if (recipe.marginPercent > 0) {
        m_detailsMargin->setStyleSheet("font-weight: bold; color: #2e7d32;");
    } else if (recipe.marginPercent < 0) {
        m_detailsMargin->setStyleSheet("font-weight: bold; color: #c62828;");
    } else {
        m_detailsMargin->setStyleSheet("font-weight: bold;");
    }

    // Ingredients
    QStringList ingredientList;
    for (const auto &ing : recipe.ingredients) {
        ingredientList << QString("• %1 × %2").arg(ing.first).arg(ing.second);
    }
    m_detailsIngredients->setText(ingredientList.join("\n"));

    // Notes
    m_detailsNotes->setText(recipe.notes.isEmpty() ? "-" : recipe.notes);
}

// =============================================================================
// Slots
// =============================================================================

void CostAnalysisTab::onFilterChanged()
{
    applyFilters();
}

void CostAnalysisTab::onSelectionChanged()
{
    updateDetails();
}
