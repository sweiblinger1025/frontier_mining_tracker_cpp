/**
 * @file database.cpp
 * @brief Database access layer implementation for Frontier Mining Tracker
 */

#include "database.h"

#include <QDebug>
#include <QSqlError>
#include <QUuid>

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

bool Database::connect(const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        qWarning() << "Failed to open database:" << m_lastError;
        return false;
    }

    return true;
}

bool Database::initialize(const QString &dbPath)
{
    if (!connect(dbPath)) {
        return false;
    }
    
    if (!createTables()) {
        m_lastError = "Failed to create tables";
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
            code TEXT UNIQUE,
            name TEXT NOT NULL,
            category_main TEXT,
            category_sub TEXT,
            buy_price_internal REAL DEFAULT 0,
            buy_price_display REAL DEFAULT 0,
            sell_price_internal REAL DEFAULT 0,
            sell_price_display REAL DEFAULT 0,
            weight REAL DEFAULT 0,
            is_purchasable INTEGER DEFAULT 1,
            is_sellable INTEGER DEFAULT 1,
            is_craftable INTEGER DEFAULT 0,
            pricing_group TEXT DEFAULT 'Base70',
            notes TEXT
        )
    )")) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to create items table:" << m_lastError;
        return false;
    }

    // Transactions table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL,
            type TEXT NOT NULL,
            account TEXT NOT NULL,
            item TEXT,
            category TEXT,
            quantity INTEGER DEFAULT 1,
            unit_price REAL DEFAULT 0,
            total_amount REAL DEFAULT 0,
            notes TEXT
        )
    )")) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to create transactions table:" << m_lastError;
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
        m_lastError = query.lastError().text();
        qWarning() << "Failed to create vehicles table:" << m_lastError;
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
        m_lastError = query.lastError().text();
        qWarning() << "Failed to create fuel_log table:" << m_lastError;
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
        m_lastError = query.lastError().text();
        qWarning() << "Failed to create movement_sessions table:" << m_lastError;
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
        m_lastError = query.lastError().text();
        qWarning() << "Failed to create movement_equipment_usage table:" << m_lastError;
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
        INSERT INTO items (code, name, category_main, category_sub,
                          buy_price_internal, buy_price_display,
                          sell_price_internal, sell_price_display,
                          weight, is_purchasable, is_sellable, is_craftable,
                          pricing_group, notes)
        VALUES (:code, :name, :category_main, :category_sub,
                :buy_price_internal, :buy_price_display,
                :sell_price_internal, :sell_price_display,
                :weight, :is_purchasable, :is_sellable, :is_craftable,
                :pricing_group, :notes)
    )");

    query.bindValue(":code", item.code);
    query.bindValue(":name", item.name);
    query.bindValue(":category_main", item.categoryMain);
    query.bindValue(":category_sub", item.categorySub);
    query.bindValue(":buy_price_internal", item.buyPriceInternal);
    query.bindValue(":buy_price_display", item.buyPriceDisplay);
    query.bindValue(":sell_price_internal", item.sellPriceInternal);
    query.bindValue(":sell_price_display", item.sellPriceDisplay);
    query.bindValue(":weight", item.weight);
    query.bindValue(":is_purchasable", item.isPurchasable ? 1 : 0);
    query.bindValue(":is_sellable", item.isSellable ? 1 : 0);
    query.bindValue(":is_craftable", item.isCraftable ? 1 : 0);
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
    item.categoryMain = query.value("category_main").toString();
    item.categorySub = query.value("category_sub").toString();
    item.buyPriceInternal = query.value("buy_price_internal").toDouble();
    item.buyPriceDisplay = query.value("buy_price_display").toDouble();
    item.sellPriceInternal = query.value("sell_price_internal").toDouble();
    item.sellPriceDisplay = query.value("sell_price_display").toDouble();
    item.weight = query.value("weight").toDouble();
    item.isPurchasable = query.value("is_purchasable").toInt() == 1;
    item.isSellable = query.value("is_sellable").toInt() == 1;
    item.isCraftable = query.value("is_craftable").toInt() == 1;
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
    item.categoryMain = query.value("category_main").toString();
    item.categorySub = query.value("category_sub").toString();
    item.buyPriceInternal = query.value("buy_price_internal").toDouble();
    item.buyPriceDisplay = query.value("buy_price_display").toDouble();
    item.sellPriceInternal = query.value("sell_price_internal").toDouble();
    item.sellPriceDisplay = query.value("sell_price_display").toDouble();
    item.weight = query.value("weight").toDouble();
    item.isPurchasable = query.value("is_purchasable").toInt() == 1;
    item.isSellable = query.value("is_sellable").toInt() == 1;
    item.isCraftable = query.value("is_craftable").toInt() == 1;
    item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
    item.notes = query.value("notes").toString();

    return item;
}

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
    item.categoryMain = query.value("category_main").toString();
    item.categorySub = query.value("category_sub").toString();
    item.buyPriceInternal = query.value("buy_price_internal").toDouble();
    item.buyPriceDisplay = query.value("buy_price_display").toDouble();
    item.sellPriceInternal = query.value("sell_price_internal").toDouble();
    item.sellPriceDisplay = query.value("sell_price_display").toDouble();
    item.weight = query.value("weight").toDouble();
    item.isPurchasable = query.value("is_purchasable").toInt() == 1;
    item.isSellable = query.value("is_sellable").toInt() == 1;
    item.isCraftable = query.value("is_craftable").toInt() == 1;
    item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
    item.notes = query.value("notes").toString();

    return item;
}

