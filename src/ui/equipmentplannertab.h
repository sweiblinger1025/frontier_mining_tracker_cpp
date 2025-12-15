/**
 * @file equipmentplannertab.h
 * @brief Equipment Planner - Plan equipment purchases from Items table
 */

#ifndef EQUIPMENTPLANNERTAB_H
#define EQUIPMENTPLANNERTAB_H

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

class EquipmentPlannerTab : public QWidget
{
    Q_OBJECT

public:
    explicit EquipmentPlannerTab(Frontier::Database *database, QWidget *parent = nullptr);

signals:
    void planChanged();

public slots:
    void refreshData();

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onClearAllClicked();
    void onQuantityChanged(int row, int newQty);
    void onItemSelected(const QString &itemName);

private:
    void setupUi();
    void loadPlan();
    void updateSummary();

    Frontier::Database *m_database;

    // Add item section
    QComboBox *m_itemCombo;
    QSpinBox *m_quantitySpin;
    QLabel *m_unitPriceLabel;
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
};

#endif // EQUIPMENTPLANNERTAB_H
