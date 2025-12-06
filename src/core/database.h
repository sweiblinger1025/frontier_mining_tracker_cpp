/**
 * @file database.h
 * @brief SQLite database manager for Frontier Mining Tracker
 *
 * Handles all database operations including connection management,
 * table creation, and CRUD operations for all data types.
 *
 * @author Stephen
 * @date December 2025
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QVector>
#include "types.h"

namespace Frontier {

class Database {
public:
    Database();
    ~Database();

    // Connection management
    bool connect(const QString &path);
    void disconnect();
    bool isConnected() const;

    // Schema setup
    bool createTables();

    // Error handling
    QString lastError() const;

    // === Item CRUD ===
    bool addItem(const Item &item);
    std::optional<Item> getItem(int id);
    QVector<Item> getAllItems();
    QVector<Item> getItemsByCategory(const QString &categoryMain);
    bool updateItem(const Item &item);
    bool deleteItem(int id);

    // === Transaction CRUD ===
    bool addTransaction(const Transaction &transaction);
    std::optional<Transaction> getTransaction(int id);
    QVector<Transaction> getAllTransactions();
    QVector<Transaction> getTransactionsByDateRange(const QDate &from, const QDate &to);
    bool updateTransaction(const Transaction &transaction);
    bool deleteTransaction(int id);

    // === Vehicle CRUD ===
    bool addVehicle(const Vehicle &vehicle);
    std::optional<Vehicle> getVehicle(int id);
    QVector<Vehicle> getAllVehicles();
    bool updateVehicle(const Vehicle &vehicle);
    bool deleteVehicle(int id);

    // === Inventory CRUD ===
    bool addInventoryItem(const InventoryItem &item);
    std::optional<InventoryItem> getInventoryItem(int id);
    QVector<InventoryItem> getAllInventory();
    bool updateInventoryItem(const InventoryItem &item);
    bool deleteInventoryItem(int id);

private:
    QString m_connectionName;
    QString m_lastError;
    bool m_connected;

    // Helper to convert PricingGroup enum to/from string
    QString pricingGroupToString(PricingGroup group) const;
    PricingGroup stringToPricingGroup(const QString &str) const;
};

} // namespace Frontier

#endif // DATABASE_H
