/**
 * @file database.cpp
 * @brief Database access layer implementation for Frontier Mining Tracker
 */

#include "database.h"

#include <QDebug>
#include <QSqlError>
#include <QUuid>
#include <cmath>

namespace Frontier {

Database::Database(QObject *parent)
    : QObject(parent)
{
    // Generate unique connection name for this instance
    m_connectionName = QUuid::createUuid().toString();
}

Database::~Database()
{
    close();
}

bool Database::initialize(const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    // Create all tables
    if (!createTables()) {
        qWarning() << "Failed to create tables";
        return false;
    }

    return true;
}

void Database::close()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool Database::isOpen() const
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        return false;
    }
    return QSqlDatabase::database(m_connectionName).isOpen();
}

QString Database::lastError() const
{
    return m_lastError;
}

bool Database::createTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Items table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            category TEXT,
            buy_price REAL DEFAULT 0,
            sell_price_internal REAL DEFAULT 0,
            sell_price_display REAL DEFAULT 0,
            weight REAL DEFAULT 0,
            pricing_group TEXT DEFAULT 'Base70',
            notes TEXT
        )
    )")) {
        qWarning() << "Failed to create items table:" << query.lastError().text();
        return false;
    }

    // Transactions table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL,
            type TEXT NOT NULL,
            account TEXT NOT NULL,
            item_name TEXT,
            category TEXT,
            quantity INTEGER DEFAULT 1,
            unit_price REAL DEFAULT 0,
            total_amount REAL DEFAULT 0,
            notes TEXT
        )
    )")) {
        qWarning() << "Failed to create transactions table:" << query.lastError().text();
        return false;
    }

    // Vehicles table
    if (!createVehiclesTable()) {
        return false;
    }

    // Operations tables
    if (!createOperationsTables()) {
        return false;
    }

    // Recipe tables
    if (!createRecipeTables()) {
        return false;
    }

    // Location tables
    if (!createLocationTables()) {
        return false;
    }

    // Inventory tables
    if (!createInventoryTables()) {
        return false;
    }

    return true;
}

bool Database::createInventoryTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Inventory table - one record per item (global pool)
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS inventory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            item_id INTEGER NOT NULL UNIQUE,
            quantity INTEGER DEFAULT 0,
            location_id INTEGER,
            last_updated TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (item_id) REFERENCES items(id),
            FOREIGN KEY (location_id) REFERENCES locations(id)
        )
    )")) {
        qWarning() << "Failed to create inventory table:" << query.lastError().text();
        return false;
    }

    // Oil tracking table (single row for settings)
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS oil_tracking (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            oil_cap INTEGER DEFAULT 10000,
            total_oil_sold INTEGER DEFAULT 0,
            last_reset TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qWarning() << "Failed to create oil_tracking table:" << query.lastError().text();
        return false;
    }

    // Initialize oil tracking with default values if empty
    if (!query.exec("INSERT OR IGNORE INTO oil_tracking (id, oil_cap, total_oil_sold) VALUES (1, 10000, 0)")) {
        qWarning() << "Failed to initialize oil_tracking:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Database::createVehiclesTable()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS vehicles (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            category_main TEXT,
            category_sub TEXT,
            bucket_capacity_m3 REAL DEFAULT 0,
            truck_capacity_m3 REAL DEFAULT 0,
            tank_capacity_l REAL DEFAULT 0,
            fuel_use_l_per_hour REAL DEFAULT 0,
            purchase_price REAL DEFAULT 0,
            active INTEGER DEFAULT 1,
            notes TEXT
        )
    )")) {
        qWarning() << "Failed to create vehicles table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Database::createOperationsTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Fuel Log table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS fuel_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date_time TEXT NOT NULL,
            equipment_id TEXT,
            liters REAL DEFAULT 0,
            unit_price REAL DEFAULT 0,
            total_cost REAL DEFAULT 0,
            meter_or_hours REAL DEFAULT 0,
            source TEXT,
            notes TEXT,
            FOREIGN KEY (equipment_id) REFERENCES vehicles(id)
        )
    )")) {
        qWarning() << "Failed to create fuel_log table:" << query.lastError().text();
        return false;
    }

    // Movement Sessions table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS movement_sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            start_time TEXT NOT NULL,
            end_time TEXT,
            map_name TEXT,
            notes TEXT
        )
    )")) {
        qWarning() << "Failed to create movement_sessions table:" << query.lastError().text();
        return false;
    }

    // Movement Equipment Usage table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS movement_equipment_usage (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id INTEGER NOT NULL,
            equipment_id TEXT NOT NULL,
            role TEXT,
            hours_used REAL DEFAULT 0,
            buckets INTEGER DEFAULT 0,
            loads INTEGER DEFAULT 0,
            dumps INTEGER DEFAULT 0,
            estimated_fuel_l REAL DEFAULT 0,
            FOREIGN KEY (session_id) REFERENCES movement_sessions(id),
            FOREIGN KEY (equipment_id) REFERENCES vehicles(id)
        )
    )")) {
        qWarning() << "Failed to create movement_equipment_usage table:" << query.lastError().text();
        return false;
    }

    return true;
}

// =============================================================================
// Item CRUD
// =============================================================================

bool Database::addItem(const Item &item)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO items (code, name, category, buy_price, sell_price_internal,
                          sell_price_display, weight, pricing_group, notes)
        VALUES (:code, :name, :category, :buy_price, :sell_price_internal,
                :sell_price_display, :weight, :pricing_group, :notes)
    )");

    query.bindValue(":code", item.code);
    query.bindValue(":name", item.name);
    query.bindValue(":category", item.displayCategory());
    query.bindValue(":buy_price", item.buyPriceInternal);
    query.bindValue(":sell_price_internal", item.sellPriceInternal);
    query.bindValue(":sell_price_display", item.sellPriceDisplay);
    query.bindValue(":weight", item.weight);
    query.bindValue(":pricing_group", pricingGroupToString(item.pricingGroup));
    query.bindValue(":notes", item.notes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to add item:" << m_lastError;
        return false;
    }

    return true;
}

std::optional<Item> Database::getItem(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM items WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Item item;
    item.id = query.value("id").toInt();
    item.code = query.value("code").toString();
    item.name = query.value("name").toString();
    item.categoryMain = query.value("category").toString();
    item.buyPriceInternal = query.value("buy_price").toDouble();
    item.buyPriceDisplay = std::round(item.buyPriceInternal);
    item.sellPriceInternal = query.value("sell_price_internal").toDouble();
    item.sellPriceDisplay = query.value("sell_price_display").toDouble();
    item.weight = query.value("weight").toDouble();
    item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
    item.notes = query.value("notes").toString();

    return item;
}

std::optional<Item> Database::getItemByCode(const QString &code)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM items WHERE code = :code");
    query.bindValue(":code", code);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Item item;
    item.id = query.value("id").toInt();
    item.code = query.value("code").toString();
    item.name = query.value("name").toString();
    item.categoryMain = query.value("category").toString();
    item.buyPriceInternal = query.value("buy_price").toDouble();
    item.buyPriceDisplay = std::round(item.buyPriceInternal);
    item.sellPriceInternal = query.value("sell_price_internal").toDouble();
    item.sellPriceDisplay = query.value("sell_price_display").toDouble();
    item.weight = query.value("weight").toDouble();
    item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
    item.notes = query.value("notes").toString();

    return item;
}

QVector<Item> Database::getAllItems()
{
    QVector<Item> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM items ORDER BY category, name")) {
        qWarning() << "Failed to get items:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("id").toInt();
        item.code = query.value("code").toString();
        item.name = query.value("name").toString();
        item.categoryMain = query.value("category").toString();
        item.buyPriceInternal = query.value("buy_price").toDouble();
        item.buyPriceDisplay = std::round(item.buyPriceInternal);
        item.sellPriceInternal = query.value("sell_price_internal").toDouble();
        item.sellPriceDisplay = query.value("sell_price_display").toDouble();
        item.weight = query.value("weight").toDouble();
        item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
        item.notes = query.value("notes").toString();
        items.append(item);
    }

    return items;
}

