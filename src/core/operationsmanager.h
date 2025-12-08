/**
 * @file operationsmanager.h
 * @brief Central manager for Operations tab data
 */

#ifndef OPERATIONSMANAGER_H
#define OPERATIONSMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QSettings>
#include "types.h"
#include "database.h"

namespace Frontier {

class OperationsManager : public QObject
{
    Q_OBJECT

public:
    explicit OperationsManager(Database *database, QObject *parent = nullptr);
    ~OperationsManager();

    // Unit System
    UnitSystem unitSystem() const;
    void setUnitSystem(UnitSystem system);

    // Cycle Time Settings (for auto-calculating hours from activity)
    double loaderCycleTimeMinutes() const;
    void setLoaderCycleTimeMinutes(double minutes);
    double truckCycleTimeMinutes() const;
    void setTruckCycleTimeMinutes(double minutes);

    // Fuel Price Setting
    double fuelPricePerLiter() const;
    void setFuelPricePerLiter(double price);

    // Vehicle Specs
    QVector<Vehicle> getActiveVehicles() const;
    QVector<Vehicle> getAllVehicles() const;
    std::optional<Vehicle> getVehicle(const QString &id) const;

    // Determine equipment role from category
    QString determineRoleFromCategory(const QString &category) const;

    // Movement Sessions
    int startSession(const QString &mapName, const QString &notes);
    void endSession(int sessionId);
    std::optional<MovementSession> getSession(int sessionId) const;
    std::optional<int> activeSessionId() const;
    QVector<MovementSession> getAllSessions() const;
    bool updateSession(const MovementSession &session);
    bool deleteSession(int sessionId);

    // Equipment Usage
    void addOrUpdateEquipmentUsage(const MovementEquipmentUsage &usage);
    QVector<MovementEquipmentUsage> getEquipmentUsageForSession(int sessionId) const;
    bool deleteEquipmentUsage(int usageId);

    // Fuel Log
    void addFuelLogEntry(const FuelLogEntry &entry);
    QVector<FuelLogEntry> getFuelLog(const QDateTime &from, const QDateTime &to,
                                     const QString &equipmentId = QString()) const;
    double getTotalFuelInRange(const QDateTime &from, const QDateTime &to) const;
    double getTotalFuelCostInRange(const QDateTime &from, const QDateTime &to) const;

    // Generate fuel log entries from session equipment usage
    void generateFuelLogFromSession(int sessionId);

    // Calculations
    double calculateVolume(const MovementEquipmentUsage &usage) const;
    double calculateEstimatedFuel(const MovementEquipmentUsage &usage) const;
    double calculateHoursFromActivity(const QString &role, int count) const;

    // Auto-calculate hours for all equipment in a session
    void autoCalculateSessionHours(int sessionId);

signals:
    void unitSystemChanged(UnitSystem system);
    void fuelLogUpdated(const FuelLogEntry &entry);
    void movementSessionStarted(int sessionId);
    void movementSessionEnded(int sessionId);
    void movementSessionUpdated(int sessionId);
    void equipmentUsageUpdated(int sessionId);
    void cycleTimesChanged();

private:
    Database *m_database;
    UnitSystem m_unitSystem;
    std::optional<int> m_activeSessionId;

    // Cycle time settings (in minutes)
    double m_loaderCycleTimeMinutes;
    double m_truckCycleTimeMinutes;

    // Fuel price (per liter)
    double m_fuelPricePerLiter;
};

} // namespace Frontier

#endif // OPERATIONSMANAGER_H
