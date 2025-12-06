/**
 * @file types.h
 * @brief Core data types for Frontier Mining Tracker
 *
 * Contains all enums, structs, and helper functions used
 * throughout the application.
 *
 * @author Stephen
 * @date December 2025
 */

#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QDate>
#include <QVector>
#include <optional>
#include <cmath>

namespace Frontier {

// === ENUMS ===

/**
 * @brief Transaction types for the ledger
 */
enum class TransactionType {
    Sale,
    Purchase,
    Transfer,
    Fuel
};

/**
 * @brief Account types for transactions
 */
enum class AccountType {
    Personal,
    Company
};

/**
 * @brief Game mode affects pricing and mechanics
 */
enum class GameMode {
    Normal,
    Hard
};

/**
 * @brief Concrete quality tiers
 */
enum class ConcreteQuality {
    Regular,        // Standard concrete
    Rough,          // Rough concrete
    Polished        // Higher quality, higher price
};

/**
 * @brief Stock status for inventory items
 */
enum class StockStatus {
    InStock,
    LowStock,
    OutOfStock
};

/**
 * @brief Pricing group based on sell percentage
 */
enum class PricingGroup {
    Base70,         // Standard 70% sell rate (most items)
    Resource72,     // Ores/Resources ~72-73%
    Special50,      // Water (50%)
    Custom          // Manually specified (doesn't fit pattern)
};

// TODO: Add MaintenanceType enum when maintenance tracking is needed

// === HELPER FUNCTIONS ===

/**
 * @brief Convert internal price to display price (game UI rounding)
 * @param internal The exact internal price
 * @return Rounded integer for display
 */
inline int toDisplayPrice(double internal) {
    return static_cast<int>(std::round(internal));
}

/**
 * @brief Calculate sell price based on pricing group
 * @param buyPrice The buy price
 * @param group The pricing group
 * @return Calculated internal sell price
 */
inline double calculateSellPrice(double buyPrice, PricingGroup group) {
    switch (group) {
    case PricingGroup::Base70:
        return buyPrice * 0.70;
    case PricingGroup::Resource72:
        return buyPrice * 0.72;
    case PricingGroup::Special50:
        return buyPrice * 0.50;
    case PricingGroup::Custom:
    default:
        return buyPrice * 0.70;  // Fallback to 70%
    }
}

// === STRUCTS ===

/**
 * @brief A financial transaction record
 */
struct Transaction {
    std::optional<int> id;      // Database ID (nullopt if not saved yet)
    QDate date;                 // When the transaction occurred
    TransactionType type;       // Sale, Purchase, Transfer, Fuel
    AccountType account;        // Personal or Company
    QString item;               // Item name (e.g., "Fast Pickaxe")
    QString category;           // Auto-filled from item lookup
    int quantity;               // Number of items
    double unitPrice;           // Price per item
    double totalAmount;         // quantity * unitPrice

    // Calculated helper
    bool isIncome() const {
        return type == TransactionType::Sale;
    }
};

/**
 * @brief An item in the reference database
 */
struct Item {
    std::optional<int> id;
    QString name;
    QString categoryMain;
    QString categorySub;

    // Internal prices (exact, for calculations)
    double buyPriceInternal;        // Exact buy price
    double sellPriceInternal;       // Exact sell price (can be fractional)

    // Display prices (what game UI shows)
    int buyPriceDisplay;            // Rounded for display
    int sellPriceDisplay;           // Rounded for display

    bool isSellable;
    bool isPurchasable;
    bool isCraftable;

    PricingGroup pricingGroup;
    QString notes;

    // === Helper Methods ===

    QString displayCategory() const {
        return categoryMain + " - " + categorySub;
    }

    // Calculate current buy with skill discount (uses internal)
    double currentBuyPrice(double discountPercent = 0.0) const {
        return buyPriceInternal * (1.0 - discountPercent / 100.0);
    }

    // Margin using internal prices
    double margin(double discountPercent = 0.0) const {
        return sellPriceInternal - currentBuyPrice(discountPercent);
    }

    // ROI using internal prices
    double roiPercent(double discountPercent = 0.0) const {
        double buyPrice = currentBuyPrice(discountPercent);
        if (buyPrice == 0) return 0.0;
        return (margin(discountPercent) / buyPrice) * 100.0;
    }

    // Sell ratio (internal)
    double sellRatio() const {
        if (buyPriceInternal == 0) return 0.0;
        return sellPriceInternal / buyPriceInternal;
    }

    // Update display prices from internal
    void updateDisplayPrices() {
        buyPriceDisplay = toDisplayPrice(buyPriceInternal);
        sellPriceDisplay = toDisplayPrice(sellPriceInternal);
    }

    // Calculate and set sell price from pricing group
    void calculateSellFromGroup() {
        sellPriceInternal = calculateSellPrice(buyPriceInternal, pricingGroup);
        updateDisplayPrices();
    }
};

/**
 * @brief A vehicle in the fleet
 */
struct Vehicle {
    std::optional<int> id;
    QString name;               // "Avrik DX20E"
    QString type;               // "Truck", "Excavator", etc.
    double purchasePrice;
    double fuelCapacity;
    double fuelConsumption;     // Per hour or per trip
    QString notes;
};

/**
 * @brief An ingredient for a recipe
 */
struct RecipeInput {
    QString item;               // "Iron Ore"
    int quantity;               // How many needed
};

/**
 * @brief A crafting recipe
 */
struct Recipe {
    std::optional<int> id;
    QString name;               // "Wood Beam Recipe"
    QString outputItem;         // What it produces
    int outputQuantity;         // How many it produces
    QVector<RecipeInput> inputs; // Required ingredients
    QString workbench;          // Which workbench is needed
    double craftTime;           // Time to craft (seconds/minutes)
};

/**
 * @brief An item in the player's inventory
 */
struct InventoryItem {
    std::optional<int> id;
    QString name;               // Links to Item name
    QString category;           // Auto-filled from Item
    int quantity;               // How many in stock

    // Helper to get stock status
    StockStatus stockStatus() const {
        if (quantity <= 0) return StockStatus::OutOfStock;
        if (quantity < 10) return StockStatus::LowStock;
        return StockStatus::InStock;
    }
};

} // namespace Frontier

#endif // TYPES_H
