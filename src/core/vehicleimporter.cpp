/**
 * @file vehicleimporter.cpp
 * @brief Implementation of vehicle JSON importer
 */

#include "vehicleimporter.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

namespace Frontier {

int VehicleImporter::importFromJson(const QString &jsonPath, Database *database,
                                    bool clearExisting)
{
    QVector<Vehicle> vehicles = loadFromJson(jsonPath);

    if (vehicles.isEmpty()) {
        qWarning() << "No vehicles loaded from" << jsonPath;
        return -1;
    }

    // Optionally clear existing vehicles
    if (clearExisting) {
        auto existing = database->getAllVehicles(false);
        for (const auto &v : existing) {
            database->deleteVehicle(v.id);
        }
        qInfo() << "Cleared" << existing.size() << "existing vehicles";
    }

    int imported = 0;
    int skipped = 0;

    for (const auto &vehicle : vehicles) {
        // Check if vehicle already exists
        auto existing = database->getVehicle(vehicle.id);

        if (existing.has_value()) {
            // Update existing
            if (database->updateVehicle(vehicle)) {
                imported++;
            } else {
                qWarning() << "Failed to update vehicle:" << vehicle.id;
                skipped++;
            }
        } else {
            // Add new
            if (database->addVehicle(vehicle)) {
                imported++;
            } else {
                qWarning() << "Failed to add vehicle:" << vehicle.id;
                skipped++;
            }
        }
    }

    qInfo() << "Vehicle import complete:" << imported << "imported," << skipped << "skipped";
    return imported;
}

QVector<Vehicle> VehicleImporter::loadFromJson(const QString &jsonPath)
{
    QVector<Vehicle> vehicles;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open vehicle file:" << jsonPath;
        return vehicles;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString();
        return vehicles;
    }

    if (!doc.isArray()) {
        qWarning() << "Expected JSON array in vehicle file";
        return vehicles;
    }

    QJsonArray array = doc.array();

    for (const QJsonValue &value : array) {
        if (!value.isObject()) continue;

        QJsonObject obj = value.toObject();

        Vehicle vehicle;
        vehicle.id = obj["id"].toString();
        vehicle.name = obj["name"].toString();
        vehicle.categoryMain = obj["category_main"].toString();
        vehicle.categorySub = obj["category_sub"].toString();
        vehicle.bucketCapacityM3 = obj["bucket_capacity_m3"].toDouble();
        vehicle.truckCapacityM3 = obj["truck_capacity_m3"].toDouble();
        vehicle.tankCapacityL = obj["tank_capacity_l"].toDouble();
        vehicle.fuelUseLPerHour = obj["fuel_use_l_per_hour"].toDouble();
        vehicle.purchasePrice = obj["purchase_price"].toDouble();
        vehicle.active = obj["active"].toInt() == 1;
        vehicle.notes = obj["notes"].toString();

        vehicles.append(vehicle);
    }

    qInfo() << "Loaded" << vehicles.size() << "vehicles from JSON";
    return vehicles;
}

int VehicleImporter::getVehicleCount(Database *database)
{
    return database->getAllVehicles(false).size();
}

} // namespace Frontier
