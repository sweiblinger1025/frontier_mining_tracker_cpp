/**
 * @file productionlogtab.h
 * @brief Production Log - track production runs with inventory integration
 */

#ifndef PRODUCTIONLOGTAB_H
#define PRODUCTIONLOGTAB_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QDateTimeEdit>
#include <QLineEdit>

#include "core/types.h"

namespace Frontier {
class Database;
}

class InventoryTab;

class ProductionLogTab : public QWidget
{
    Q_OBJECT

public:
    explicit ProductionLogTab(Frontier::Database *database,
                              InventoryTab *inventoryTab = nullptr,
                              QWidget *parent = nullptr);

public slots:
    void refreshData();

signals:
    void productionLogged();

private slots:
    void onWorkbenchChanged();
    void onRecipeChanged();
    void onLogProduction();
    void onDeleteRun();
    void onSelectionChanged();

private:
    void setupUi();
    QWidget* createInputPanel();
    QWidget* createSummaryPanel();

    void populateWorkbenches();
    void populateRecipes();
    void loadHistory();
    void updatePreview();
    void updateSummary();

    bool deductInputsFromInventory(const Frontier::Recipe &recipe, int runs);
    bool addOutputsToInventory(const Frontier::Recipe &recipe, int runs);
    double calculateInputCost(const Frontier::Recipe &recipe) const;
    double calculateOutputValue(const Frontier::Recipe &recipe) const;

    Frontier::Database *m_database;
    InventoryTab *m_inventoryTab;

    // Cached data
    QVector<Frontier::Workbench> m_workbenches;
    QVector<Frontier::Recipe> m_recipes;
    QVector<Frontier::Recipe> m_filteredRecipes;
    QVector<Frontier::ProductionRun> m_runs;

    // Input controls
    QComboBox *m_workbenchCombo;
    QComboBox *m_recipeCombo;
    QSpinBox *m_quantitySpin;
    QDateTimeEdit *m_dateTimeEdit;
    QCheckBox *m_deductInputsCheck;
    QCheckBox *m_addOutputsCheck;
    QLineEdit *m_notesEdit;
    QPushButton *m_logBtn;

    // Preview labels
    QLabel *m_previewInputCost;
    QLabel *m_previewOutputValue;
    QLabel *m_previewProfit;
    QLabel *m_previewIngredients;

    // Summary labels
    QLabel *m_totalRunsLabel;
    QLabel *m_totalOutputLabel;
    QLabel *m_totalValueLabel;

    // History table
    QTableWidget *m_historyTable;
    QPushButton *m_deleteBtn;
};

#endif // PRODUCTIONLOGTAB_H
