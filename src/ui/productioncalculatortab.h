/**
 * @file productioncalculatortab.h
 * @brief Production Calculator - calculates material requirements for recipes
 */

#ifndef PRODUCTIONCALCULATORTAB_H
#define PRODUCTIONCALCULATORTAB_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTreeWidget>
#include <QMap>

#include "core/types.h"

namespace Frontier {
class Database;
}

class InventoryTab;

/**
 * @brief Represents a node in the production chain tree
 */
struct ProductionNode {
    QString itemName;
    int quantityNeeded = 0;
    int quantityInInventory = 0;
    bool isCraftable = false;
    bool isRawMaterial = false;  // Leaf node (no recipe or not expanding)
    double unitCost = 0.0;       // Buy price for raw materials
    QVector<ProductionNode> children;

    int shortfall() const {
        return qMax(0, quantityNeeded - quantityInInventory);
    }

    bool hasShortfall() const {
        return quantityNeeded > quantityInInventory;
    }
};

/**
 * @brief Summary of production requirements
 */
struct ProductionSummary {
    int totalMaterialTypes = 0;      // Unique raw materials
    int totalMaterialUnits = 0;      // Total raw material units
    double inputCost = 0.0;          // Cost to buy all raw materials
    double outputValue = 0.0;        // Value of output
    double profit = 0.0;
    double marginPercent = 0.0;
    QMap<QString, int> rawMaterials; // item name -> quantity needed
    QMap<QString, int> shortfalls;   // item name -> shortfall quantity
};

class ProductionCalculatorTab : public QWidget
{
    Q_OBJECT

public:
    explicit ProductionCalculatorTab(Frontier::Database *database,
                                     InventoryTab *inventoryTab = nullptr,
                                     QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    void onRecipeChanged();
    void onCalculate();

private:
    void setupUi();
    QWidget* createInputPanel();
    QWidget* createSummaryPanel();

    void populateRecipeCombo();
    void calculate();
    void buildProductionTree(ProductionNode &node, int quantity, bool expandChain,
                             QSet<QString> &visited);
    void collectRawMaterials(const ProductionNode &node, QMap<QString, int> &materials);
    void populateTreeWidget(const ProductionNode &node, QTreeWidgetItem *parent = nullptr);
    void updateSummary(const ProductionSummary &summary);
    void clearResults();

    int getInventoryQuantity(const QString &itemName) const;
    std::optional<Frontier::Recipe> findRecipeForItem(const QString &itemName) const;

    Frontier::Database *m_database;
    InventoryTab *m_inventoryTab;

    // Cached data
    QVector<Frontier::Recipe> m_recipes;
    QMap<QString, Frontier::Recipe> m_recipesByOutput;  // output item -> recipe

    // Input controls
    QComboBox *m_recipeCombo;
    QSpinBox *m_quantitySpin;
    QCheckBox *m_fullChainCheck;
    QPushButton *m_calculateBtn;

    // Summary labels
    QLabel *m_totalMaterialsLabel;
    QLabel *m_totalUnitsLabel;
    QLabel *m_inputCostLabel;
    QLabel *m_outputValueLabel;
    QLabel *m_profitLabel;
    QLabel *m_marginLabel;

    // Results tree
    QTreeWidget *m_treeWidget;

    // Current calculation result
    ProductionNode m_rootNode;
    ProductionSummary m_summary;
};

#endif // PRODUCTIONCALCULATORTAB_H