QVector<Item> Database::getAllItems()
{
    QVector<Item> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM items ORDER BY category_main, category_sub, name")) {
        m_lastError = query.lastError().text();
        qWarning() << "Failed to get items:" << m_lastError;
        return items;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("id").toInt();
        item.code = query.value("code").toString();
        item.name = query.value("name").toString();
        item.categoryMain = query.value("category_main").toString();
        item.categorySub = query.value("category_sub").toString();
        item.buyPriceInternal = query.value("buy_price_internal").toDouble();
        item.buyPriceDisplay = query.value("buy_price_display").toDouble();
        item.sellPriceInternal = query.value("sell_price_internal").toDouble();
        item.sellPriceDisplay = query.value("sell_price_display").toDouble();
        item.weight = query.value("weight").toDouble();
        item.isPurchasable = query.value("is_purchasable").toInt() == 1;
        item.isSellable = query.value("is_sellable").toInt() == 1;
        item.isCraftable = query.value("is_craftable").toInt() == 1;
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

    if (!query.exec("SELECT DISTINCT category_main FROM items WHERE category_main IS NOT NULL ORDER BY category_main")) {
        m_lastError = query.lastError().text();
        return categories;
    }

    while (query.next()) {
        QString category = query.value("category_main").toString();
        if (!category.isEmpty()) {
            categories.append(category);
        }
    }

    return categories;
}

QVector<QString> Database::getAllMainCategories()
{
    return getAllCategories();
}

QVector<QString> Database::getSubCategoriesFor(const QString &mainCategory)
{
    QVector<QString> subCategories;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT DISTINCT category_sub FROM items WHERE category_main = :main AND category_sub IS NOT NULL ORDER BY category_sub");
    query.bindValue(":main", mainCategory);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return subCategories;
    }

    while (query.next()) {
        QString subCat = query.value("category_sub").toString();
        if (!subCat.isEmpty()) {
            subCategories.append(subCat);
        }
    }

    return subCategories;
}

QVector<Item> Database::getItemsByCategory(const QString &category)
{
    QVector<Item> items;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM items WHERE category_main = :category ORDER BY name");
    query.bindValue(":category", category);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return items;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("id").toInt();
        item.code = query.value("code").toString();
        item.name = query.value("name").toString();
        item.categoryMain = query.value("category_main").toString();
        item.categorySub = query.value("category_sub").toString();
        item.buyPriceInternal = query.value("buy_price_internal").toDouble();
        item.buyPriceDisplay = query.value("buy_price_display").toDouble();
        item.sellPriceInternal = query.value("sell_price_internal").toDouble();
        item.sellPriceDisplay = query.value("sell_price_display").toDouble();
        item.weight = query.value("weight").toDouble();
        item.isPurchasable = query.value("is_purchasable").toInt() == 1;
        item.isSellable = query.value("is_sellable").toInt() == 1;
        item.isCraftable = query.value("is_craftable").toInt() == 1;
        item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
        item.notes = query.value("notes").toString();
        items.append(item);
    }

    return items;
}

