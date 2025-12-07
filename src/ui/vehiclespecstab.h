/**
 * @file vehiclespecstab.h
 * @brief Vehicle Specs tab for Data Hub
 */

#ifndef VEHICLESPECSTAB_H
#define VEHICLESPECSTAB_H

#include <QWidget>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include "core/database.h"

class VehicleSpecsTab : public QWidget
{
    Q_OBJECT

public:
    explicit VehicleSpecsTab(Frontier::Database *database, QWidget *parent = nullptr);
    ~VehicleSpecsTab();

public slots:
    void refreshData();

private slots:
    void onCategoryFilterChanged(int index);
    void onSearchTextChanged(const QString &text);
    void onVehicleDoubleClicked(const QModelIndex &index);

private:
    void setupUi();
    void loadCategories();
    void loadVehicles();
    void populateTable(const QVector<Frontier::Vehicle> &vehicles);

    Frontier::Database *m_database;

    // UI elements
    QComboBox *m_categoryFilter;
    QLineEdit *m_searchBox;
    QTableView *m_tableView;
    QLabel *m_vehicleCountLabel;

    // Model
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_proxyModel;

    // Cached data
    QVector<Frontier::Vehicle> m_vehicles;
};

#endif // VEHICLESPECSTAB_H
