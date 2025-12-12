/**
 * @file productiontab.h
 * @brief Production tab container with Calculator, Log, and Cost Analysis subtabs
 */

#ifndef PRODUCTIONTAB_H
#define PRODUCTIONTAB_H

#include <QWidget>
#include <QTabWidget>

namespace Frontier {
class Database;
}

class CostAnalysisTab;
class ProductionCalculatorTab;
class ProductionLogTab;
class InventoryTab;

class ProductionTab : public QWidget
{
    Q_OBJECT

public:
    explicit ProductionTab(Frontier::Database *database,
                           InventoryTab *inventoryTab = nullptr,
                           QWidget *parent = nullptr);

public slots:
    void refreshData();

private:
    void setupUi();

    Frontier::Database *m_database;
    InventoryTab *m_inventoryTab;  // For Production Log integration
    QTabWidget *m_subTabs;

    // Subtabs
    ProductionCalculatorTab *m_calculatorTab;
    ProductionLogTab *m_logTab;
    CostAnalysisTab *m_costAnalysisTab;
};

#endif // PRODUCTIONTAB_H
