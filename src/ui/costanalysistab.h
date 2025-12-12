/**
 * @file costanalysistab.h
 * @brief Cost Analysis tab for Production - analyzes recipe profitability
 */

#ifndef COSTANALYSISTAB_H
#define COSTANALYSISTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QFrame>

#include "core/types.h"

namespace Frontier {
class Database;
}

/**
 * @brief Struct to hold calculated recipe profitability data
 */
struct RecipeProfitability {
    int recipeId = 0;
    QString outputItem;
    int outputQty = 1;
    QString workbenchName;
    QString notes;
    double inputCost = 0.0;
    double outputValue = 0.0;
    double profit = 0.0;
    double marginPercent = 0.0;

    // For ingredient details
    QVector<QPair<QString, int>> ingredients;  // name, quantity
};

class CostAnalysisTab : public QWidget
{
    Q_OBJECT

public:
    explicit CostAnalysisTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    void onFilterChanged();
    void onSelectionChanged();

private:
    void setupUi();
    QWidget* createPodiumPanel();
    QWidget* createFilterPanel();
    QWidget* createDetailsPanel();
    QFrame* createPodiumCard(const QString &title, const QString &color);

    void loadRecipeProfitability();
    void applyFilters();
    void populateTable();
    void updatePodium();
    void updateDetails();

    RecipeProfitability calculateRecipeProfitability(const Frontier::Recipe &recipe);

    Frontier::Database *m_database;

    // Data
    QVector<RecipeProfitability> m_allRecipes;
    QVector<RecipeProfitability> m_filteredRecipes;

    // Podium labels
    QLabel *m_firstPlaceName;
    QLabel *m_firstPlaceProfit;
    QLabel *m_firstPlaceMargin;

    QLabel *m_secondPlaceName;
    QLabel *m_secondPlaceProfit;
    QLabel *m_secondPlaceMargin;

    QLabel *m_thirdPlaceName;
    QLabel *m_thirdPlaceProfit;
    QLabel *m_thirdPlaceMargin;

    // Filters
    QComboBox *m_workbenchCombo;

    // Table
    QTableWidget *m_table;

    // Details panel
    QLabel *m_detailsTitle;
    QLabel *m_detailsWorkbench;
    QLabel *m_detailsInputCost;
    QLabel *m_detailsOutputValue;
    QLabel *m_detailsProfit;
    QLabel *m_detailsMargin;
    QLabel *m_detailsIngredients;
    QLabel *m_detailsNotes;
};

#endif // COSTANALYSISTAB_H
