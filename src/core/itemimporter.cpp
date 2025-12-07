/**
 * @file itemimporter.cpp
 * @brief Item import utility implementation
 */

#include "itemimporter.h"
#include "database.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <cmath>

namespace Frontier {

int ItemImporter::importFromJson(const QString &jsonPath, Database *database, bool clearExisting)
{
    QVector<Item> items = loadFromJson(jsonPath);
    
    if (items.isEmpty()) {
        return -1;
    }
    
    // Clear existing items if requested
    if (clearExisting) {
        // Get all items and delete them
        auto existingItems = database->getAllItems();
        for (const auto &item : existingItems) {
            if (item.id.has_value()) {
                database->deleteItem(item.id.value());
            }
        }
    }
    
    // Import items
    int importedCount = 0;
    for (const auto &item : items) {
        if (database->addItem(item)) {
            importedCount++;
        } else {
            qWarning() << "Failed to import item:" << item.name;
        }
    }
    
    return importedCount;
}

QVector<Item> ItemImporter::loadFromJson(const QString &jsonPath)
{
    QVector<Item> items;
    
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file:" << jsonPath;
        return items;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return items;
    }
    
    if (!doc.isArray()) {
        qWarning() << "JSON root is not an array";
        return items;
    }
    
    QJsonArray array = doc.array();
    
    for (const QJsonValue &value : array) {
        if (!value.isObject()) continue;
        
        QJsonObject obj = value.toObject();
        
        Item item;
        
        // Basic fields
        item.code = obj["code"].toString();
        item.name = obj["name"].toString();
        
        // Categories - handle both old and new field names
        item.categoryMain = obj["category_main"].toString();
        if (item.categoryMain.isEmpty()) {
            item.categoryMain = obj["categoryMain"].toString();
        }
        if (item.categoryMain.isEmpty()) {
            item.categoryMain = obj["category"].toString();
        }
        
        item.categorySub = obj["category_sub"].toString();
        if (item.categorySub.isEmpty()) {
            item.categorySub = obj["categorySub"].toString();
        }
        
        // Prices - handle both old and new field names
        if (obj.contains("buy_price_internal")) {
            item.buyPriceInternal = obj["buy_price_internal"].toDouble();
        } else if (obj.contains("buyPriceInternal")) {
            item.buyPriceInternal = obj["buyPriceInternal"].toDouble();
        } else if (obj.contains("buyPrice")) {
            item.buyPriceInternal = obj["buyPrice"].toDouble();
        } else if (obj.contains("buy_price")) {
            item.buyPriceInternal = obj["buy_price"].toDouble();
        }
        
        if (obj.contains("buy_price_display")) {
            item.buyPriceDisplay = obj["buy_price_display"].toDouble();
        } else {
            item.buyPriceDisplay = std::round(item.buyPriceInternal);
        }
        
        if (obj.contains("sell_price_internal")) {
            item.sellPriceInternal = obj["sell_price_internal"].toDouble();
        } else if (obj.contains("sellPriceInternal")) {
            item.sellPriceInternal = obj["sellPriceInternal"].toDouble();
        } else {
            // Calculate from buy price (70% sell rate)
            item.sellPriceInternal = item.buyPriceInternal * 0.70;
        }
        
        if (obj.contains("sell_price_display")) {
            item.sellPriceDisplay = obj["sell_price_display"].toDouble();
        } else {
            item.sellPriceDisplay = std::round(item.sellPriceInternal);
        }
        
        // Weight
        item.weight = obj["weight"].toDouble(0.0);
        
        // Boolean flags
        if (obj.contains("is_purchasable")) {
            item.isPurchasable = obj["is_purchasable"].toBool(true);
        } else if (obj.contains("isPurchasable")) {
            item.isPurchasable = obj["isPurchasable"].toBool(true);
        } else {
            item.isPurchasable = true;
        }
        
        if (obj.contains("is_sellable")) {
            item.isSellable = obj["is_sellable"].toBool(true);
        } else if (obj.contains("isSellable")) {
            item.isSellable = obj["isSellable"].toBool(true);
        } else {
            item.isSellable = true;
        }
        
        if (obj.contains("is_craftable")) {
            item.isCraftable = obj["is_craftable"].toBool(false);
        } else if (obj.contains("isCraftable")) {
            item.isCraftable = obj["isCraftable"].toBool(false);
        } else {
            item.isCraftable = false;
        }
        
        // Pricing group
        QString pricingGroupStr = obj["pricing_group"].toString();
        if (pricingGroupStr.isEmpty()) {
            pricingGroupStr = obj["pricingGroup"].toString("Base70");
        }
        item.pricingGroup = stringToPricingGroup(pricingGroupStr);
        
        // Notes
        item.notes = obj["notes"].toString();
        
        items.append(item);
    }
    
    return items;
}

int ItemImporter::getItemCount(Database *database)
{
    return database->getAllItems().size();
}

} // namespace Frontier
