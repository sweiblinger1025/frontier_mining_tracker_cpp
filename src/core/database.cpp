/**
 * @file database.cpp
 * @brief Implementation of SQLite database manager
 */

#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace Frontier {

// Static counter for unique connection names
static int s_connectionCounter = 0;

// === Constructor / Destructor ===

Database::Database()
    : m_connected(false)
{
    m_connectionName = QString("FrontierDB_%1").arg(++s_connectionCounter);
}

Database::~Database()
{
    disconnect();
}

// === Connection Management ===

bool Database::connect(const QString &path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(path);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }

    m_connected = true;
    return true;
}

void Database::disconnect()
{
    if (m_connected) {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connected = false;
    }
}

bool Database::isConnected() const
{
    return m_connected;
}

QString Database::lastError() const
{
    return m_lastError;
}

// === Schema Setup ===

bool Database::createTables()
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    // Items table
    bool success = query.exec(R"(
        CREATE TABLE IF NOT EXISTS items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            category_main TEXT,
            category_sub TEXT,
            buy_price_internal REAL,
            sell_price_internal REAL,
            buy_price_display INTEGER,
            sell_price_display INTEGER,
            is_sellable INTEGER DEFAULT 0,
            is_purchasable INTEGER DEFAULT 0,
            is_craftable INTEGER DEFAULT 0,
            pricing_group TEXT DEFAULT 'Base70',
            notes TEXT
        )
    )");

    if (!success) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Transactions table
    success = query.exec(R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL,
            type TEXT NOT NULL,
            account TEXT NOT NULL,
            item TEXT,
            category TEXT,
            quantity INTEGER,
            unit_price REAL,
            total_amount REAL
        )
    )");

    if (!success) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Vehicles table
    success = query.exec(R"(
        CREATE TABLE IF NOT EXISTS vehicles (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT,
            purchase_price REAL,
            fuel_capacity REAL,
            fuel_consumption REAL,
            notes TEXT
        )
    )");

    if (!success) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Inventory table
    success = query.exec(R"(
        CREATE TABLE IF NOT EXISTS inventory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            category TEXT,
            quantity INTEGER DEFAULT 0
        )
    )");

    if (!success) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

// === Helper Functions ===

QString Database::pricingGroupToString(PricingGroup group) const
{
    switch (group) {
    case PricingGroup::Base70: return "Base70";
    // case PricingGroup::Resource72: return "Resource72";
    // case PricingGroup::Special75: return "Special75";
    case PricingGroup::Custom: return "Custom";
    default: return "Base70";
    }
}

PricingGroup Database::stringToPricingGroup(const QString &str) const
{
    // if (str == "Resource72") return PricingGroup::Resource72;
    // if (str == "Special75") return PricingGroup::Special75;
    if (str == "Custom") return PricingGroup::Custom;
    return PricingGroup::Base70;
}

// === Item CRUD ===