QVector<QString> Database::getAllCategories()
{
    QVector<QString> categories;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT DISTINCT category FROM items ORDER BY category")) {
        qWarning() << "Failed to get categories:" << query.lastError().text();
        return categories;
    }

    while (query.next()) {
        QString category = query.value("category").toString();
        if (!category.isEmpty()) {
            categories.append(category);
        }
    }

    return categories;
}

QVector<Item> Database::getItemsByCategory(const QString &category)
{
    QVector<Item> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM items WHERE category = :category ORDER BY name");
    query.bindValue(":category", category);

    if (!query.exec()) {
        qWarning() << "Failed to get items by category:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("id").toInt();
        item.code = query.value("code").toString();
        item.name = query.value("name").toString();
        item.categoryMain = query.value("category").toString();
        item.buyPriceInternal = query.value("buy_price").toDouble();
        item.buyPriceDisplay = std::round(item.buyPriceInternal);
        item.sellPriceInternal = query.value("sell_price_internal").toDouble();
        item.sellPriceDisplay = query.value("sell_price_display").toDouble();
        item.weight = query.value("weight").toDouble();
        item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
        item.notes = query.value("notes").toString();
        items.append(item);
    }

    return items;
}

bool Database::updateItem(const Item &item)
{
    if (!item.id.has_value()) {
        m_lastError = "Cannot update item without id";
        qWarning() << m_lastError;
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE items SET
            code = :code,
            name = :name,
            category = :category,
            buy_price = :buy_price,
            sell_price_internal = :sell_price_internal,
            sell_price_display = :sell_price_display,
            weight = :weight,
            pricing_group = :pricing_group,
            notes = :notes
        WHERE id = :id
    )");

    query.bindValue(":id", item.id.value());
    query.bindValue(":code", item.code);
    query.bindValue(":name", item.name);
    query.bindValue(":category", item.displayCategory());
    query.bindValue(":buy_price", item.buyPriceInternal);
    query.bindValue(":sell_price_internal", item.sellPriceInternal);
    query.bindValue(":sell_price_display", item.sellPriceDisplay);
    query.bindValue(":weight", item.weight);
    query.bindValue(":pricing_group", pricingGroupToString(item.pricingGroup));
    query.bindValue(":notes", item.notes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to update item:" << m_lastError;
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool Database::deleteItem(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM items WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Failed to delete item:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// =============================================================================
// Transaction CRUD
// =============================================================================

bool Database::addTransaction(const Transaction &transaction)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO transactions (date, type, account, item_name, category,
                                  quantity, unit_price, total_amount, notes)
        VALUES (:date, :type, :account, :item_name, :category,
                :quantity, :unit_price, :total_amount, :notes)
    )");

    query.bindValue(":date", transaction.date.toString(Qt::ISODate));
    query.bindValue(":type", transactionTypeToString(transaction.type));
    query.bindValue(":account", accountTypeToString(transaction.account));
    query.bindValue(":item_name", transaction.item);
    query.bindValue(":category", transaction.category);
    query.bindValue(":quantity", transaction.quantity);
    query.bindValue(":unit_price", transaction.unitPrice);
    query.bindValue(":total_amount", transaction.totalAmount);
    query.bindValue(":notes", transaction.notes);

    if (!query.exec()) {
        qWarning() << "Failed to add transaction:" << query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Transaction> Database::getTransaction(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM transactions WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Transaction transaction;
    transaction.id = query.value("id").toInt();
    transaction.date = QDate::fromString(query.value("date").toString(), Qt::ISODate);
    transaction.type = stringToTransactionType(query.value("type").toString());
    transaction.account = stringToAccountType(query.value("account").toString());
    transaction.item = query.value("item_name").toString();
    transaction.category = query.value("category").toString();
    transaction.quantity = query.value("quantity").toInt();
    transaction.unitPrice = query.value("unit_price").toDouble();
    transaction.totalAmount = query.value("total_amount").toDouble();
    transaction.notes = query.value("notes").toString();

    return transaction;
}

QVector<Transaction> Database::getAllTransactions()
{
    QVector<Transaction> transactions;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM transactions ORDER BY date DESC, id DESC")) {
        qWarning() << "Failed to get transactions:" << query.lastError().text();
        return transactions;
    }

    while (query.next()) {
        Transaction transaction;
        transaction.id = query.value("id").toInt();
        transaction.date = QDate::fromString(query.value("date").toString(), Qt::ISODate);
        transaction.type = stringToTransactionType(query.value("type").toString());
        transaction.account = stringToAccountType(query.value("account").toString());
        transaction.item = query.value("item_name").toString();
        transaction.category = query.value("category").toString();
        transaction.quantity = query.value("quantity").toInt();
        transaction.unitPrice = query.value("unit_price").toDouble();
        transaction.totalAmount = query.value("total_amount").toDouble();
        transaction.notes = query.value("notes").toString();
        transactions.append(transaction);
    }

    return transactions;
}

QVector<Transaction> Database::getTransactionsByDateRange(const QDate &from, const QDate &to)
{
    QVector<Transaction> transactions;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT * FROM transactions
        WHERE date >= :from AND date <= :to
        ORDER BY date DESC, id DESC
    )");

    query.bindValue(":from", from.toString(Qt::ISODate));
    query.bindValue(":to", to.toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to get transactions by date range:" << query.lastError().text();
        return transactions;
    }

    while (query.next()) {
        Transaction transaction;
        transaction.id = query.value("id").toInt();
        transaction.date = QDate::fromString(query.value("date").toString(), Qt::ISODate);
        transaction.type = stringToTransactionType(query.value("type").toString());
        transaction.account = stringToAccountType(query.value("account").toString());
        transaction.item = query.value("item_name").toString();
        transaction.category = query.value("category").toString();
        transaction.quantity = query.value("quantity").toInt();
        transaction.unitPrice = query.value("unit_price").toDouble();
        transaction.totalAmount = query.value("total_amount").toDouble();
        transaction.notes = query.value("notes").toString();
        transactions.append(transaction);
    }

    return transactions;
}

bool Database::updateTransaction(const Transaction &transaction)
{
    if (!transaction.id.has_value()) {
        qWarning() << "Cannot update transaction without id";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE transactions SET
            date = :date,
            type = :type,
            account = :account,
            item_name = :item_name,
            category = :category,
            quantity = :quantity,
            unit_price = :unit_price,
            total_amount = :total_amount,
            notes = :notes
        WHERE id = :id
    )");

    query.bindValue(":id", transaction.id.value());
    query.bindValue(":date", transaction.date.toString(Qt::ISODate));
    query.bindValue(":type", transactionTypeToString(transaction.type));
    query.bindValue(":account", accountTypeToString(transaction.account));
    query.bindValue(":item_name", transaction.item);
    query.bindValue(":category", transaction.category);
    query.bindValue(":quantity", transaction.quantity);
    query.bindValue(":unit_price", transaction.unitPrice);
    query.bindValue(":total_amount", transaction.totalAmount);
    query.bindValue(":notes", transaction.notes);

    if (!query.exec()) {
        qWarning() << "Failed to update transaction:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool Database::deleteTransaction(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM transactions WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Failed to delete transaction:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// =============================================================================
// Vehicle CRUD
// =============================================================================

bool Database::addVehicle(const Vehicle &vehicle)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO vehicles (id, name, category_main, category_sub,
                              bucket_capacity_m3, truck_capacity_m3,
                              tank_capacity_l, fuel_use_l_per_hour,
                              purchase_price, active, notes)
        VALUES (:id, :name, :category_main, :category_sub,
                :bucket_capacity_m3, :truck_capacity_m3,
                :tank_capacity_l, :fuel_use_l_per_hour,
                :purchase_price, :active, :notes)
    )");

    query.bindValue(":id", vehicle.id);
    query.bindValue(":name", vehicle.name);
    query.bindValue(":category_main", vehicle.categoryMain);
    query.bindValue(":category_sub", vehicle.categorySub);
    query.bindValue(":bucket_capacity_m3", vehicle.bucketCapacityM3);
    query.bindValue(":truck_capacity_m3", vehicle.truckCapacityM3);
    query.bindValue(":tank_capacity_l", vehicle.tankCapacityL);
    query.bindValue(":fuel_use_l_per_hour", vehicle.fuelUseLPerHour);
    query.bindValue(":purchase_price", vehicle.purchasePrice);
    query.bindValue(":active", vehicle.active ? 1 : 0);
    query.bindValue(":notes", vehicle.notes);

    if (!query.exec()) {
        qWarning() << "Failed to add vehicle:" << query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Vehicle> Database::getVehicle(const QString &id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM vehicles WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Vehicle vehicle;
    vehicle.id = query.value("id").toString();
    vehicle.name = query.value("name").toString();
    vehicle.categoryMain = query.value("category_main").toString();
    vehicle.categorySub = query.value("category_sub").toString();
    vehicle.bucketCapacityM3 = query.value("bucket_capacity_m3").toDouble();
    vehicle.truckCapacityM3 = query.value("truck_capacity_m3").toDouble();
    vehicle.tankCapacityL = query.value("tank_capacity_l").toDouble();
    vehicle.fuelUseLPerHour = query.value("fuel_use_l_per_hour").toDouble();
    vehicle.purchasePrice = query.value("purchase_price").toDouble();
    vehicle.active = query.value("active").toInt() == 1;
    vehicle.notes = query.value("notes").toString();

    return vehicle;
}

QVector<Vehicle> Database::getAllVehicles(bool activeOnly)
{
    QVector<Vehicle> vehicles;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    QString sql = "SELECT * FROM vehicles";
    if (activeOnly) {
        sql += " WHERE active = 1";
    }
    sql += " ORDER BY category_main, name";

    if (!query.exec(sql)) {
        qWarning() << "Failed to get vehicles:" << query.lastError().text();
        return vehicles;
    }

    while (query.next()) {
        Vehicle vehicle;
        vehicle.id = query.value("id").toString();
        vehicle.name = query.value("name").toString();
        vehicle.categoryMain = query.value("category_main").toString();
        vehicle.categorySub = query.value("category_sub").toString();
        vehicle.bucketCapacityM3 = query.value("bucket_capacity_m3").toDouble();
        vehicle.truckCapacityM3 = query.value("truck_capacity_m3").toDouble();
        vehicle.tankCapacityL = query.value("tank_capacity_l").toDouble();
        vehicle.fuelUseLPerHour = query.value("fuel_use_l_per_hour").toDouble();
        vehicle.purchasePrice = query.value("purchase_price").toDouble();
        vehicle.active = query.value("active").toInt() == 1;
        vehicle.notes = query.value("notes").toString();
        vehicles.append(vehicle);
    }

    return vehicles;
}

bool Database::updateVehicle(const Vehicle &vehicle)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE vehicles SET
            name = :name,
            category_main = :category_main,
            category_sub = :category_sub,
            bucket_capacity_m3 = :bucket_capacity_m3,
            truck_capacity_m3 = :truck_capacity_m3,
            tank_capacity_l = :tank_capacity_l,
            fuel_use_l_per_hour = :fuel_use_l_per_hour,
            purchase_price = :purchase_price,
            active = :active,
            notes = :notes
        WHERE id = :id
    )");

    query.bindValue(":id", vehicle.id);
    query.bindValue(":name", vehicle.name);
    query.bindValue(":category_main", vehicle.categoryMain);
    query.bindValue(":category_sub", vehicle.categorySub);
    query.bindValue(":bucket_capacity_m3", vehicle.bucketCapacityM3);
    query.bindValue(":truck_capacity_m3", vehicle.truckCapacityM3);
    query.bindValue(":tank_capacity_l", vehicle.tankCapacityL);
    query.bindValue(":fuel_use_l_per_hour", vehicle.fuelUseLPerHour);
    query.bindValue(":purchase_price", vehicle.purchasePrice);
    query.bindValue(":active", vehicle.active ? 1 : 0);
    query.bindValue(":notes", vehicle.notes);

    if (!query.exec()) {
        qWarning() << "Failed to update vehicle:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool Database::deleteVehicle(const QString &id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM vehicles WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Failed to delete vehicle:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// =============================================================================
// Fuel Log CRUD
// =============================================================================

bool Database::addFuelLogEntry(const FuelLogEntry &entry)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO fuel_log (date_time, equipment_id, liters, unit_price,
                              total_cost, meter_or_hours, source, notes)
        VALUES (:date_time, :equipment_id, :liters, :unit_price,
                :total_cost, :meter_or_hours, :source, :notes)
    )");

    query.bindValue(":date_time", entry.dateTime.toString(Qt::ISODate));
    query.bindValue(":equipment_id", entry.equipmentId);
    query.bindValue(":liters", entry.liters);
    query.bindValue(":unit_price", entry.unitPrice);
    query.bindValue(":total_cost", entry.totalCost);
    query.bindValue(":meter_or_hours", entry.meterOrHours);
    query.bindValue(":source", entry.source);
    query.bindValue(":notes", entry.notes);

    if (!query.exec()) {
        qWarning() << "Failed to add fuel log entry:" << query.lastError().text();
        return false;
    }

    return true;
}

QVector<FuelLogEntry> Database::getFuelLog(const QDateTime &from, const QDateTime &to,
                                            const QString &equipmentId)
{
    QVector<FuelLogEntry> entries;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    QString sql = R"(
        SELECT * FROM fuel_log
        WHERE date_time >= :from AND date_time <= :to
    )";

    if (!equipmentId.isEmpty()) {
        sql += " AND equipment_id = :equipment_id";
    }

    sql += " ORDER BY date_time DESC";

    query.prepare(sql);
    query.bindValue(":from", from.toString(Qt::ISODate));
    query.bindValue(":to", to.toString(Qt::ISODate));

    if (!equipmentId.isEmpty()) {
        query.bindValue(":equipment_id", equipmentId);
    }

    if (!query.exec()) {
        qWarning() << "Failed to get fuel log:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        FuelLogEntry entry;
        entry.id = query.value("id").toInt();
        entry.dateTime = QDateTime::fromString(query.value("date_time").toString(), Qt::ISODate);
        entry.equipmentId = query.value("equipment_id").toString();
        entry.liters = query.value("liters").toDouble();
        entry.unitPrice = query.value("unit_price").toDouble();
        entry.totalCost = query.value("total_cost").toDouble();
        entry.meterOrHours = query.value("meter_or_hours").toDouble();
        entry.source = query.value("source").toString();
        entry.notes = query.value("notes").toString();
        entries.append(entry);
    }

    return entries;
}

