/**
 * @file datahubwidget.h
 * @brief Data Hub main widget with subtabs
 */

#ifndef DATAHUBWIDGET_H
#define DATAHUBWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include "core/database.h"

// Forward declaration
class VehicleSpecsTab;

class DataHubWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataHubWidget(Frontier::Database *database, QWidget *parent = nullptr);
    ~DataHubWidget();

public slots:
    void refreshData();

private slots:
    void onCategoryFilterChanged(int index);
    void onSearchTextChanged(const QString &text);
    void onAddItemClicked();
    void onDeleteItemClicked();
    void onItemDoubleClicked(const QModelIndex &index);

private:
    void setupUi();
    QWidget* createItemsTab();
    void loadCategories();
    void loadItems();
    void applyFilters();

    Frontier::Database *m_database;

    // Main tab widget
    QTabWidget *m_subTabs;

    // Subtab widgets
    VehicleSpecsTab *m_vehicleSpecsTab;

    // Items tab UI elements
    QComboBox *m_categoryFilter;
    QLineEdit *m_searchBox;
    QTableView *m_tableView;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QLabel *m_itemCountLabel;

    // Items model
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_proxyModel;

    // Cached data
    QVector<Frontier::Item> m_items;
};

#endif // DATAHUBWIDGET_H