bool Database::addItem(const Item &item)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO items (name, category_main, category_sub, buy_price_internal,
            sell_price_internal, buy_price_display, sell_price_display,
            is_sellable, is_purchasable, is_craftable, pricing_group, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(item.name);
    query.addBindValue(item.categoryMain);
    query.addBindValue(item.categorySub);
    query.addBindValue(item.buyPriceInternal);
    query.addBindValue(item.sellPriceInternal);
    query.addBindValue(item.buyPriceDisplay);
    query.addBindValue(item.sellPriceDisplay);
    query.addBindValue(item.isSellable ? 1 : 0);
    query.addBindValue(item.isPurchasable ? 1 : 0);
    query.addBindValue(item.isCraftable ? 1 : 0);
    query.addBindValue(pricingGroupToString(item.pricingGroup));
    query.addBindValue(item.notes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Item> Database::getItem(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return std::nullopt;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM items WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    Item item;
    item.id = query.value("id").toInt();
    item.name = query.value("name").toString();
    item.categoryMain = query.value("category_main").toString();
    item.categorySub = query.value("category_sub").toString();
    item.buyPriceInternal = query.value("buy_price_internal").toDouble();
    item.sellPriceInternal = query.value("sell_price_internal").toDouble();
    item.buyPriceDisplay = query.value("buy_price_display").toInt();
    item.sellPriceDisplay = query.value("sell_price_display").toInt();
    item.isSellable = query.value("is_sellable").toBool();
    item.isPurchasable = query.value("is_purchasable").toBool();
    item.isCraftable = query.value("is_craftable").toBool();
    item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
    item.notes = query.value("notes").toString();

    return item;
}

QVector<Item> Database::getAllItems()
{
    QVector<Item> items;

    if (!m_connected) {
        m_lastError = "Not connected to database";
        return items;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM items ORDER BY category_main, category_sub, name")) {
        m_lastError = query.lastError().text();
        return items;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("id").toInt();
        item.name = query.value("name").toString();
        item.categoryMain = query.value("category_main").toString();
        item.categorySub = query.value("category_sub").toString();
        item.buyPriceInternal = query.value("buy_price_internal").toDouble();
        item.sellPriceInternal = query.value("sell_price_internal").toDouble();
        item.buyPriceDisplay = query.value("buy_price_display").toInt();
        item.sellPriceDisplay = query.value("sell_price_display").toInt();
        item.isSellable = query.value("is_sellable").toBool();
        item.isPurchasable = query.value("is_purchasable").toBool();
        item.isCraftable = query.value("is_craftable").toBool();
        item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
        item.notes = query.value("notes").toString();
        items.append(item);
    }

    return items;
}

QVector<Item> Database::getItemsByCategory(const QString &categoryMain)
{
    QVector<Item> items;

    if (!m_connected) {
        m_lastError = "Not connected to database";
        return items;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM items WHERE category_main = ? ORDER BY category_sub, name");
    query.addBindValue(categoryMain);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return items;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("id").toInt();
        item.name = query.value("name").toString();
        item.categoryMain = query.value("category_main").toString();
        item.categorySub = query.value("category_sub").toString();
        item.buyPriceInternal = query.value("buy_price_internal").toDouble();
        item.sellPriceInternal = query.value("sell_price_internal").toDouble();
        item.buyPriceDisplay = query.value("buy_price_display").toInt();
        item.sellPriceDisplay = query.value("sell_price_display").toInt();
        item.isSellable = query.value("is_sellable").toBool();
        item.isPurchasable = query.value("is_purchasable").toBool();
        item.isCraftable = query.value("is_craftable").toBool();
        item.pricingGroup = stringToPricingGroup(query.value("pricing_group").toString());
        item.notes = query.value("notes").toString();
        items.append(item);
    }

    return items;
}

bool Database::updateItem(const Item &item)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    if (!item.id.has_value()) {
        m_lastError = "Item has no ID";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE items SET
            name = ?, category_main = ?, category_sub = ?,
            buy_price_internal = ?, sell_price_internal = ?,
            buy_price_display = ?, sell_price_display = ?,
            is_sellable = ?, is_purchasable = ?, is_craftable = ?,
            pricing_group = ?, notes = ?
        WHERE id = ?
    )");

    query.addBindValue(item.name);
    query.addBindValue(item.categoryMain);
    query.addBindValue(item.categorySub);
    query.addBindValue(item.buyPriceInternal);
    query.addBindValue(item.sellPriceInternal);
    query.addBindValue(item.buyPriceDisplay);
    query.addBindValue(item.sellPriceDisplay);
    query.addBindValue(item.isSellable ? 1 : 0);
    query.addBindValue(item.isPurchasable ? 1 : 0);
    query.addBindValue(item.isCraftable ? 1 : 0);
    query.addBindValue(pricingGroupToString(item.pricingGroup));
    query.addBindValue(item.notes);
    query.addBindValue(item.id.value());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool Database::deleteItem(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM items WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

// === Transaction CRUD ===

bool Database::addTransaction(const Transaction &transaction)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO transactions (date, type, account, item, category,
            quantity, unit_price, total_amount)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    // Convert enums to strings for storage
    QString typeStr;
    switch (transaction.type) {
    case TransactionType::Sale: typeStr = "Sale"; break;
    case TransactionType::Purchase: typeStr = "Purchase"; break;
    case TransactionType::Transfer: typeStr = "Transfer"; break;
    case TransactionType::Fuel: typeStr = "Fuel"; break;
    }

    QString accountStr = (transaction.account == AccountType::Personal) ? "Personal" : "Company";

    query.addBindValue(transaction.date.toString("yyyy-MM-dd"));
    query.addBindValue(typeStr);
    query.addBindValue(accountStr);
    query.addBindValue(transaction.item);
    query.addBindValue(transaction.category);
    query.addBindValue(transaction.quantity);
    query.addBindValue(transaction.unitPrice);
    query.addBindValue(transaction.totalAmount);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Frontier::Transaction> Frontier::Database::getTransaction(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return std::nullopt;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM transactions WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Transaction trans;
    trans.id = query.value("id").toInt();
    trans.date = QDate::fromString(query.value("date").toString(), "yyyy-MM-dd");

    // Convert strings to enums
    QString typeStr = query.value("type").toString();
    if (typeStr == "Sale") trans.type = TransactionType::Sale;
    else if (typeStr == "Purchase") trans.type = TransactionType::Purchase;
    else if (typeStr == "Transfer") trans.type = TransactionType::Transfer;
    else trans.type = TransactionType::Fuel;

    QString accountStr = query.value("account").toString();
    trans.account = (accountStr == "Company") ? AccountType::Company : AccountType::Personal;

    trans.item = query.value("item").toString();
    trans.category = query.value("category").toString();
    trans.quantity = query.value("quantity").toInt();
    trans.unitPrice = query.value("unit_price").toDouble();
    trans.totalAmount = query.value("total_amount").toDouble();

    return trans;
}

QVector<Transaction> Database::getAllTransactions()
{
    QVector<Transaction> transactions;

    if (!m_connected) {
        m_lastError = "Not connected to database";
        return transactions;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM transactions ORDER BY date DESC, id DESC")) {
        m_lastError = query.lastError().text();
        return transactions;
    }

    while (query.next()) {
        Transaction t;
        t.id = query.value("id").toInt();
        t.date = QDate::fromString(query.value("date").toString(), "yyyy-MM-dd");

        QString typeStr = query.value("type").toString();
        if (typeStr == "Sale") t.type = TransactionType::Sale;
        else if (typeStr == "Purchase") t.type = TransactionType::Purchase;
        else if (typeStr == "Transfer") t.type = TransactionType::Transfer;
        else t.type = TransactionType::Fuel;

        t.account = (query.value("account").toString() == "Personal")
                        ? AccountType::Personal : AccountType::Company;

        t.item = query.value("item").toString();
        t.category = query.value("category").toString();
        t.quantity = query.value("quantity").toInt();
        t.unitPrice = query.value("unit_price").toDouble();
        t.totalAmount = query.value("total_amount").toDouble();

        transactions.append(t);
    }

    return transactions;
}

QVector<Transaction> Database::getTransactionsByDateRange(const QDate &from, const QDate &to)
{
    QVector<Transaction> transactions;

    if (!m_connected) {
        m_lastError = "Not connected to database";
        return transactions;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM transactions WHERE date >= ? AND date <= ? ORDER BY date DESC, id DESC");
    query.addBindValue(from.toString("yyyy-MM-dd"));
    query.addBindValue(to.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return transactions;
    }

    while (query.next()) {
        Transaction t;
        t.id = query.value("id").toInt();
        t.date = QDate::fromString(query.value("date").toString(), "yyyy-MM-dd");

        QString typeStr = query.value("type").toString();
        if (typeStr == "Sale") t.type = TransactionType::Sale;
        else if (typeStr == "Purchase") t.type = TransactionType::Purchase;
        else if (typeStr == "Transfer") t.type = TransactionType::Transfer;
        else t.type = TransactionType::Fuel;

        t.account = (query.value("account").toString() == "Personal")
                        ? AccountType::Personal : AccountType::Company;

        t.item = query.value("item").toString();
        t.category = query.value("category").toString();
        t.quantity = query.value("quantity").toInt();
        t.unitPrice = query.value("unit_price").toDouble();
        t.totalAmount = query.value("total_amount").toDouble();

        transactions.append(t);
    }

    return transactions;
}

bool Database::updateTransaction(const Transaction &transaction)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    if (!transaction.id.has_value()) {
        m_lastError = "Transaction has no ID";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE transactions SET
            date = ?, type = ?, account = ?, item = ?, category = ?,
            quantity = ?, unit_price = ?, total_amount = ?
        WHERE id = ?
    )");

    QString typeStr;
    switch (transaction.type) {
    case TransactionType::Sale: typeStr = "Sale"; break;
    case TransactionType::Purchase: typeStr = "Purchase"; break;
    case TransactionType::Transfer: typeStr = "Transfer"; break;
    case TransactionType::Fuel: typeStr = "Fuel"; break;
    }

    QString accountStr = (transaction.account == AccountType::Personal) ? "Personal" : "Company";

    query.addBindValue(transaction.date.toString("yyyy-MM-dd"));
    query.addBindValue(typeStr);
    query.addBindValue(accountStr);
    query.addBindValue(transaction.item);
    query.addBindValue(transaction.category);
    query.addBindValue(transaction.quantity);
    query.addBindValue(transaction.unitPrice);
    query.addBindValue(transaction.totalAmount);
    query.addBindValue(transaction.id.value());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool Database::deleteTransaction(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM transactions WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

// === Vehicle CRUD ===

bool Database::addVehicle(const Vehicle &vehicle)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO vehicles (name, type, purchase_price, fuel_capacity, fuel_consumption, notes)
        VALUES (?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(vehicle.name);
    query.addBindValue(vehicle.type);
    query.addBindValue(vehicle.purchasePrice);
    query.addBindValue(vehicle.fuelCapacity);
    query.addBindValue(vehicle.fuelConsumption);
    query.addBindValue(vehicle.notes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Vehicle> Database::getVehicle(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return std::nullopt;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM vehicles WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    Vehicle v;
    v.id = query.value("id").toInt();
    v.name = query.value("name").toString();
    v.type = query.value("type").toString();
    v.purchasePrice = query.value("purchase_price").toDouble();
    v.fuelCapacity = query.value("fuel_capacity").toDouble();
    v.fuelConsumption = query.value("fuel_consumption").toDouble();
    v.notes = query.value("notes").toString();

    return v;
}

QVector<Vehicle> Database::getAllVehicles()
{
    QVector<Vehicle> vehicles;

    if (!m_connected) {
        m_lastError = "Not connected to database";
        return vehicles;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM vehicles ORDER BY type, name")) {
        m_lastError = query.lastError().text();
        return vehicles;
    }

    while (query.next()) {
        Vehicle v;
        v.id = query.value("id").toInt();
        v.name = query.value("name").toString();
        v.type = query.value("type").toString();
        v.purchasePrice = query.value("purchase_price").toDouble();
        v.fuelCapacity = query.value("fuel_capacity").toDouble();
        v.fuelConsumption = query.value("fuel_consumption").toDouble();
        v.notes = query.value("notes").toString();
        vehicles.append(v);
    }

    return vehicles;
}

bool Database::updateVehicle(const Vehicle &vehicle)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    if (!vehicle.id.has_value()) {
        m_lastError = "Vehicle has no ID";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE vehicles SET
            name = ?, type = ?, purchase_price = ?,
            fuel_capacity = ?, fuel_consumption = ?, notes = ?
        WHERE id = ?
    )");

    query.addBindValue(vehicle.name);
    query.addBindValue(vehicle.type);
    query.addBindValue(vehicle.purchasePrice);
    query.addBindValue(vehicle.fuelCapacity);
    query.addBindValue(vehicle.fuelConsumption);
    query.addBindValue(vehicle.notes);
    query.addBindValue(vehicle.id.value());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool Database::deleteVehicle(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM vehicles WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

// === Inventory CRUD ===

bool Database::addInventoryItem(const InventoryItem &item)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO inventory (name, category, quantity)
        VALUES (?, ?, ?)
    )");

    query.addBindValue(item.name);
    query.addBindValue(item.category);
    query.addBindValue(item.quantity);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<InventoryItem> Database::getInventoryItem(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return std::nullopt;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("SELECT * FROM inventory WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    InventoryItem item;
    item.id = query.value("id").toInt();
    item.name = query.value("name").toString();
    item.category = query.value("category").toString();
    item.quantity = query.value("quantity").toInt();

    return item;
}

QVector<InventoryItem> Database::getAllInventory()
{
    QVector<InventoryItem> inventory;

    if (!m_connected) {
        m_lastError = "Not connected to database";
        return inventory;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM inventory ORDER BY category, name")) {
        m_lastError = query.lastError().text();
        return inventory;
    }

    while (query.next()) {
        InventoryItem item;
        item.id = query.value("id").toInt();
        item.name = query.value("name").toString();
        item.category = query.value("category").toString();
        item.quantity = query.value("quantity").toInt();
        inventory.append(item);
    }

    return inventory;
}

bool Database::updateInventoryItem(const InventoryItem &item)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    if (!item.id.has_value()) {
        m_lastError = "Inventory item has no ID";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE inventory SET name = ?, category = ?, quantity = ?
        WHERE id = ?
    )");

    query.addBindValue(item.name);
    query.addBindValue(item.category);
    query.addBindValue(item.quantity);
    query.addBindValue(item.id.value());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool Database::deleteInventoryItem(int id)
{
    if (!m_connected) {
        m_lastError = "Not connected to database";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    query.prepare("DELETE FROM inventory WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

} // namespace Frontier