double Database::getTotalFuelInRange(const QDateTime &from, const QDateTime &to)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT COALESCE(SUM(liters), 0) as total
        FROM fuel_log
        WHERE date_time >= :from AND date_time <= :to
    )");

    query.bindValue(":from", from.toString(Qt::ISODate));
    query.bindValue(":to", to.toString(Qt::ISODate));

    if (query.exec() && query.next()) {
        return query.value("total").toDouble();
    }

    return 0;
}

double Database::getTotalFuelCostInRange(const QDateTime &from, const QDateTime &to)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT COALESCE(SUM(total_cost), 0) as total
        FROM fuel_log
        WHERE date_time >= :from AND date_time <= :to
    )");

    query.bindValue(":from", from.toString(Qt::ISODate));
    query.bindValue(":to", to.toString(Qt::ISODate));

    if (query.exec() && query.next()) {
        return query.value("total").toDouble();
    }

    return 0;
}

// =============================================================================
// Movement Sessions CRUD
// =============================================================================

int Database::addMovementSession(const MovementSession &session)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO movement_sessions (start_time, end_time, map_name, notes)
        VALUES (:start_time, :end_time, :map_name, :notes)
    )");

    query.bindValue(":start_time", session.startTime.toString(Qt::ISODate));
    query.bindValue(":end_time", session.endTime.isValid()
                    ? session.endTime.toString(Qt::ISODate) : QVariant());
    query.bindValue(":map_name", session.mapName);
    query.bindValue(":notes", session.notes);

    if (!query.exec()) {
        qWarning() << "Failed to add movement session:" << query.lastError().text();
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<MovementSession> Database::getMovementSession(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM movement_sessions WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    MovementSession session;
    session.id = query.value("id").toInt();
    session.startTime = QDateTime::fromString(query.value("start_time").toString(), Qt::ISODate);

    QString endTimeStr = query.value("end_time").toString();
    if (!endTimeStr.isEmpty()) {
        session.endTime = QDateTime::fromString(endTimeStr, Qt::ISODate);
    }

    session.mapName = query.value("map_name").toString();
    session.notes = query.value("notes").toString();

    return session;
}

QVector<MovementSession> Database::getAllMovementSessions()
{
    QVector<MovementSession> sessions;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM movement_sessions ORDER BY start_time DESC")) {
        qWarning() << "Failed to get movement sessions:" << query.lastError().text();
        return sessions;
    }

    while (query.next()) {
        MovementSession session;
        session.id = query.value("id").toInt();
        session.startTime = QDateTime::fromString(query.value("start_time").toString(), Qt::ISODate);

        QString endTimeStr = query.value("end_time").toString();
        if (!endTimeStr.isEmpty()) {
            session.endTime = QDateTime::fromString(endTimeStr, Qt::ISODate);
        }

        session.mapName = query.value("map_name").toString();
        session.notes = query.value("notes").toString();
        sessions.append(session);
    }

    return sessions;
}

