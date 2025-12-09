/**
 * @file operationswidget.h
 * @brief Operations tab with subtabs for equipment, fuel, material movement
 */

#ifndef OPERATIONSWIDGET_H
#define OPERATIONSWIDGET_H

#include <QWidget>
#include <QTabWidget>

#include "core/operationsmanager.h"

class InventoryTab;  // Forward declaration

class OperationsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OperationsWidget(Frontier::OperationsManager *manager, QWidget *parent = nullptr);
    ~OperationsWidget();

    // Accessor for Production Log integration
    InventoryTab* inventoryTab() const;

private:
    void setupUi();

    Frontier::OperationsManager *m_manager;
    QTabWidget *m_subTabs;
    InventoryTab *m_inventoryTab = nullptr;
};

#endif // OPERATIONSWIDGET_H
