/**
 * @file datahubwidget.h
 * @brief Data Hub tab - Reference data management
 */

#ifndef DATAHUBWIDGET_H
#define DATAHUBWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "core/database.h"

class DataHubWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DataHubWidget(Frontier::Database *database, QWidget *parent = nullptr);
    ~DataHubWidget();

    void refreshData();

private slots:
    void onCategoryFilterChanged(int index);
    void onSearchTextChanged(const QString &text);
    void onAddItemClicked();
    void onDeleteItemClicked();
    void onItemDoubleClicked(const QModelIndex &index);

private:
    void setupUi();
    void loadCategories();
    void loadItems();
    void applyFilters();

    // Database reference
    Frontier::Database *m_database;

    // UI Components
    QComboBox *m_categoryFilter;
    QLineEdit *m_searchBox;
    QTableView *m_tableView;
    QLabel *m_itemCountLabel;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;

    // Data Model
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_proxyModel;

    // Data cache
    QVector<Frontier::Item> m_items;
};

#endif // DATAHUBWIDGET_H