bool Database::updateMovementSession(const MovementSession &session)
{
    if (!session.id.has_value()) return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE movement_sessions SET
            start_time = :start_time,
            end_time = :end_time,
            map_name = :map_name,
            notes = :notes
        WHERE id = :id
    )");

    query.bindValue(":id", session.id.value());
    query.bindValue(":start_time", session.startTime.toString(Qt::ISODate));
    query.bindValue(":end_time", session.endTime.isValid()
                    ? session.endTime.toString(Qt::ISODate) : QVariant());
    query.bindValue(":map_name", session.mapName);
    query.bindValue(":notes", session.notes);

    if (!query.exec()) {
        qWarning() << "Failed to update movement session:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool Database::deleteMovementSession(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    // First delete associated equipment usage
    deleteEquipmentUsageForSession(id);

    QSqlQuery query(db);
    query.prepare("DELETE FROM movement_sessions WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Failed to delete movement session:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// =============================================================================
// Movement Equipment Usage CRUD
// =============================================================================

bool Database::addOrUpdateEquipmentUsage(const MovementEquipmentUsage &usage)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Check if entry exists for this session/equipment/role combo
    query.prepare(R"(
        SELECT id FROM movement_equipment_usage
        WHERE session_id = :session_id
          AND equipment_id = :equipment_id
          AND role = :role
    )");
    query.bindValue(":session_id", usage.sessionId);
    query.bindValue(":equipment_id", usage.equipmentId);
    query.bindValue(":role", usage.role);

    if (query.exec() && query.next()) {
        // Update existing
        int existingId = query.value("id").toInt();

        query.prepare(R"(
            UPDATE movement_equipment_usage SET
                hours_used = :hours_used,
                buckets = :buckets,
                loads = :loads,
                dumps = :dumps,
                estimated_fuel_l = :estimated_fuel_l
            WHERE id = :id
        )");
        query.bindValue(":id", existingId);
    } else {
        // Insert new
        query.prepare(R"(
            INSERT INTO movement_equipment_usage
                (session_id, equipment_id, role, hours_used, buckets, loads, dumps, estimated_fuel_l)
            VALUES
                (:session_id, :equipment_id, :role, :hours_used, :buckets, :loads, :dumps, :estimated_fuel_l)
        )");
        query.bindValue(":session_id", usage.sessionId);
        query.bindValue(":equipment_id", usage.equipmentId);
        query.bindValue(":role", usage.role);
    }

    query.bindValue(":hours_used", usage.hoursUsed);
    query.bindValue(":buckets", usage.buckets);
    query.bindValue(":loads", usage.loads);
    query.bindValue(":dumps", usage.dumps);
    query.bindValue(":estimated_fuel_l", usage.estimatedFuelL);

    if (!query.exec()) {
        qWarning() << "Failed to add/update equipment usage:" << query.lastError().text();
        return false;
    }

    return true;
}

QVector<MovementEquipmentUsage> Database::getEquipmentUsageForSession(int sessionId)
{
    QVector<MovementEquipmentUsage> usages;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT * FROM movement_equipment_usage
        WHERE session_id = :session_id
        ORDER BY id
    )");
    query.bindValue(":session_id", sessionId);

    if (!query.exec()) {
        qWarning() << "Failed to get equipment usage:" << query.lastError().text();
        return usages;
    }

    while (query.next()) {
        MovementEquipmentUsage usage;
        usage.id = query.value("id").toInt();
        usage.sessionId = query.value("session_id").toInt();
        usage.equipmentId = query.value("equipment_id").toString();
        usage.role = query.value("role").toString();
        usage.hoursUsed = query.value("hours_used").toDouble();
        usage.buckets = query.value("buckets").toInt();
        usage.loads = query.value("loads").toInt();
        usage.dumps = query.value("dumps").toInt();
        usage.estimatedFuelL = query.value("estimated_fuel_l").toDouble();
        usages.append(usage);
    }

    return usages;
}

bool Database::deleteEquipmentUsage(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM movement_equipment_usage WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Failed to delete equipment usage:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool Database::deleteEquipmentUsageForSession(int sessionId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM movement_equipment_usage WHERE session_id = :session_id");
    query.bindValue(":session_id", sessionId);

    if (!query.exec()) {
        qWarning() << "Failed to delete equipment usage for session:" << query.lastError().text();
        return false;
    }

    return true;
}

// =============================================================================
// Recipe Tables
// =============================================================================

bool Database::createRecipeTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Workbenches table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS workbenches (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL
        )
    )")) {
        qWarning() << "Failed to create workbenches table:" << query.lastError().text();
        return false;
    }

    // Recipes table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS recipes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            workbench_id INTEGER NOT NULL,
            output_item TEXT NOT NULL,
            output_qty INTEGER DEFAULT 1,
            notes TEXT,
            FOREIGN KEY (workbench_id) REFERENCES workbenches(id)
        )
    )")) {
        qWarning() << "Failed to create recipes table:" << query.lastError().text();
        return false;
    }

    // Recipe ingredients table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS recipe_ingredients (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            recipe_id INTEGER NOT NULL,
            item_name TEXT NOT NULL,
            quantity INTEGER DEFAULT 1,
            FOREIGN KEY (recipe_id) REFERENCES recipes(id) ON DELETE CASCADE
        )
    )")) {
        qWarning() << "Failed to create recipe_ingredients table:" << query.lastError().text();
        return false;
    }

    return true;
}

