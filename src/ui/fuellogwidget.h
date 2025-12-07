/**
 * @file fuellogwidget.h
 * @brief Fuel & Fluids Log subtab for Operations
 */

#ifndef FUELLOGWIDGET_H
#define FUELLOGWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

#include "core/operationsmanager.h"

class FuelLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FuelLogWidget(Frontier::OperationsManager *manager, QWidget *parent = nullptr);
    ~FuelLogWidget();

private slots:
    void onRefreshClicked();
    void onFilterChanged();
    void onUnitSystemChanged(Frontier::UnitSystem system);
    void onFuelLogUpdated(const Frontier::FuelLogEntry &entry);

private:
    void setupUi();
    void loadEquipmentFilter();
    void loadFuelLog();
    void updateSummary();

    Frontier::OperationsManager *m_manager;

    // Filter controls
    QDateEdit *m_fromDateEdit;
    QDateEdit *m_toDateEdit;
    QComboBox *m_equipmentCombo;
    QPushButton *m_refreshButton;

    // Table
    QTableView *m_tableView;
    QStandardItemModel *m_model;

    // Summary
    QLabel *m_totalFuelLabel;
    QLabel *m_totalCostLabel;
    QLabel *m_entryCountLabel;
};

#endif // FUELLOGWIDGET_H