bool Database::updateItem(const Item &item)
{
    if (!item.id.has_value()) {
        qWarning() << "Cannot update item without id";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE items SET
            code = :code,
            name = :name,
            category_main = :category_main,
            category_sub = :category_sub,
            buy_price_internal = :buy_price_internal,
            buy_price_display = :buy_price_display,
            sell_price_internal = :sell_price_internal,
            sell_price_display = :sell_price_display,
            weight = :weight,
            is_purchasable = :is_purchasable,
            is_sellable = :is_sellable,
            is_craftable = :is_craftable,
            pricing_group = :pricing_group,
            notes = :notes
        WHERE id = :id
    )");

    query.bindValue(":id", item.id.value());
    query.bindValue(":code", item.code);
    query.bindValue(":name", item.name);
    query.bindValue(":category_main", item.categoryMain);
    query.bindValue(":category_sub", item.categorySub);
    query.bindValue(":buy_price_internal", item.buyPriceInternal);
    query.bindValue(":buy_price_display", item.buyPriceDisplay);
    query.bindValue(":sell_price_internal", item.sellPriceInternal);
    query.bindValue(":sell_price_display", item.sellPriceDisplay);
    query.bindValue(":weight", item.weight);
    query.bindValue(":is_purchasable", item.isPurchasable ? 1 : 0);
    query.bindValue(":is_sellable", item.isSellable ? 1 : 0);
    query.bindValue(":is_craftable", item.isCraftable ? 1 : 0);
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
        m_lastError = query.lastError().text();
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
        INSERT INTO transactions (date, type, account, item, category,
                                  quantity, unit_price, total_amount, notes)
        VALUES (:date, :type, :account, :item, :category,
                :quantity, :unit_price, :total_amount, :notes)
    )");

    query.bindValue(":date", transaction.date.toString(Qt::ISODate));
    query.bindValue(":type", transactionTypeToString(transaction.type));
    query.bindValue(":account", accountTypeToString(transaction.account));
    query.bindValue(":item", transaction.item);
    query.bindValue(":category", transaction.category);
    query.bindValue(":quantity", transaction.quantity);
    query.bindValue(":unit_price", transaction.unitPrice);
    query.bindValue(":total_amount", transaction.totalAmount);
    query.bindValue(":notes", transaction.notes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
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

    Transaction trans;
    trans.id = query.value("id").toInt();
    trans.date = QDate::fromString(query.value("date").toString(), Qt::ISODate);
    trans.type = stringToTransactionType(query.value("type").toString());
    trans.account = stringToAccountType(query.value("account").toString());
    trans.item = query.value("item").toString();
    trans.category = query.value("category").toString();
    trans.quantity = query.value("quantity").toInt();
    trans.unitPrice = query.value("unit_price").toDouble();
    trans.totalAmount = query.value("total_amount").toDouble();
    trans.notes = query.value("notes").toString();

    return trans;
}

QVector<Transaction> Database::getAllTransactions()
{
    QVector<Transaction> transactions;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM transactions ORDER BY date DESC, id DESC")) {
        m_lastError = query.lastError().text();
        return transactions;
    }

    while (query.next()) {
        Transaction trans;
        trans.id = query.value("id").toInt();
        trans.date = QDate::fromString(query.value("date").toString(), Qt::ISODate);
        trans.type = stringToTransactionType(query.value("type").toString());
        trans.account = stringToAccountType(query.value("account").toString());
        trans.item = query.value("item").toString();
        trans.category = query.value("category").toString();
        trans.quantity = query.value("quantity").toInt();
        trans.unitPrice = query.value("unit_price").toDouble();
        trans.totalAmount = query.value("total_amount").toDouble();
        trans.notes = query.value("notes").toString();
        transactions.append(trans);
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
        m_lastError = query.lastError().text();
        return transactions;
    }

    while (query.next()) {
        Transaction trans;
        trans.id = query.value("id").toInt();
        trans.date = QDate::fromString(query.value("date").toString(), Qt::ISODate);
        trans.type = stringToTransactionType(query.value("type").toString());
        trans.account = stringToAccountType(query.value("account").toString());
        trans.item = query.value("item").toString();
        trans.category = query.value("category").toString();
        trans.quantity = query.value("quantity").toInt();
        trans.unitPrice = query.value("unit_price").toDouble();
        trans.totalAmount = query.value("total_amount").toDouble();
        trans.notes = query.value("notes").toString();
        transactions.append(trans);
    }

    return transactions;
}