// =============================================================================
// Workbench CRUD
// =============================================================================

int Database::addWorkbench(const Workbench &workbench)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("INSERT INTO workbenches (name) VALUES (:name)");
    query.bindValue(":name", workbench.name);

    if (!query.exec()) {
        qWarning() << "Failed to add workbench:" << query.lastError().text();
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<Workbench> Database::getWorkbench(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM workbenches WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Workbench wb;
    wb.id = query.value("id").toInt();
    wb.name = query.value("name").toString();

    return wb;
}

std::optional<Workbench> Database::getWorkbenchByName(const QString &name)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM workbenches WHERE name = :name");
    query.bindValue(":name", name);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Workbench wb;
    wb.id = query.value("id").toInt();
    wb.name = query.value("name").toString();

    return wb;
}

QVector<Workbench> Database::getAllWorkbenches()
{
    QVector<Workbench> workbenches;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM workbenches ORDER BY name")) {
        qWarning() << "Failed to get workbenches:" << query.lastError().text();
        return workbenches;
    }

    while (query.next()) {
        Workbench wb;
        wb.id = query.value("id").toInt();
        wb.name = query.value("name").toString();
        workbenches.append(wb);
    }

    return workbenches;
}

bool Database::deleteWorkbench(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM workbenches WHERE id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

bool Database::clearAllWorkbenches()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Delete in order due to foreign keys
    if (!query.exec("DELETE FROM recipe_ingredients")) {
        qWarning() << "Failed to clear recipe_ingredients:" << query.lastError().text();
        return false;
    }
    if (!query.exec("DELETE FROM recipes")) {
        qWarning() << "Failed to clear recipes:" << query.lastError().text();
        return false;
    }
    if (!query.exec("DELETE FROM workbenches")) {
        qWarning() << "Failed to clear workbenches:" << query.lastError().text();
        return false;
    }

    return true;
}

// =============================================================================
// Recipe CRUD
// =============================================================================

int Database::addRecipe(const Recipe &recipe)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO recipes (workbench_id, output_item, output_qty, notes)
        VALUES (:workbench_id, :output_item, :output_qty, :notes)
    )");
    query.bindValue(":workbench_id", recipe.workbenchId);
    query.bindValue(":output_item", recipe.outputItem);
    query.bindValue(":output_qty", recipe.outputQty);
    query.bindValue(":notes", recipe.notes);

    if (!query.exec()) {
        qWarning() << "Failed to add recipe:" << query.lastError().text();
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<Recipe> Database::getRecipe(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT r.*, w.name as workbench_name
        FROM recipes r
        JOIN workbenches w ON r.workbench_id = w.id
        WHERE r.id = :id
    )");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Recipe recipe;
    recipe.id = query.value("id").toInt();
    recipe.workbenchId = query.value("workbench_id").toInt();
    recipe.workbenchName = query.value("workbench_name").toString();
    recipe.outputItem = query.value("output_item").toString();
    recipe.outputQty = query.value("output_qty").toInt();
    recipe.notes = query.value("notes").toString();

    // Load ingredients
    recipe.ingredients = getIngredientsForRecipe(recipe.id.value());

    return recipe;
}

QVector<Recipe> Database::getAllRecipes()
{
    QVector<Recipe> recipes;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec(R"(
        SELECT r.*, w.name as workbench_name
        FROM recipes r
        JOIN workbenches w ON r.workbench_id = w.id
        ORDER BY w.name, r.output_item
    )")) {
        qWarning() << "Failed to get recipes:" << query.lastError().text();
        return recipes;
    }

    while (query.next()) {
        Recipe recipe;
        recipe.id = query.value("id").toInt();
        recipe.workbenchId = query.value("workbench_id").toInt();
        recipe.workbenchName = query.value("workbench_name").toString();
        recipe.outputItem = query.value("output_item").toString();
        recipe.outputQty = query.value("output_qty").toInt();
        recipe.notes = query.value("notes").toString();

        // Load ingredients
        recipe.ingredients = getIngredientsForRecipe(recipe.id.value());

        recipes.append(recipe);
    }

    return recipes;
}

QVector<Recipe> Database::getRecipesByWorkbench(int workbenchId)
{
    QVector<Recipe> recipes;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT r.*, w.name as workbench_name
        FROM recipes r
        JOIN workbenches w ON r.workbench_id = w.id
        WHERE r.workbench_id = :workbench_id
        ORDER BY r.output_item
    )");
    query.bindValue(":workbench_id", workbenchId);

    if (!query.exec()) {
        qWarning() << "Failed to get recipes by workbench:" << query.lastError().text();
        return recipes;
    }

    while (query.next()) {
        Recipe recipe;
        recipe.id = query.value("id").toInt();
        recipe.workbenchId = query.value("workbench_id").toInt();
        recipe.workbenchName = query.value("workbench_name").toString();
        recipe.outputItem = query.value("output_item").toString();
        recipe.outputQty = query.value("output_qty").toInt();
        recipe.notes = query.value("notes").toString();
        recipe.ingredients = getIngredientsForRecipe(recipe.id.value());
        recipes.append(recipe);
    }

    return recipes;
}

QVector<Recipe> Database::getRecipesByWorkbenchName(const QString &workbenchName)
{
    auto wb = getWorkbenchByName(workbenchName);
    if (!wb.has_value()) {
        return QVector<Recipe>();
    }
    return getRecipesByWorkbench(wb->id.value());
}

QVector<Recipe> Database::getRecipesForOutput(const QString &outputItem)
{
    QVector<Recipe> recipes;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT r.*, w.name as workbench_name
        FROM recipes r
        JOIN workbenches w ON r.workbench_id = w.id
        WHERE r.output_item = :output_item
        ORDER BY w.name
    )");
    query.bindValue(":output_item", outputItem);

    if (!query.exec()) {
        qWarning() << "Failed to get recipes for output:" << query.lastError().text();
        return recipes;
    }

    while (query.next()) {
        Recipe recipe;
        recipe.id = query.value("id").toInt();
        recipe.workbenchId = query.value("workbench_id").toInt();
        recipe.workbenchName = query.value("workbench_name").toString();
        recipe.outputItem = query.value("output_item").toString();
        recipe.outputQty = query.value("output_qty").toInt();
        recipe.notes = query.value("notes").toString();
        recipe.ingredients = getIngredientsForRecipe(recipe.id.value());
        recipes.append(recipe);
    }

    return recipes;
}

bool Database::deleteRecipe(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Delete ingredients first (cascade should handle this, but be explicit)
    deleteIngredientsForRecipe(id);

    query.prepare("DELETE FROM recipes WHERE id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

bool Database::clearAllRecipes()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("DELETE FROM recipe_ingredients")) {
        return false;
    }
    if (!query.exec("DELETE FROM recipes")) {
        return false;
    }

    return true;
}

// =============================================================================
// Recipe Ingredient CRUD
// =============================================================================

bool Database::addRecipeIngredient(const RecipeIngredient &ingredient)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO recipe_ingredients (recipe_id, item_name, quantity)
        VALUES (:recipe_id, :item_name, :quantity)
    )");
    query.bindValue(":recipe_id", ingredient.recipeId);
    query.bindValue(":item_name", ingredient.itemName);
    query.bindValue(":quantity", ingredient.quantity);

    if (!query.exec()) {
        qWarning() << "Failed to add recipe ingredient:" << query.lastError().text();
        return false;
    }

    return true;
}

QVector<RecipeIngredient> Database::getIngredientsForRecipe(int recipeId)
{
    QVector<RecipeIngredient> ingredients;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM recipe_ingredients WHERE recipe_id = :recipe_id");
    query.bindValue(":recipe_id", recipeId);

    if (!query.exec()) {
        qWarning() << "Failed to get ingredients:" << query.lastError().text();
        return ingredients;
    }

    while (query.next()) {
        RecipeIngredient ing;
        ing.id = query.value("id").toInt();
        ing.recipeId = query.value("recipe_id").toInt();
        ing.itemName = query.value("item_name").toString();
        ing.quantity = query.value("quantity").toInt();
        ingredients.append(ing);
    }

    return ingredients;
}

