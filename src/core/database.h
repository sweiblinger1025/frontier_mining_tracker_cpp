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

    bool initialize(const QString &dbPath);
    void close();
    bool isOpen() const;
    QString lastError() const;

    // === Item CRUD ===
    bool addItem(const Item &item);
    std::optional<Item> getItem(int id);
    std::optional<Item> getItemByCode(const QString &code);
    std::optional<Item> getItemByName(const QString &name);
    QVector<Item> getAllItems();
    QVector<QString> getAllCategories();
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

    // === Workbench CRUD ===
    int addWorkbench(const Workbench &workbench);
    std::optional<Workbench> getWorkbench(int id);
    std::optional<Workbench> getWorkbenchByName(const QString &name);
    QVector<Workbench> getAllWorkbenches();
    bool deleteWorkbench(int id);
    bool clearAllWorkbenches();

    // === Recipe CRUD ===
    int addRecipe(const Recipe &recipe);
    std::optional<Recipe> getRecipe(int id);
    QVector<Recipe> getAllRecipes();
    QVector<Recipe> getRecipesByWorkbench(int workbenchId);
    QVector<Recipe> getRecipesByWorkbenchName(const QString &workbenchName);
    QVector<Recipe> getRecipesForOutput(const QString &outputItem);
    bool deleteRecipe(int id);
    bool clearAllRecipes();

    // === Recipe Ingredient CRUD ===
    bool addRecipeIngredient(const RecipeIngredient &ingredient);
    QVector<RecipeIngredient> getIngredientsForRecipe(int recipeId);
    bool deleteIngredientsForRecipe(int recipeId);

    // === Location Tables ===
    bool createLocationTables();

    // === Map CRUD ===
    int addMap(const Map &map);
    std::optional<Map> getMap(int id);
    std::optional<Map> getMapByAbbrev(const QString &abbrev);
    QVector<Map> getAllMaps();
    bool deleteMap(int id);
    bool clearAllMaps();

    // === LocationType CRUD ===
    int addLocationType(const LocationType &type);
    std::optional<LocationType> getLocationType(int id);
    std::optional<LocationType> getLocationTypeByName(const QString &name);
    QVector<LocationType> getAllLocationTypes();
    bool deleteLocationType(int id);
    bool clearAllLocationTypes();

    // === Location CRUD ===
    int addLocation(const Location &location);
    std::optional<Location> getLocation(int id);
    QVector<Location> getAllLocations();
    QVector<Location> getLocationsByMap(int mapId);
    QVector<Location> getLocationsByType(int typeId);
    QVector<Location> getLocationsByMapAndType(int mapId, int typeId);
    bool deleteLocation(int id);
    bool clearAllLocations();

    // === Inventory Tables ===
    bool createInventoryTables();

    // === Inventory CRUD ===
    int addInventoryItem(const InventoryItem &item);
    std::optional<InventoryItem> getInventoryItem(int id);
    std::optional<InventoryItem> getInventoryByItemId(int itemId);
    std::optional<InventoryItem> getInventoryByItemName(const QString &itemName);
    QVector<InventoryItem> getAllInventory();
    QVector<InventoryItem> getInventoryByCategory(const QString &category);
    QVector<InventoryItem> getInventoryByLocation(int locationId);
    QVector<InventoryItem> getInventoryWithStock();  // quantity > 0
    bool updateInventoryQuantity(int id, int newQuantity);
    bool adjustInventoryQuantity(int id, int delta);  // Add/subtract
    bool updateInventoryItem(const InventoryItem &item);
    bool deleteInventoryItem(int id);
    bool clearAllInventory();

    // === Oil Tracking ===
    OilTracking getOilTracking();
    bool saveOilTracking(const OilTracking &tracking);
    bool addOilSold(int quantity);  // Increment totalOilSold
    bool resetOilTracking();

    // === Production Runs Table ===
    bool createProductionRunsTable();

    // === Production Runs CRUD ===
    int addProductionRun(const ProductionRun &run);
    std::optional<ProductionRun> getProductionRun(int id);
    QVector<ProductionRun> getAllProductionRuns();
    QVector<ProductionRun> getProductionRunsByDateRange(const QDateTime &from, const QDateTime &to);
    QVector<ProductionRun> getProductionRunsByRecipe(int recipeId);
    bool updateProductionRun(const ProductionRun &run);
    bool deleteProductionRun(int id);
    bool clearAllProductionRuns();

    // === Production Statistics ===
    int getTotalProductionRuns();
    double getTotalValueCreated();

    // === Shifts Table ===
    bool createShiftsTable();

    // === Shifts CRUD ===
    int addShift(const Shift &shift);
    std::optional<Shift> getShift(int id);
    QVector<Shift> getAllShifts();
    QVector<Shift> getShiftsByDateRange(const QDate &from, const QDate &to);
    bool updateShift(const Shift &shift);
    bool deleteShift(int id);
    bool clearAllShifts();

    // === Shift Statistics ===
    int getTotalShiftCount();
    int getTotalShiftMinutes();

    // === Cycle Time Tables ===
    bool createCycleProfilesTable();
    bool createCycleRecordsTable();

    // === Cycle Profiles CRUD ===
    int addCycleProfile(const CycleProfile &profile);
    std::optional<CycleProfile> getCycleProfile(int id);
    QVector<CycleProfile> getAllCycleProfiles();
    QVector<CycleProfile> getCycleProfilesWithStats();
    bool updateCycleProfile(const CycleProfile &profile);
    bool deleteCycleProfile(int id);

    // === Cycle Records CRUD ===
    int addCycleRecord(const CycleRecord &record);
    std::optional<CycleRecord> getCycleRecord(int id);
    QVector<CycleRecord> getAllCycleRecords();
    QVector<CycleRecord> getCycleRecordsByProfile(int profileId);
    bool updateCycleRecord(const CycleRecord &record);
    bool deleteCycleRecord(int id);
    bool clearCycleRecordsByProfile(int profileId);

    // === Finance Calculations ===
    AccountBalance calculateBalances();
    AccountBalance calculateBalancesAsOf(const QDate &date);
    FinanceSummary getFinanceSummary(const QDate &from, const QDate &to);
    FinanceSummary getFinanceSummaryByAccount(const QDate &from, const QDate &to, AccountType account);

    // === Budget Table ===
    bool createBudgetsTable();
    int addBudget(const Budget &budget);
    std::optional<Budget> getBudget(int id);
    QVector<Budget> getBudgetsForMonth(int year, int month);
    QVector<Budget> getAllBudgets();
    bool updateBudget(const Budget &budget);
    bool deleteBudget(int id);

    // === Finance Settings ===
    double getAutoSplitCompanyPercent();  // Default 90%
    void setAutoSplitCompanyPercent(double percent);

    // === Factory Buildings Table ===
    bool createFactoryBuildingsTable();
    int addFactoryBuilding(const FactoryBuilding &building);
    std::optional<FactoryBuilding> getFactoryBuilding(int id);
    std::optional<FactoryBuilding> getFactoryBuildingByName(const QString &name);
    QVector<FactoryBuilding> getAllFactoryBuildings();
    QVector<FactoryBuilding> getFactoryBuildingsByCategory(const QString &category);
    bool updateFactoryBuilding(const FactoryBuilding &building);
    bool deleteFactoryBuilding(int id);

    // Convenience getters
    QVector<FactoryBuilding> getConveyors();
    QVector<FactoryBuilding> getPipelines();
    QVector<FactoryBuilding> getPowerEquipment();
    QVector<FactoryBuilding> getProductionBuildings();
    QVector<FactoryBuilding> getGenerators();  // Items with generatedKw > 0

    // === Equipment Plan ===
    bool createEquipmentPlanTable();
    int addEquipmentPlanItem(const EquipmentPlanItem &item);
    std::optional<EquipmentPlanItem> getEquipmentPlanItem(int id);
    std::optional<EquipmentPlanItem> getEquipmentPlanItemByItemId(int itemId);
    QVector<EquipmentPlanItem> getEquipmentPlan();
    bool updateEquipmentPlanItem(const EquipmentPlanItem &item);
    bool deleteEquipmentPlanItem(int id);
    void clearEquipmentPlan();

    // === Facility Plan ===
    bool createFacilityPlanTable();
    int addFacilityPlanItem(const FacilityPlanItem &item);
    std::optional<FacilityPlanItem> getFacilityPlanItem(int id);
    std::optional<FacilityPlanItem> getFacilityPlanItemByBuildingId(int buildingId);
    QVector<FacilityPlanItem> getFacilityPlan();
    bool updateFacilityPlanItem(const FacilityPlanItem &item);
    bool deleteFacilityPlanItem(int id);
    void clearFacilityPlan();

private:
    bool createTables();
    bool createVehiclesTable();
    bool createRecipeTables();


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
