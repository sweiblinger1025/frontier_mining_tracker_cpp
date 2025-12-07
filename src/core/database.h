/**
 * @file database.h
 * @brief Database access layer for Frontier Mining Tracker
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <optional>

#include "types.h"

namespace Frontier {

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    // Database connection
    bool connect(const QString &dbPath);
    bool initialize(const QString &dbPath);  // Alias for connect + createTables
    void close();
    bool isOpen() const;
    QString lastError() const;

    // Table creation (public for initial setup)
    bool createTables();

    // === Item CRUD ===
    bool addItem(const Item &item);
    std::optional<Item> getItem(int id);
    std::optional<Item> getItemByCode(const QString &code);
    std::optional<Item> getItemByName(const QString &name);
    QVector<Item> getAllItems();
    QVector<QString> getAllCategories();
    QVector<QString> getAllMainCategories();
    QVector<QString> getSubCategoriesFor(const QString &mainCategory);
    QVector<Item> getItemsByCategory(const QString &category);
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
    std::optional<Vehicle> getVehicle(const QString &id);
    QVector<Vehicle> getAllVehicles(bool activeOnly = false);
    bool updateVehicle(const Vehicle &vehicle);
    bool deleteVehicle(const QString &id);

    // === Operations Tables ===
    bool createOperationsTables();

    // === Fuel Log CRUD ===
    bool addFuelLogEntry(const FuelLogEntry &entry);
    QVector<FuelLogEntry> getFuelLog(const QDateTime &from, const QDateTime &to,
                                     const QString &equipmentId = QString());
    double getTotalFuelInRange(const QDateTime &from, const QDateTime &to);
    double getTotalFuelCostInRange(const QDateTime &from, const QDateTime &to);

    // === Movement Session CRUD ===
    int addMovementSession(const MovementSession &session);
    std::optional<MovementSession> getMovementSession(int id);
    QVector<MovementSession> getAllMovementSessions();
    bool updateMovementSession(const MovementSession &session);
    bool deleteMovementSession(int id);

    // === Movement Equipment Usage CRUD ===
    bool addOrUpdateEquipmentUsage(const MovementEquipmentUsage &usage);
    QVector<MovementEquipmentUsage> getEquipmentUsageForSession(int sessionId);
    bool deleteEquipmentUsage(int id);
    bool deleteEquipmentUsageForSession(int sessionId);

private:
    bool createVehiclesTable();

    QString m_connectionName;
    QString m_lastError;
};

// Helper functions for enum conversion
QString pricingGroupToString(PricingGroup group);
PricingGroup stringToPricingGroup(const QString &str);
QString transactionTypeToString(TransactionType type);
TransactionType stringToTransactionType(const QString &str);
QString accountTypeToString(AccountType type);
AccountType stringToAccountType(const QString &str);

} // namespace Frontier

#endif // DATABASE_H