bool Database::deleteIngredientsForRecipe(int recipeId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM recipe_ingredients WHERE recipe_id = :recipe_id");
    query.bindValue(":recipe_id", recipeId);

    return query.exec();
}

// =============================================================================
// Item Helper
// =============================================================================

std::optional<Item> Database::getItemByName(const QString &name)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM items WHERE name = :name");
    query.bindValue(":name", name);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Item item;
    item.id = query.value("id").toInt();
    item.code = query.value("code").toString();
    item.name = query.value("name").toString();
    item.categoryMain = query.value("category").toString();
    item.buyPriceInternal = query.value("buy_price").toDouble();
    item.buyPriceDisplay = std::round(item.buyPriceInternal);
    item.sellPriceInternal = query.value("sell_price_internal").toDouble();
    item.sellPriceDisplay = query.value("sell_price_display").toDouble();
    item.weight = query.value("weight").toDouble();
    item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
    item.notes = query.value("notes").toString();

    return item;
}

// =============================================================================
// Location Tables Creation
// =============================================================================

bool Database::createLocationTables()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Maps table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS maps (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            abbrev TEXT NOT NULL UNIQUE,
            name TEXT NOT NULL UNIQUE
        )
    )")) {
        qWarning() << "Failed to create maps table:" << query.lastError().text();
        return false;
    }

    // Location types table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS location_types (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        )
    )")) {
        qWarning() << "Failed to create location_types table:" << query.lastError().text();
        return false;
    }

    // Locations table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS locations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            map_id INTEGER NOT NULL,
            type_id INTEGER NOT NULL,
            FOREIGN KEY (map_id) REFERENCES maps(id),
            FOREIGN KEY (type_id) REFERENCES location_types(id)
        )
    )")) {
        qWarning() << "Failed to create locations table:" << query.lastError().text();
        return false;
    }

    return true;
}

// =============================================================================
// Map CRUD
// =============================================================================

int Database::addMap(const Map &map)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO maps (abbrev, name)
        VALUES (:abbrev, :name)
    )");
    query.bindValue(":abbrev", map.abbrev);
    query.bindValue(":name", map.name);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to add map:" << m_lastError;
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<Map> Database::getMap(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM maps WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Map map;
    map.id = query.value("id").toInt();
    map.abbrev = query.value("abbrev").toString();
    map.name = query.value("name").toString();

    return map;
}

std::optional<Map> Database::getMapByAbbrev(const QString &abbrev)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM maps WHERE abbrev = :abbrev");
    query.bindValue(":abbrev", abbrev);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Map map;
    map.id = query.value("id").toInt();
    map.abbrev = query.value("abbrev").toString();
    map.name = query.value("name").toString();

    return map;
}

QVector<Map> Database::getAllMaps()
{
    QVector<Map> maps;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM maps ORDER BY abbrev")) {
        qWarning() << "Failed to get maps:" << query.lastError().text();
        return maps;
    }

    while (query.next()) {
        Map map;
        map.id = query.value("id").toInt();
        map.abbrev = query.value("abbrev").toString();
        map.name = query.value("name").toString();
        maps.append(map);
    }

    return maps;
}

bool Database::deleteMap(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM maps WHERE id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

bool Database::clearAllMaps()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Clear locations first (foreign key constraint)
    if (!query.exec("DELETE FROM locations")) {
        return false;
    }
    if (!query.exec("DELETE FROM maps")) {
        return false;
    }

    return true;
}

// =============================================================================
// LocationType CRUD
// =============================================================================

int Database::addLocationType(const LocationType &type)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("INSERT INTO location_types (name) VALUES (:name)");
    query.bindValue(":name", type.name);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to add location type:" << m_lastError;
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<LocationType> Database::getLocationType(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM location_types WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    LocationType type;
    type.id = query.value("id").toInt();
    type.name = query.value("name").toString();

    return type;
}

std::optional<LocationType> Database::getLocationTypeByName(const QString &name)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM location_types WHERE name = :name");
    query.bindValue(":name", name);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    LocationType type;
    type.id = query.value("id").toInt();
    type.name = query.value("name").toString();

    return type;
}

QVector<LocationType> Database::getAllLocationTypes()
{
    QVector<LocationType> types;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM location_types ORDER BY name")) {
        qWarning() << "Failed to get location types:" << query.lastError().text();
        return types;
    }

    while (query.next()) {
        LocationType type;
        type.id = query.value("id").toInt();
        type.name = query.value("name").toString();
        types.append(type);
    }

    return types;
}

bool Database::deleteLocationType(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM location_types WHERE id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

bool Database::clearAllLocationTypes()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Clear locations first (foreign key constraint)
    if (!query.exec("DELETE FROM locations")) {
        return false;
    }
    if (!query.exec("DELETE FROM location_types")) {
        return false;
    }

    return true;
}

// =============================================================================
// Location CRUD
// =============================================================================

int Database::addLocation(const Location &location)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO locations (name, map_id, type_id)
        VALUES (:name, :map_id, :type_id)
    )");
    query.bindValue(":name", location.name);
    query.bindValue(":map_id", location.mapId);
    query.bindValue(":type_id", location.typeId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to add location:" << m_lastError;
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<Location> Database::getLocation(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT l.*, m.abbrev as map_abbrev, m.name as map_name, t.name as type_name
        FROM locations l
        JOIN maps m ON l.map_id = m.id
        JOIN location_types t ON l.type_id = t.id
        WHERE l.id = :id
    )");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Location loc;
    loc.id = query.value("id").toInt();
    loc.name = query.value("name").toString();
    loc.mapId = query.value("map_id").toInt();
    loc.typeId = query.value("type_id").toInt();
    loc.mapAbbrev = query.value("map_abbrev").toString();
    loc.mapName = query.value("map_name").toString();
    loc.typeName = query.value("type_name").toString();

    return loc;
}

QVector<Location> Database::getAllLocations()
{
    QVector<Location> locations;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec(R"(
        SELECT l.*, m.abbrev as map_abbrev, m.name as map_name, t.name as type_name
        FROM locations l
        JOIN maps m ON l.map_id = m.id
        JOIN location_types t ON l.type_id = t.id
        ORDER BY m.abbrev, l.name
    )")) {
        qWarning() << "Failed to get locations:" << query.lastError().text();
        return locations;
    }

    while (query.next()) {
        Location loc;
        loc.id = query.value("id").toInt();
        loc.name = query.value("name").toString();
        loc.mapId = query.value("map_id").toInt();
        loc.typeId = query.value("type_id").toInt();
        loc.mapAbbrev = query.value("map_abbrev").toString();
        loc.mapName = query.value("map_name").toString();
        loc.typeName = query.value("type_name").toString();
        locations.append(loc);
    }

    return locations;
}

QVector<Location> Database::getLocationsByMap(int mapId)
{
    QVector<Location> locations;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT l.*, m.abbrev as map_abbrev, m.name as map_name, t.name as type_name
        FROM locations l
        JOIN maps m ON l.map_id = m.id
        JOIN location_types t ON l.type_id = t.id
        WHERE l.map_id = :map_id
        ORDER BY l.name
    )");
    query.bindValue(":map_id", mapId);

    if (!query.exec()) {
        qWarning() << "Failed to get locations by map:" << query.lastError().text();
        return locations;
    }

    while (query.next()) {
        Location loc;
        loc.id = query.value("id").toInt();
        loc.name = query.value("name").toString();
        loc.mapId = query.value("map_id").toInt();
        loc.typeId = query.value("type_id").toInt();
        loc.mapAbbrev = query.value("map_abbrev").toString();
        loc.mapName = query.value("map_name").toString();
        loc.typeName = query.value("type_name").toString();
        locations.append(loc);
    }

    return locations;
}

