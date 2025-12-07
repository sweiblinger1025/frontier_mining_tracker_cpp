/**
 * @file operationsmanager.cpp
 * @brief Implementation of OperationsManager
 */

#include "operationsmanager.h"

namespace Frontier {

OperationsManager::OperationsManager(Database *database, QObject *parent)
    : QObject(parent)
    , m_database(database)
    , m_activeSessionId(std::nullopt)
{
    // Load unit system preference
    QSettings settings("FrontierMining", "Tracker");
    int unitVal = settings.value("Operations/unitSystem",
                                  static_cast<int>(UnitSystem::Imperial)).toInt();
    m_unitSystem = static_cast<UnitSystem>(unitVal);
}

OperationsManager::~OperationsManager()
{
}

// === Unit System ===

UnitSystem OperationsManager::unitSystem() const
{
    return m_unitSystem;
}

void OperationsManager::setUnitSystem(UnitSystem system)
{
    if (m_unitSystem != system) {
        m_unitSystem = system;

        // Save preference
        QSettings settings("FrontierMining", "Tracker");
        settings.setValue("Operations/unitSystem", static_cast<int>(system));

        emit unitSystemChanged(system);
    }
}

// === Vehicle Specs ===

QVector<Vehicle> OperationsManager::getActiveVehicles() const
{
    return m_database->getAllVehicles(true);
}

QVector<Vehicle> OperationsManager::getAllVehicles() const
{
    return m_database->getAllVehicles(false);
}

std::optional<Vehicle> OperationsManager::getVehicle(const QString &id) const
{
    return m_database->getVehicle(id);
}

// === Movement Sessions ===

int OperationsManager::startSession(const QString &mapName, const QString &notes)
{
    MovementSession session;
    session.startTime = QDateTime::currentDateTime();
    session.mapName = mapName;
    session.notes = notes;

    int sessionId = m_database->addMovementSession(session);

    if (sessionId > 0) {
        m_activeSessionId = sessionId;
        emit movementSessionStarted(sessionId);
    }

    return sessionId;
}

void OperationsManager::endSession(int sessionId)
{
    auto session = m_database->getMovementSession(sessionId);
    if (session.has_value()) {
        MovementSession updated = session.value();
        updated.endTime = QDateTime::currentDateTime();

        if (m_database->updateMovementSession(updated)) {
            if (m_activeSessionId.has_value() && m_activeSessionId.value() == sessionId) {
                m_activeSessionId = std::nullopt;
            }
            emit movementSessionEnded(sessionId);
        }
    }
}

std::optional<MovementSession> OperationsManager::getSession(int sessionId) const
{
    return m_database->getMovementSession(sessionId);
}

std::optional<int> OperationsManager::activeSessionId() const
{
    return m_activeSessionId;
}

QVector<MovementSession> OperationsManager::getAllSessions() const
{
    return m_database->getAllMovementSessions();
}

bool OperationsManager::updateSession(const MovementSession &session)
{
    bool success = m_database->updateMovementSession(session);
    if (success && session.id.has_value()) {
        emit movementSessionUpdated(session.id.value());
    }
    return success;
}

bool OperationsManager::deleteSession(int sessionId)
{
    return m_database->deleteMovementSession(sessionId);
}

// === Equipment Usage ===

void OperationsManager::addOrUpdateEquipmentUsage(const MovementEquipmentUsage &usage)
{
    // Calculate estimated fuel before saving
    MovementEquipmentUsage updated = usage;
    updated.estimatedFuelL = calculateEstimatedFuel(usage);

    if (m_database->addOrUpdateEquipmentUsage(updated)) {
        emit equipmentUsageUpdated(usage.sessionId);
    }
}

QVector<MovementEquipmentUsage> OperationsManager::getEquipmentUsageForSession(int sessionId) const
{
    QVector<MovementEquipmentUsage> usages = m_database->getEquipmentUsageForSession(sessionId);

    // Calculate derived values
    for (auto &usage : usages) {
        usage.volumeM3 = calculateVolume(usage);
    }

    return usages;
}

bool OperationsManager::deleteEquipmentUsage(int usageId)
{
    return m_database->deleteEquipmentUsage(usageId);
}

// === Fuel Log ===

void OperationsManager::addFuelLogEntry(const FuelLogEntry &entry)
{
    FuelLogEntry updated = entry;
    updated.totalCost = entry.liters * entry.unitPrice;

    if (m_database->addFuelLogEntry(updated)) {
        emit fuelLogUpdated(updated);
    }
}

QVector<FuelLogEntry> OperationsManager::getFuelLog(const QDateTime &from,
                                                     const QDateTime &to,
                                                     const QString &equipmentId) const
{
    return m_database->getFuelLog(from, to, equipmentId);
}

double OperationsManager::getTotalFuelInRange(const QDateTime &from, const QDateTime &to) const
{
    return m_database->getTotalFuelInRange(from, to);
}

double OperationsManager::getTotalFuelCostInRange(const QDateTime &from, const QDateTime &to) const
{
    return m_database->getTotalFuelCostInRange(from, to);
}

void OperationsManager::generateFuelLogFromSession(int sessionId)
{
    auto usages = getEquipmentUsageForSession(sessionId);
    auto session = getSession(sessionId);

    if (!session.has_value()) return;

    for (const auto &usage : usages) {
        if (usage.estimatedFuelL > 0) {
            FuelLogEntry entry;
            entry.dateTime = session->endTime.isValid() ? session->endTime : QDateTime::currentDateTime();
            entry.equipmentId = usage.equipmentId;
            entry.liters = usage.estimatedFuelL;
            entry.unitPrice = 0;  // Can be set later
            entry.totalCost = 0;
            entry.meterOrHours = usage.hoursUsed;
            entry.source = QString("Session #%1").arg(sessionId);
            entry.notes = QString("Auto-generated from movement session");

            addFuelLogEntry(entry);
        }
    }
}

// === Calculations ===

double OperationsManager::calculateVolume(const MovementEquipmentUsage &usage) const
{
    auto spec = getVehicle(usage.equipmentId);
    if (!spec.has_value()) return 0;

    if (usage.buckets > 0 && spec->bucketCapacityM3 > 0) {
        return usage.buckets * spec->bucketCapacityM3;
    }

    if (usage.loads > 0 && spec->truckCapacityM3 > 0) {
        return usage.loads * spec->truckCapacityM3;
    }

    return 0;
}

double OperationsManager::calculateEstimatedFuel(const MovementEquipmentUsage &usage) const
{
    auto spec = getVehicle(usage.equipmentId);
    if (!spec.has_value()) return 0;

    return usage.hoursUsed * spec->fuelUseLPerHour;
}

} // namespace Frontier
