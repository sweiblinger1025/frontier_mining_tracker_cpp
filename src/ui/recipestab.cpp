/**
 * @file recipestab.cpp
 * @brief Recipes tab implementation for Data Hub
 */

#include "recipestab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QPushButton>
#include <QSplitter>

RecipesTab::RecipesTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    loadRecipes();
}

void RecipesTab::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // === Filters ===
    QGroupBox *filterGroup = new QGroupBox("Filters");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);

    filterLayout->addWidget(new QLabel("Workbench:"));
    m_workbenchCombo = new QComboBox();
    m_workbenchCombo->addItem("All Workbenches", -1);
    filterLayout->addWidget(m_workbenchCombo);

    filterLayout->addSpacing(20);

    filterLayout->addWidget(new QLabel("Search:"));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search recipes...");
    m_searchEdit->setClearButtonEnabled(true);
    filterLayout->addWidget(m_searchEdit);

    filterLayout->addStretch();

    QPushButton *refreshBtn = new QPushButton("Refresh");
    filterLayout->addWidget(refreshBtn);

    mainLayout->addWidget(filterGroup);

    // === Main Content (Splitter) ===
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Left: Recipe table
    QWidget *tableWidget = new QWidget();
    QVBoxLayout *tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"Output", "Qty", "Workbench", "Inputs", "Notes"});

    m_tableView = new QTableView();
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setAlternatingRowColors(true);

    tableLayout->addWidget(m_tableView);

    splitter->addWidget(tableWidget);

    // Right: Details panel
    QWidget *detailsWidget = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsWidget);

    QGroupBox *detailsGroup = new QGroupBox("Recipe Details");
    QVBoxLayout *detailsInner = new QVBoxLayout(detailsGroup);

    QFormLayout *formLayout = new QFormLayout();

    m_outputLabel = new QLabel("-");
    m_outputLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    formLayout->addRow("Output:", m_outputLabel);

    m_workbenchLabel = new QLabel("-");
    formLayout->addRow("Workbench:", m_workbenchLabel);

    m_notesLabel = new QLabel("-");
    formLayout->addRow("Notes:", m_notesLabel);

    detailsInner->addLayout(formLayout);

    QLabel *ingredientsLabel = new QLabel("Ingredients:");
    ingredientsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    detailsInner->addWidget(ingredientsLabel);

    m_ingredientsText = new QTextEdit();
    m_ingredientsText->setReadOnly(true);
    m_ingredientsText->setMaximumHeight(200);
    detailsInner->addWidget(m_ingredientsText);

    detailsInner->addStretch();

    detailsLayout->addWidget(detailsGroup);
    detailsLayout->addStretch();

    splitter->addWidget(detailsWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // === Summary ===
    QGroupBox *summaryGroup = new QGroupBox("Summary");
    QHBoxLayout *summaryLayout = new QHBoxLayout(summaryGroup);

    m_totalWorkbenchesLabel = new QLabel("Workbenches: 0");
    summaryLayout->addWidget(m_totalWorkbenchesLabel);

    m_totalRecipesLabel = new QLabel("Total Recipes: 0");
    m_totalRecipesLabel->setStyleSheet("font-weight: bold;");
    summaryLayout->addWidget(m_totalRecipesLabel);

    summaryLayout->addStretch();

    mainLayout->addWidget(summaryGroup);

    // === Connections ===
    connect(m_workbenchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecipesTab::onFilterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &RecipesTab::onFilterChanged);
    connect(refreshBtn, &QPushButton::clicked,
            this, &RecipesTab::refreshData);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &RecipesTab::onRecipeSelected);
}

void RecipesTab::refreshData()
{
    // Reload workbench combo
    m_workbenchCombo->blockSignals(true);
    m_workbenchCombo->clear();
    m_workbenchCombo->addItem("All Workbenches", -1);

    auto workbenches = m_database->getAllWorkbenches();
    for (const auto &wb : workbenches) {
        m_workbenchCombo->addItem(wb.name, wb.id.value_or(-1));
    }

    m_workbenchCombo->blockSignals(false);

    loadRecipes();
}