QVector<Location> Database::getLocationsByType(int typeId)
{
    QVector<Location> locations;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT l.*, m.abbrev as map_abbrev, m.name as map_name, t.name as type_name
        FROM locations l
        JOIN maps m ON l.map_id = m.id
        JOIN location_types t ON l.type_id = t.id
        WHERE l.type_id = :type_id
        ORDER BY m.abbrev, l.name
    )");
    query.bindValue(":type_id", typeId);

    if (!query.exec()) {
        qWarning() << "Failed to get locations by type:" << query.lastError().text();
        return locations;
    }

    while (query.next()) {
        Location loc;
        loc.id = query.value("id").toInt();
        loc.name = query.value("name").toString();
        loc.mapId = query.value("map_id").toInt();
        loc.typeId = query.value("type_id").toInt();
        loc.mapAbbrev = query.value("map_abbrev").toString();
        loc.mapName = query.value("map_name").toString();
        loc.typeName = query.value("type_name").toString();
        locations.append(loc);
    }

    return locations;
}

QVector<Location> Database::getLocationsByMapAndType(int mapId, int typeId)
{
    QVector<Location> locations;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT l.*, m.abbrev as map_abbrev, m.name as map_name, t.name as type_name
        FROM locations l
        JOIN maps m ON l.map_id = m.id
        JOIN location_types t ON l.type_id = t.id
        WHERE l.map_id = :map_id AND l.type_id = :type_id
        ORDER BY l.name
    )");
    query.bindValue(":map_id", mapId);
    query.bindValue(":type_id", typeId);

    if (!query.exec()) {
        qWarning() << "Failed to get locations by map and type:" << query.lastError().text();
        return locations;
    }

    while (query.next()) {
        Location loc;
        loc.id = query.value("id").toInt();
        loc.name = query.value("name").toString();
        loc.mapId = query.value("map_id").toInt();
        loc.typeId = query.value("type_id").toInt();
        loc.mapAbbrev = query.value("map_abbrev").toString();
        loc.mapName = query.value("map_name").toString();
        loc.typeName = query.value("type_name").toString();
        locations.append(loc);
    }

    return locations;
}

bool Database::deleteLocation(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM locations WHERE id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

bool Database::clearAllLocations()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    return query.exec("DELETE FROM locations");
}

// =============================================================================
// Inventory CRUD
// =============================================================================

    int Database::addInventoryItem(const InventoryItem &item)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO inventory (item_id, quantity, location_id, last_updated)
        VALUES (:item_id, :quantity, :location_id, :last_updated)
    )");
    query.bindValue(":item_id", item.itemId);
    query.bindValue(":quantity", item.quantity);
    query.bindValue(":location_id", item.locationId.has_value() ? item.locationId.value() : QVariant());
    query.bindValue(":last_updated", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to add inventory item:" << m_lastError;
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<InventoryItem> Database::getInventoryItem(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT inv.*,
               i.name as item_name, i.code as item_code, i.category, i.sell_price_internal as unit_price,
               l.name as location_name
        FROM inventory inv
        JOIN items i ON inv.item_id = i.id
        LEFT JOIN locations l ON inv.location_id = l.id
        WHERE inv.id = :id
    )");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    InventoryItem item;
    item.id = query.value("id").toInt();
    item.itemId = query.value("item_id").toInt();
    item.quantity = query.value("quantity").toInt();
    if (!query.value("location_id").isNull()) {
        item.locationId = query.value("location_id").toInt();
    }
    item.lastUpdated = QDateTime::fromString(query.value("last_updated").toString(), Qt::ISODate);
    item.itemName = query.value("item_name").toString();
    item.itemCode = query.value("item_code").toString();
    item.category = query.value("category").toString();
    item.unitPrice = query.value("unit_price").toDouble();
    item.locationName = query.value("location_name").toString();

    return item;
}

std::optional<InventoryItem> Database::getInventoryByItemId(int itemId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT inv.*,
               i.name as item_name, i.code as item_code, i.category, i.sell_price_internal as unit_price,
               l.name as location_name
        FROM inventory inv
        JOIN items i ON inv.item_id = i.id
        LEFT JOIN locations l ON inv.location_id = l.id
        WHERE inv.item_id = :item_id
    )");
    query.bindValue(":item_id", itemId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    InventoryItem item;
    item.id = query.value("id").toInt();
    item.itemId = query.value("item_id").toInt();
    item.quantity = query.value("quantity").toInt();
    if (!query.value("location_id").isNull()) {
        item.locationId = query.value("location_id").toInt();
    }
    item.lastUpdated = QDateTime::fromString(query.value("last_updated").toString(), Qt::ISODate);
    item.itemName = query.value("item_name").toString();
    item.itemCode = query.value("item_code").toString();
    item.category = query.value("category").toString();
    item.unitPrice = query.value("unit_price").toDouble();
    item.locationName = query.value("location_name").toString();

    return item;
}

std::optional<InventoryItem> Database::getInventoryByItemName(const QString &itemName)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT inv.*,
               i.name as item_name, i.code as item_code, i.category, i.sell_price_internal as unit_price,
               l.name as location_name
        FROM inventory inv
        JOIN items i ON inv.item_id = i.id
        LEFT JOIN locations l ON inv.location_id = l.id
        WHERE i.name = :item_name
    )");
    query.bindValue(":item_name", itemName);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    InventoryItem item;
    item.id = query.value("id").toInt();
    item.itemId = query.value("item_id").toInt();
    item.quantity = query.value("quantity").toInt();
    if (!query.value("location_id").isNull()) {
        item.locationId = query.value("location_id").toInt();
    }
    item.lastUpdated = QDateTime::fromString(query.value("last_updated").toString(), Qt::ISODate);
    item.itemName = query.value("item_name").toString();
    item.itemCode = query.value("item_code").toString();
    item.category = query.value("category").toString();
    item.unitPrice = query.value("unit_price").toDouble();
    item.locationName = query.value("location_name").toString();

    return item;
}

QVector<InventoryItem> Database::getAllInventory()
{
    QVector<InventoryItem> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec(R"(
        SELECT inv.*,
               i.name as item_name, i.code as item_code, i.category, i.sell_price_internal as unit_price,
               l.name as location_name
        FROM inventory inv
        JOIN items i ON inv.item_id = i.id
        LEFT JOIN locations l ON inv.location_id = l.id
        ORDER BY i.category, i.name
    )")) {
        qWarning() << "Failed to get inventory:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        InventoryItem item;
        item.id = query.value("id").toInt();
        item.itemId = query.value("item_id").toInt();
        item.quantity = query.value("quantity").toInt();
        if (!query.value("location_id").isNull()) {
            item.locationId = query.value("location_id").toInt();
        }
        item.lastUpdated = QDateTime::fromString(query.value("last_updated").toString(), Qt::ISODate);
        item.itemName = query.value("item_name").toString();
        item.itemCode = query.value("item_code").toString();
        item.category = query.value("category").toString();
        item.unitPrice = query.value("unit_price").toDouble();
        item.locationName = query.value("location_name").toString();
        items.append(item);
    }

    return items;
}