bool Database::updateTransaction(const Transaction &transaction)
{
    if (!transaction.id.has_value()) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE transactions SET
            date = :date,
            type = :type,
            account = :account,
            item = :item,
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
    query.bindValue(":item", transaction.item);
    query.bindValue(":category", transaction.category);
    query.bindValue(":quantity", transaction.quantity);
    query.bindValue(":unit_price", transaction.unitPrice);
    query.bindValue(":total_amount", transaction.totalAmount);
    query.bindValue(":notes", transaction.notes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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

    Vehicle v;
    v.id = query.value("id").toString();
    v.name = query.value("name").toString();
    v.categoryMain = query.value("category_main").toString();
    v.categorySub = query.value("category_sub").toString();
    v.bucketCapacityM3 = query.value("bucket_capacity_m3").toDouble();
    v.truckCapacityM3 = query.value("truck_capacity_m3").toDouble();
    v.tankCapacityL = query.value("tank_capacity_l").toDouble();
    v.fuelUseLPerHour = query.value("fuel_use_l_per_hour").toDouble();
    v.purchasePrice = query.value("purchase_price").toDouble();
    v.active = query.value("active").toInt() == 1;
    v.notes = query.value("notes").toString();

    return v;
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
        m_lastError = query.lastError().text();
        return vehicles;
    }

    while (query.next()) {
        Vehicle v;
        v.id = query.value("id").toString();
        v.name = query.value("name").toString();
        v.categoryMain = query.value("category_main").toString();
        v.categorySub = query.value("category_sub").toString();
        v.bucketCapacityM3 = query.value("bucket_capacity_m3").toDouble();
        v.truckCapacityM3 = query.value("truck_capacity_m3").toDouble();
        v.tankCapacityL = query.value("tank_capacity_l").toDouble();
        v.fuelUseLPerHour = query.value("fuel_use_l_per_hour").toDouble();
        v.purchasePrice = query.value("purchase_price").toDouble();
        v.active = query.value("active").toInt() == 1;
        v.notes = query.value("notes").toString();
        vehicles.append(v);
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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
        SELECT COALESCE(SUM(liters), 0) as total FROM fuel_log
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
        SELECT COALESCE(SUM(total_cost), 0) as total FROM fuel_log
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
// Movement Session CRUD
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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

    // Check if entry exists
    query.prepare(R"(
        SELECT id FROM movement_equipment_usage
        WHERE session_id = :session_id AND equipment_id = :equipment_id AND role = :role
    )");
    query.bindValue(":session_id", usage.sessionId);
    query.bindValue(":equipment_id", usage.equipmentId);
    query.bindValue(":role", usage.role);

    if (query.exec() && query.next()) {
        // Update
        int existingId = query.value("id").toInt();
        query.prepare(R"(
            UPDATE movement_equipment_usage SET
                hours_used = :hours_used, buckets = :buckets,
                loads = :loads, dumps = :dumps, estimated_fuel_l = :estimated_fuel_l
            WHERE id = :id
        )");
        query.bindValue(":id", existingId);
    } else {
        // Insert
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
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QVector<MovementEquipmentUsage> Database::getEquipmentUsageForSession(int sessionId)
{
    QVector<MovementEquipmentUsage> usages;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM movement_equipment_usage WHERE session_id = :session_id ORDER BY id");
    query.bindValue(":session_id", sessionId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
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
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

// =============================================================================
// Helper Functions
// =============================================================================

QString pricingGroupToString(PricingGroup group)
{
    switch (group) {
    case PricingGroup::Base70: return "Base70";
    case PricingGroup::Custom: return "Custom";
    default: return "Base70";
    }
}

PricingGroup stringToPricingGroup(const QString &str)
{
    if (str == "Base70") return PricingGroup::Base70;
    if (str == "Custom") return PricingGroup::Custom;
    return PricingGroup::Base70;
}

QString transactionTypeToString(TransactionType type)
{
    switch (type) {
    case TransactionType::Sale: return "Sale";
    case TransactionType::Purchase: return "Purchase";
    case TransactionType::Transfer: return "Transfer";
    case TransactionType::Fuel: return "Fuel";
    default: return "Sale";
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
    case AccountType::Company: return "Company";
    case AccountType::Personal: return "Personal";
    default: return "Company";
    }
}

AccountType stringToAccountType(const QString &str)
{
    if (str == "Company") return AccountType::Company;
    if (str == "Personal") return AccountType::Personal;
    return AccountType::Company;
}

} // namespace Frontier
