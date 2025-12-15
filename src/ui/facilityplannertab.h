/**
 * @file facilityplannertab.h
 * @brief Facility Planner - Plan building and power infrastructure purchases
 */

#ifndef FACILITYPLANNERTAB_H
#define FACILITYPLANNERTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

#include "core/types.h"

namespace Frontier {
class Database;
}

class FacilityPlannerTab : public QWidget
{
    Q_OBJECT

public:
    explicit FacilityPlannerTab(Frontier::Database *database, QWidget *parent = nullptr);

signals:
    void planChanged();

public slots:
    void refreshData();

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onClearAllClicked();
    void onQuantityChanged(int row, int newQty);
    void onBuildingSelected(const QString &buildingName);
    void onCategoryFilterChanged();

private:
    void setupUi();
    void loadPlan();
    void updateSummary();
    void populateBuildingCombo();

    Frontier::Database *m_database;

    // Add building section
    QComboBox *m_categoryFilterCombo;
    QComboBox *m_buildingCombo;
    QSpinBox *m_quantitySpin;
    QLabel *m_unitPriceLabel;
    QLabel *m_powerLabel;
    QLabel *m_lineTotalLabel;
    QPushButton *m_addBtn;

    // Plan table
    QTableWidget *m_table;

    // Actions
    QPushButton *m_removeBtn;
    QPushButton *m_clearAllBtn;

    // Summary
    QLabel *m_itemCountLabel;
    QLabel *m_totalCostLabel;
    QLabel *m_totalPowerLabel;
    QLabel *m_totalGeneratedLabel;
    QLabel *m_powerBalanceLabel;
};

#endif // FACILITYPLANNERTAB_H