QVector<InventoryItem> Database::getInventoryByCategory(const QString &category)
{
    QVector<InventoryItem> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT inv.*,
               i.name as item_name, i.code as item_code, i.category, i.sell_price_internal as unit_price,
               l.name as location_name
        FROM inventory inv
        JOIN items i ON inv.item_id = i.id
        LEFT JOIN locations l ON inv.location_id = l.id
        WHERE i.category = :category
        ORDER BY i.name
    )");
    query.bindValue(":category", category);

    if (!query.exec()) {
        qWarning() << "Failed to get inventory by category:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        InventoryItem item;
        item.id = query.value("id").toInt();
        item.itemId = query.value("item_id").toInt();
        item.quantity = query.value("quantity").toInt();
        if (!query.value("location_id").isNull()) {
            item.locationId = query.value("location_id").toInt();
        }
        item.lastUpdated = QDateTime::fromString(query.value("last_updated").toString(), Qt::ISODate);
        item.itemName = query.value("item_name").toString();
        item.itemCode = query.value("item_code").toString();
        item.category = query.value("category").toString();
        item.unitPrice = query.value("unit_price").toDouble();
        item.locationName = query.value("location_name").toString();
        items.append(item);
    }

    return items;
}

QVector<InventoryItem> Database::getInventoryByLocation(int locationId)
{
    QVector<InventoryItem> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT inv.*,
               i.name as item_name, i.code as item_code, i.category, i.sell_price_internal as unit_price,
               l.name as location_name
        FROM inventory inv
        JOIN items i ON inv.item_id = i.id
        LEFT JOIN locations l ON inv.location_id = l.id
        WHERE inv.location_id = :location_id
        ORDER BY i.category, i.name
    )");
    query.bindValue(":location_id", locationId);

    if (!query.exec()) {
        qWarning() << "Failed to get inventory by location:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        InventoryItem item;
        item.id = query.value("id").toInt();
        item.itemId = query.value("item_id").toInt();
        item.quantity = query.value("quantity").toInt();
        if (!query.value("location_id").isNull()) {
            item.locationId = query.value("location_id").toInt();
        }
        item.lastUpdated = QDateTime::fromString(query.value("last_updated").toString(), Qt::ISODate);
        item.itemName = query.value("item_name").toString();
        item.itemCode = query.value("item_code").toString();
        item.category = query.value("category").toString();
        item.unitPrice = query.value("unit_price").toDouble();
        item.locationName = query.value("location_name").toString();
        items.append(item);
    }

    return items;
}

QVector<InventoryItem> Database::getInventoryWithStock()
{
    QVector<InventoryItem> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec(R"(
        SELECT inv.*,
               i.name as item_name, i.code as item_code, i.category, i.sell_price_internal as unit_price,
               l.name as location_name
        FROM inventory inv
        JOIN items i ON inv.item_id = i.id
        LEFT JOIN locations l ON inv.location_id = l.id
        WHERE inv.quantity > 0
        ORDER BY i.category, i.name
    )")) {
        qWarning() << "Failed to get inventory with stock:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        InventoryItem item;
        item.id = query.value("id").toInt();
        item.itemId = query.value("item_id").toInt();
        item.quantity = query.value("quantity").toInt();
        if (!query.value("location_id").isNull()) {
            item.locationId = query.value("location_id").toInt();
        }
        item.lastUpdated = QDateTime::fromString(query.value("last_updated").toString(), Qt::ISODate);
        item.itemName = query.value("item_name").toString();
        item.itemCode = query.value("item_code").toString();
        item.category = query.value("category").toString();
        item.unitPrice = query.value("unit_price").toDouble();
        item.locationName = query.value("location_name").toString();
        items.append(item);
    }

    return items;
}

bool Database::updateInventoryQuantity(int id, int newQuantity)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE inventory
        SET quantity = :quantity, last_updated = :last_updated
        WHERE id = :id
    )");
    query.bindValue(":quantity", newQuantity);
    query.bindValue(":last_updated", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":id", id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool Database::adjustInventoryQuantity(int id, int delta)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE inventory
        SET quantity = MAX(0, quantity + :delta), last_updated = :last_updated
        WHERE id = :id
    )");
    query.bindValue(":delta", delta);
    query.bindValue(":last_updated", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":id", id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool Database::updateInventoryItem(const InventoryItem &item)
{
    if (!item.id.has_value()) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE inventory
        SET item_id = :item_id, quantity = :quantity, location_id = :location_id,
            last_updated = :last_updated
        WHERE id = :id
    )");
    query.bindValue(":item_id", item.itemId);
    query.bindValue(":quantity", item.quantity);
    query.bindValue(":location_id", item.locationId.has_value() ? item.locationId.value() : QVariant());
    query.bindValue(":last_updated", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":id", item.id.value());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool Database::deleteInventoryItem(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM inventory WHERE id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

bool Database::clearAllInventory()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    return query.exec("DELETE FROM inventory");
}

// =============================================================================
// Oil Tracking CRUD
// =============================================================================

OilTracking Database::getOilTracking()
{
    OilTracking tracking;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (query.exec("SELECT * FROM oil_tracking WHERE id = 1") && query.next()) {
        tracking.oilCap = query.value("oil_cap").toInt();
        tracking.totalOilSold = query.value("total_oil_sold").toInt();
        tracking.lastReset = QDateTime::fromString(query.value("last_reset").toString(), Qt::ISODate);
    }

    return tracking;
}

bool Database::saveOilTracking(const OilTracking &tracking)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE oil_tracking
        SET oil_cap = :oil_cap, total_oil_sold = :total_oil_sold
        WHERE id = 1
    )");
    query.bindValue(":oil_cap", tracking.oilCap);
    query.bindValue(":total_oil_sold", tracking.totalOilSold);

    return query.exec();
}

bool Database::addOilSold(int quantity)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("UPDATE oil_tracking SET total_oil_sold = total_oil_sold + :qty WHERE id = 1");
    query.bindValue(":qty", quantity);

    return query.exec();
}

bool Database::resetOilTracking()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE oil_tracking
        SET total_oil_sold = 0, last_reset = :last_reset
        WHERE id = 1
    )");
    query.bindValue(":last_reset", QDateTime::currentDateTime().toString(Qt::ISODate));

    return query.exec();
}

// =============================================================================
// Helper Functions
// =============================================================================

QString pricingGroupToString(PricingGroup group)
{
    switch (group) {
        case PricingGroup::Base70:
            return "Base70";
        // case PricingGroup::Resource72:
        //     return "Resource72";
        // case PricingGroup::Special75:
        //     return "Special75";
        case PricingGroup::Custom:
            return "Custom";
        default:
            return "Base70";
    }
}

PricingGroup stringToPricingGroup(const QString &str)
{
    if (str == "Base70") return PricingGroup::Base70;
    // Legacy support for old data
    // if (str == "Resource72") return PricingGroup::Resource72;
    // if (str == "Special75") return PricingGroup::Special75;
    if (str == "Custom") return PricingGroup::Custom;
    return PricingGroup::Base70;
}

QString transactionTypeToString(TransactionType type)
{
    switch (type) {
        case TransactionType::Sale:
            return "Sale";
        case TransactionType::Purchase:
            return "Purchase";
        case TransactionType::Transfer:
            return "Transfer";
        case TransactionType::Fuel:
            return "Fuel";
        default:
            return "Sale";
    }
}

TransactionType stringToTransactionType(const QString &str)
{
    if (str == "Sale") return TransactionType::Sale;
    if (str == "Purchase") return TransactionType::Purchase;
    if (str == "Transfer") return TransactionType::Transfer;
    if (str == "Fuel") return TransactionType::Fuel;
    return TransactionType::Sale;
}

QString accountTypeToString(AccountType type)
{
    switch (type) {
        case AccountType::Company:
            return "Company";
        case AccountType::Personal:
            return "Personal";
        default:
            return "Company";
    }
}

AccountType stringToAccountType(const QString &str)
{
    if (str == "Company") return AccountType::Company;
    if (str == "Personal") return AccountType::Personal;
    return AccountType::Company;
}

} // namespace Frontier
