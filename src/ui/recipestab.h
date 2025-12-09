/**
 * @file recipestab.h
 * @brief Recipes tab for Data Hub showing all crafting recipes
 */

#ifndef RECIPESTAB_H
#define RECIPESTAB_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTableView>
#include <QStandardItemModel>
#include <QLabel>
#include <QTextEdit>

namespace Frontier {
class Database;
}

class RecipesTab : public QWidget
{
    Q_OBJECT

public:
    explicit RecipesTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    void onFilterChanged();
    void onRecipeSelected(const QModelIndex &index);

private:
    void setupUi();
    void loadRecipes();
    void updateRecipeDetails(int recipeId);

    Frontier::Database *m_database;

    // Filters
    QComboBox *m_workbenchCombo;
    QLineEdit *m_searchEdit;

    // Table
    QTableView *m_tableView;
    QStandardItemModel *m_model;

    // Details panel
    QLabel *m_outputLabel;
    QLabel *m_workbenchLabel;
    QLabel *m_notesLabel;
    QTextEdit *m_ingredientsText;

    // Summary
    QLabel *m_totalRecipesLabel;
    QLabel *m_totalWorkbenchesLabel;
};

#endif // RECIPESTAB_H
