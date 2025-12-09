/**
 * @file inventorytab.h
 * @brief Inventory management tab for Operations
 */

#ifndef INVENTORYTAB_H
#define INVENTORYTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QSpinBox>
#include <QTextEdit>
#include <QFrame>

#include "core/types.h"

namespace Frontier {
class Database;
}

class InventoryTab : public QWidget
{
    Q_OBJECT

public:
    explicit InventoryTab(Frontier::Database *database, QWidget *parent = nullptr);

    // Public API for other tabs (e.g., Production Log)
    bool adjustItemQuantity(const QString &itemName, int delta, bool trackOil = true);
    bool addOrUpdateItem(int itemId, int quantity, std::optional<int> locationId = std::nullopt);
    int getItemQuantity(const QString &itemName) const;

public slots:
    void refreshData();

signals:
    void dataChanged();

private slots:
    // Table actions
    void onAddItem();
    void onEditItem();
    void onAdjustQuantity();
    void onDeleteItem();
    void onSelectionChanged();

    // Filters
    void onFilterChanged();

    // Oil tracking
    void onOilCapChanged(int value);
    void onResetOilCounter();

    // Sync from ledger
    void onSyncFromLedger();

private:
    void setupUi();
    QWidget* createSummaryPanel();
    QWidget* createFilterPanel();
    QWidget* createDetailsPanel();
    QWidget* createOilTracker();
    QFrame* createSeparator();

    void loadInventory();
    void applyFilters();
    void populateTable();
    void updateSummary();
    void updateOilTracker();
    void updateDetails();

    QString getStockStatus(int quantity) const;
    QColor getStatusColor(const QString &status) const;

    Frontier::Database *m_database;

    // Data
    QVector<Frontier::InventoryItem> m_allItems;
    QVector<Frontier::InventoryItem> m_filteredItems;
    Frontier::OilTracking m_oilTracking;

    // Summary labels
    QLabel *m_totalValueLabel;
    QLabel *m_itemsCountLabel;
    QLabel *m_locationsCountLabel;
    QLabel *m_lowStockLabel;
    QLabel *m_oilStatusLabel;

    // Filters
    QLineEdit *m_searchEdit;
    QComboBox *m_categoryCombo;
    QComboBox *m_locationCombo;
    QComboBox *m_statusCombo;
    QCheckBox *m_showZeroCheck;

    // Table
    QTableWidget *m_table;

    // Buttons
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_adjustBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_syncBtn;

    // Details panel
    QTextEdit *m_detailsText;

    // Oil tracker
    QLabel *m_oilWarningLabel;
    QLabel *m_oilInventoryLabel;
    QLabel *m_oilSoldLabel;
    QLabel *m_oilRemainingLabel;
    QSpinBox *m_oilCapSpin;
    QProgressBar *m_oilProgress;
    QLabel *m_oilProgressText;
    QLabel *m_oilRevenueLabel;
    QLabel *m_oilCompanyLabel;
    QLabel *m_oilPersonalLabel;

    // Constants
    static constexpr int STOCK_LOW = 10;
    static constexpr int STOCK_GOOD = 100;
    static constexpr double COMPANY_SPLIT = 0.90;
    static constexpr double PERSONAL_SPLIT = 0.10;
};

#endif // INVENTORYTAB_H
