/**
 * @file locationimporter.h
 * @brief Location data import utility for JSON files
 */

#ifndef LOCATIONIMPORTER_H
#define LOCATIONIMPORTER_H

#include <QString>
#include <QVector>
#include "types.h"

namespace Frontier {

class Database;

class LocationImporter
{
public:
    /**
     * @brief Import locations from JSON files into database
     * @param mapsPath Path to maps.json
     * @param typesPath Path to location_types.json
     * @param locationsPath Path to locations.json
     * @param database Database instance
     * @param clearExisting If true, clears all existing location data first
     * @return true on success
     */
    static bool importFromJson(const QString &mapsPath,
                               const QString &typesPath,
                               const QString &locationsPath,
                               Database *database,
                               bool clearExisting = true);

    /**
     * @brief Import all location files from a directory
     * @param directoryPath Path to directory containing maps.json, location_types.json, locations.json
     * @param database Database instance
     * @param clearExisting If true, clears all existing location data first
     * @return true on success
     */
    static bool importFromDirectory(const QString &directoryPath,
                                    Database *database,
                                    bool clearExisting = true);

    // Accessors for import results
    static int mapsImported() { return s_mapsImported; }
    static int typesImported() { return s_typesImported; }
    static int locationsImported() { return s_locationsImported; }
    static QString lastError() { return s_lastError; }

private:
    static QVector<Map> loadMapsFromJson(const QString &path);
    static QVector<LocationType> loadTypesFromJson(const QString &path);
    static QVector<Location> loadLocationsFromJson(const QString &path);

    static int s_mapsImported;
    static int s_typesImported;
    static int s_locationsImported;
    static QString s_lastError;
};

} // namespace Frontier

#endif // LOCATIONIMPORTER_H
