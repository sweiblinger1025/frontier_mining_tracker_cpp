/**
 * @file locationimporter.cpp
 * @brief Location data import utility implementation
 */

#include "locationimporter.h"
#include "database.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

namespace Frontier {

// Static member initialization
int LocationImporter::s_mapsImported = 0;
int LocationImporter::s_typesImported = 0;
int LocationImporter::s_locationsImported = 0;
QString LocationImporter::s_lastError;

bool LocationImporter::importFromJson(const QString &mapsPath,
                                      const QString &typesPath,
                                      const QString &locationsPath,
                                      Database *database,
                                      bool clearExisting)
{
    s_mapsImported = 0;
    s_typesImported = 0;
    s_locationsImported = 0;
    s_lastError.clear();

    // Load all data first (validate before modifying database)
    QVector<Map> maps = loadMapsFromJson(mapsPath);
    if (maps.isEmpty() && !s_lastError.isEmpty()) {
        return false;
    }

    QVector<LocationType> types = loadTypesFromJson(typesPath);
    if (types.isEmpty() && !s_lastError.isEmpty()) {
        return false;
    }

    QVector<Location> locations = loadLocationsFromJson(locationsPath);
    if (locations.isEmpty() && !s_lastError.isEmpty()) {
        return false;
    }

    // Clear existing data if requested (order matters due to foreign keys)
    if (clearExisting) {
        database->clearAllLocations();
        database->clearAllLocationTypes();
        database->clearAllMaps();
    }

    // Import maps first (locations depend on map_id)
    QMap<int, int> mapIdMapping;  // old_id -> new_id
    for (const auto &map : maps) {
        int oldId = map.id.value_or(0);
        int newId = database->addMap(map);
        if (newId > 0) {
            mapIdMapping[oldId] = newId;
            s_mapsImported++;
        } else {
            qWarning() << "Failed to import map:" << map.name;
        }
    }

    // Import location types (locations depend on type_id)
    QMap<int, int> typeIdMapping;  // old_id -> new_id
    for (const auto &type : types) {
        int oldId = type.id.value_or(0);
        int newId = database->addLocationType(type);
        if (newId > 0) {
            typeIdMapping[oldId] = newId;
            s_typesImported++;
        } else {
            qWarning() << "Failed to import location type:" << type.name;
        }
    }

    // Import locations with remapped IDs
    for (auto &loc : locations) {
        // Remap foreign keys to new IDs
        loc.mapId = mapIdMapping.value(loc.mapId, loc.mapId);
        loc.typeId = typeIdMapping.value(loc.typeId, loc.typeId);

        int newId = database->addLocation(loc);
        if (newId > 0) {
            s_locationsImported++;
        } else {
            qWarning() << "Failed to import location:" << loc.name;
        }
    }

    qDebug() << "Location import complete:"
             << s_mapsImported << "maps,"
             << s_typesImported << "types,"
             << s_locationsImported << "locations";

    return true;
}

bool LocationImporter::importFromDirectory(const QString &directoryPath,
                                           Database *database,
                                           bool clearExisting)
{
    QDir dir(directoryPath);

    QString mapsPath = dir.filePath("maps.json");
    QString typesPath = dir.filePath("location_types.json");
    QString locationsPath = dir.filePath("locations.json");

    // Check all files exist
    if (!QFile::exists(mapsPath)) {
        s_lastError = QString("maps.json not found in %1").arg(directoryPath);
        return false;
    }
    if (!QFile::exists(typesPath)) {
        s_lastError = QString("location_types.json not found in %1").arg(directoryPath);
        return false;
    }
    if (!QFile::exists(locationsPath)) {
        s_lastError = QString("locations.json not found in %1").arg(directoryPath);
        return false;
    }

    return importFromJson(mapsPath, typesPath, locationsPath, database, clearExisting);
}

QVector<Map> LocationImporter::loadMapsFromJson(const QString &path)
{
    QVector<Map> maps;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = QString("Cannot open maps file: %1").arg(file.errorString());
        return maps;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        s_lastError = QString("JSON parse error in maps: %1").arg(parseError.errorString());
        return maps;
    }

    if (!doc.isArray()) {
        s_lastError = "maps.json root is not an array";
        return maps;
    }

    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();

        Map map;
        map.id = obj["id"].toInt();
        map.abbrev = obj["abbrev"].toString();
        map.name = obj["name"].toString();

        if (!map.abbrev.isEmpty() && !map.name.isEmpty()) {
            maps.append(map);
        }
    }

    return maps;
}

QVector<LocationType> LocationImporter::loadTypesFromJson(const QString &path)
{
    QVector<LocationType> types;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = QString("Cannot open location_types file: %1").arg(file.errorString());
        return types;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        s_lastError = QString("JSON parse error in location_types: %1").arg(parseError.errorString());
        return types;
    }

    if (!doc.isArray()) {
        s_lastError = "location_types.json root is not an array";
        return types;
    }

    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();

        LocationType type;
        type.id = obj["id"].toInt();
        type.name = obj["name"].toString();

        if (!type.name.isEmpty()) {
            types.append(type);
        }
    }

    return types;
}

QVector<Location> LocationImporter::loadLocationsFromJson(const QString &path)
{
    QVector<Location> locations;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = QString("Cannot open locations file: %1").arg(file.errorString());
        return locations;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        s_lastError = QString("JSON parse error in locations: %1").arg(parseError.errorString());
        return locations;
    }

    if (!doc.isArray()) {
        s_lastError = "locations.json root is not an array";
        return locations;
    }

    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();

        Location loc;
        loc.id = obj["id"].toInt();
        loc.name = obj["name"].toString();
        loc.mapId = obj["map_id"].toInt();
        loc.typeId = obj["type_id"].toInt();

        if (!loc.name.isEmpty() && loc.mapId > 0 && loc.typeId > 0) {
            locations.append(loc);
        }
    }

    return locations;
}

} // namespace Frontier