void RecipesTab::loadRecipes()
{
    m_model->removeRows(0, m_model->rowCount());

    int workbenchId = m_workbenchCombo->currentData().toInt();
    QString searchText = m_searchEdit->text().trimmed().toLower();

    QVector<Frontier::Recipe> recipes;
    if (workbenchId > 0) {
        recipes = m_database->getRecipesByWorkbench(workbenchId);
    } else {
        recipes = m_database->getAllRecipes();
    }

    int displayedCount = 0;

    for (const auto &recipe : recipes) {
        // Apply search filter
        if (!searchText.isEmpty()) {
            bool matches = recipe.outputItem.toLower().contains(searchText)
            || recipe.workbenchName.toLower().contains(searchText)
                || recipe.notes.toLower().contains(searchText);

            // Also search ingredients
            if (!matches) {
                for (const auto &ing : recipe.ingredients) {
                    if (ing.itemName.toLower().contains(searchText)) {
                        matches = true;
                        break;
                    }
                }
            }

            if (!matches) continue;
        }

        QList<QStandardItem*> row;

        // Output
        QStandardItem *outputItem = new QStandardItem(recipe.outputItem);
        outputItem->setData(recipe.id.value_or(-1), Qt::UserRole);
        row << outputItem;

        // Output Qty
        QStandardItem *qtyItem = new QStandardItem(QString::number(recipe.outputQty));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        row << qtyItem;

        // Workbench
        row << new QStandardItem(recipe.workbenchName);

        // Inputs summary
        QStringList inputList;
        for (const auto &ing : recipe.ingredients) {
            inputList << QString("%1x %2").arg(ing.quantity).arg(ing.itemName);
        }
        row << new QStandardItem(inputList.join(", "));

        // Notes
        row << new QStandardItem(recipe.notes);

        m_model->appendRow(row);
        displayedCount++;
    }

    // Resize columns
    m_tableView->resizeColumnsToContents();

    // Update summary
    auto workbenches = m_database->getAllWorkbenches();
    m_totalWorkbenchesLabel->setText(QString("Workbenches: %1").arg(workbenches.size()));
    m_totalRecipesLabel->setText(QString("Total Recipes: %1").arg(displayedCount));
}

void RecipesTab::onFilterChanged()
{
    loadRecipes();
}

void RecipesTab::onRecipeSelected(const QModelIndex &index)
{
    if (!index.isValid()) {
        m_outputLabel->setText("-");
        m_workbenchLabel->setText("-");
        m_notesLabel->setText("-");
        m_ingredientsText->clear();
        return;
    }

    int recipeId = m_model->item(index.row(), 0)->data(Qt::UserRole).toInt();
    updateRecipeDetails(recipeId);
}

void RecipesTab::updateRecipeDetails(int recipeId)
{
    auto recipe = m_database->getRecipe(recipeId);
    if (!recipe.has_value()) {
        return;
    }

    m_outputLabel->setText(QString("%1 (x%2)").arg(recipe->outputItem).arg(recipe->outputQty));
    m_workbenchLabel->setText(recipe->workbenchName);
    m_notesLabel->setText(recipe->notes.isEmpty() ? "-" : recipe->notes);

    // Format ingredients
    QString ingredientsHtml = "<table cellspacing='5'>";
    for (const auto &ing : recipe->ingredients) {
        // Try to get item price
        auto item = m_database->getItemByName(ing.itemName);
        QString priceStr = "-";
        if (item.has_value()) {
            double price = item->buyPriceDisplay > 0 ? item->buyPriceDisplay : item->buyPriceInternal;
            priceStr = QString("$%1").arg(price, 0, 'f', 0);
        }

        ingredientsHtml += QString("<tr><td><b>%1x</b></td><td>%2</td><td style='color:gray;'>%3</td></tr>")
                               .arg(ing.quantity)
                               .arg(ing.itemName)
                               .arg(priceStr);
    }
    ingredientsHtml += "</table>";

    m_ingredientsText->setHtml(ingredientsHtml);
}
