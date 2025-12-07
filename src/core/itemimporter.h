/**
 * @file itemimporter.h
 * @brief Item import utility for JSON files
 */

#ifndef ITEMIMPORTER_H
#define ITEMIMPORTER_H

#include <QString>
#include <QVector>
#include "types.h"

namespace Frontier {

class Database;

class ItemImporter
{
public:
    /**
     * @brief Import items from JSON file into database
     * @param jsonPath Path to JSON file
     * @param database Database instance
     * @param clearExisting If true, deletes all existing items first
     * @return Number of items imported, or -1 on error
     */
    static int importFromJson(const QString &jsonPath, Database *database, bool clearExisting = false);

    /**
     * @brief Load items from JSON without database import
     * @param jsonPath Path to JSON file
     * @return Vector of Item structs
     */
    static QVector<Item> loadFromJson(const QString &jsonPath);

    /**
     * @brief Get count of items in database
     * @param database Database instance
     * @return Number of items
     */
    static int getItemCount(Database *database);
};

} // namespace Frontier

#endif // ITEMIMPORTER_H
