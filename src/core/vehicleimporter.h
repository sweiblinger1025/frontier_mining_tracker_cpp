/**
 * @file vehicleimporter.h
 * @brief Utility to import vehicles from JSON file
 */

#ifndef VEHICLEIMPORTER_H
#define VEHICLEIMPORTER_H

#include <QString>
#include <QVector>
#include "types.h"
#include "database.h"

namespace Frontier {

class VehicleImporter
{
public:
    /**
     * @brief Import vehicles from JSON file into database
     * @param jsonPath Path to vehicles.json
     * @param database Database instance
     * @param clearExisting If true, delete all existing vehicles first
     * @return Number of vehicles imported, or -1 on error
     */
    static int importFromJson(const QString &jsonPath, Database *database,
                              bool clearExisting = true);

    /**
     * @brief Load vehicles from JSON file (without importing to DB)
     * @param jsonPath Path to vehicles.json
     * @return Vector of Vehicle objects
     */
    static QVector<Vehicle> loadFromJson(const QString &jsonPath);

    /**
     * @brief Get count of vehicles in database
     * @param database Database instance
     * @return Number of vehicles
     */
    static int getVehicleCount(Database *database);
};

} // namespace Frontier

#endif // VEHICLEIMPORTER_H
