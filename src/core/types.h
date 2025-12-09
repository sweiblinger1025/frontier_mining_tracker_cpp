/**
 * @file types.h
 * @brief Core data types for Frontier Mining Tracker
 */

#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <optional>

namespace Frontier {

// === Enums ===

enum class TransactionType {
    Sale,
    Purchase,
    Transfer,
    Fuel
};

enum class AccountType {
    Company,
    Personal
};

enum class PricingGroup {
    Base70,
    // Resource72,  // Commented out - all items use 70%
    // Special75,   // Commented out - all items use 70%
    Custom
};

enum class UnitSystem {
    Metric,     // m³, liters, km
    Imperial    // yd³, gallons, miles
};

// === Helper Functions ===

inline double calculateSellPrice(double buyPrice, PricingGroup group) {
    switch (group) {
    case PricingGroup::Base70:
        return buyPrice * 0.70;
    case PricingGroup::Custom:
    default:
        return buyPrice * 0.70;
    }
}

// === Data Structures ===

struct Item {
    std::optional<int> id;
    QString code;           // "100001"
    QString name;           // "Iron Ore"
    QString categoryMain;   // "Materials"
    QString categorySub;    // "Ores"
    double buyPriceInternal = 0;    // Internal fractional price
    double buyPriceDisplay = 0;     // Rounded display price
    double sellPriceInternal = 0;   // Fractional (for qty > 1)
    double sellPriceDisplay = 0;    // Rounded (for qty = 1)
    double weight = 0;
    PricingGroup pricingGroup = PricingGroup::Base70;
    bool isPurchasable = true;
    bool isSellable = true;
    bool isCraftable = false;
    QString notes;

    // For display
    QString displayCategory() const {
        if (categorySub.isEmpty() || categorySub.compare("None", Qt::CaseInsensitive) == 0) {
            return categoryMain;
        }
        return categoryMain + " - " + categorySub;
    }

    // Calculate ROI percentage (sell - buy) / buy * 100
    double roiPercent() const {
        if (buyPriceInternal <= 0) return 0;
        return ((sellPriceInternal - buyPriceInternal) / buyPriceInternal) * 100.0;
    }
};

struct Transaction {
    std::optional<int> id;
    QDate date;
    TransactionType type;
    AccountType account;
    QString item;
    QString category;
    int quantity;
    double unitPrice;
    double totalAmount;
    QString notes;

    // Helper to check if this is income (Sale) vs expense (Purchase/Fuel)
    bool isIncome() const {
        return type == TransactionType::Sale;
    }
};

struct Vehicle {
    QString id;                     // Primary key, e.g. "ARVIK_L9"
    QString name;                   // Display name, e.g. "Arvik L9"

    // Categories
    QString categoryMain;           // "Loader", "Excavator", "Rock Truck", "Drill"
    QString categorySub;            // "Wheel Loader", "Haul Truck", "Crawler"

    // Capacities (stored in METRIC)
    double bucketCapacityM3 = 0;    // For loaders/excavators (cubic meters)
    double truckCapacityM3 = 0;     // For haul trucks (cubic meters)

    // Fuel (stored in METRIC)
    double tankCapacityL = 0;       // Tank size in liters
    double fuelUseLPerHour = 0;     // Fuel consumption L/hr

    // Financial
    double purchasePrice = 0;

    // Status
    bool active = true;

    // Notes
    QString notes;
};

// === Operations Types ===

struct FuelLogEntry {
    std::optional<int> id;
    QDateTime dateTime;
    QString equipmentId;            // FK to Vehicle.id
    double liters = 0;
    double unitPrice = 0;
    double totalCost = 0;
    double meterOrHours = 0;
    QString source;                 // e.g. "On-site tank", "Gas Station"
    QString notes;
};

struct MovementSession {
    std::optional<int> id;
    QDateTime startTime;
    QDateTime endTime;              // Null until session closed
    QString mapName;
    QString notes;
};

struct MovementEquipmentUsage {
    std::optional<int> id;
    int sessionId = 0;              // FK to MovementSession.id
    QString equipmentId;            // FK to Vehicle.id
    QString role;                   // "Loader", "Excavator", "HaulTruck"
    double hoursUsed = 0;
    int buckets = 0;                // For loaders/excavators
    int loads = 0;                  // For haul trucks (full loads)
    int dumps = 0;                  // For haul trucks (dumps at hopper)
    double estimatedFuelL = 0;

    // Computed at runtime, not stored in DB
    double volumeM3 = 0;
};

// === Recipe Types ===

struct Workbench {
    std::optional<int> id;
    QString name;                   // "CNC Cutter", "Concrete Mixer", etc.
};

struct RecipeIngredient {
    std::optional<int> id;
    int recipeId = 0;
    QString itemName;               // Input item name
    int quantity = 1;
};

struct Recipe {
    std::optional<int> id;
    int workbenchId = 0;
    QString workbenchName;          // For display (not stored, loaded via join)
    QString outputItem;             // Output item name
    int outputQty = 1;
    QString notes;                  // e.g., "Rough Concrete", "Quest item"
    QVector<RecipeIngredient> ingredients;

    // Computed at runtime for cost analysis
    double inputCost = 0;
    double outputValue = 0;
    double profit = 0;
    double marginPercent = 0;
};

} // namespace Frontier

#endif // TYPES_H
