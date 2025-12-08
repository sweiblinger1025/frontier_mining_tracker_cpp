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
    , m_loaderCycleTimeMinutes(1.5)
    , m_truckCycleTimeMinutes(6.0)
    , m_fuelPricePerLiter(0.32)
{
    // Load preferences
    QSettings settings("FrontierMining", "Tracker");

    // Unit system
    int unitVal = settings.value("Operations/unitSystem",
                                  static_cast<int>(UnitSystem::Imperial)).toInt();
    m_unitSystem = static_cast<UnitSystem>(unitVal);

    // Cycle times
    m_loaderCycleTimeMinutes = settings.value("Operations/loaderCycleTimeMinutes", 1.5).toDouble();
    m_truckCycleTimeMinutes = settings.value("Operations/truckCycleTimeMinutes", 6.0).toDouble();

    // Fuel price
    m_fuelPricePerLiter = settings.value("Operations/fuelPricePerLiter", 0.32).toDouble();
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

// === Cycle Time Settings ===

double OperationsManager::loaderCycleTimeMinutes() const
{
    return m_loaderCycleTimeMinutes;
}

void OperationsManager::setLoaderCycleTimeMinutes(double minutes)
{
    if (m_loaderCycleTimeMinutes != minutes) {
        m_loaderCycleTimeMinutes = minutes;

        QSettings settings("FrontierMining", "Tracker");
        settings.setValue("Operations/loaderCycleTimeMinutes", minutes);

        emit cycleTimesChanged();
    }
}

double OperationsManager::truckCycleTimeMinutes() const
{
    return m_truckCycleTimeMinutes;
}

void OperationsManager::setTruckCycleTimeMinutes(double minutes)
{
    if (m_truckCycleTimeMinutes != minutes) {
        m_truckCycleTimeMinutes = minutes;

        QSettings settings("FrontierMining", "Tracker");
        settings.setValue("Operations/truckCycleTimeMinutes", minutes);

        emit cycleTimesChanged();
    }
}

// === Fuel Price ===

double OperationsManager::fuelPricePerLiter() const
{
    return m_fuelPricePerLiter;
}

void OperationsManager::setFuelPricePerLiter(double price)
{
    if (m_fuelPricePerLiter != price) {
        m_fuelPricePerLiter = price;

        QSettings settings("FrontierMining", "Tracker");
        settings.setValue("Operations/fuelPricePerLiter", price);
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

QString OperationsManager::determineRoleFromCategory(const QString &category) const
{
    // Check if it's a hauler/truck
    if (category.contains("Truck", Qt::CaseInsensitive) ||
        category.contains("Hauler", Qt::CaseInsensitive)) {
        return "HaulTruck";
    }

    // Check if it's a loader/excavator
    if (category.contains("Loader", Qt::CaseInsensitive) ||
        category.contains("Excavator", Qt::CaseInsensitive) ||
        category.contains("Backhoe", Qt::CaseInsensitive) ||
        category.contains("Shovel", Qt::CaseInsensitive)) {
        return "Loader";
    }

    // Check if it's a dozer/grader (support equipment)
    if (category.contains("Dozer", Qt::CaseInsensitive) ||
        category.contains("Grader", Qt::CaseInsensitive) ||
        category.contains("Scraper", Qt::CaseInsensitive)) {
        return "Support";
    }

    // Default to loader role
    return "Loader";
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
    // First, auto-calculate hours for all equipment in the session
    autoCalculateSessionHours(sessionId);

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
    // Calculate hours from activity and estimated fuel before saving
    MovementEquipmentUsage updated = usage;

    // Auto-calculate hours from activity count
    int activityCount = 0;
    if (updated.role == "HaulTruck") {
        activityCount = updated.dumps;
    } else {
        activityCount = updated.buckets;
    }
    updated.hoursUsed = calculateHoursFromActivity(updated.role, activityCount);

    // Calculate fuel
    updated.estimatedFuelL = calculateEstimatedFuel(updated);

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
            entry.unitPrice = m_fuelPricePerLiter;
            entry.totalCost = usage.estimatedFuelL * m_fuelPricePerLiter;
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

    // For loaders/excavators: buckets × bucket capacity
    if (usage.buckets > 0 && spec->bucketCapacityM3 > 0) {
        return usage.buckets * spec->bucketCapacityM3;
    }

    // For trucks: dumps × truck capacity
    if (usage.dumps > 0 && spec->truckCapacityM3 > 0) {
        return usage.dumps * spec->truckCapacityM3;
    }

    // Legacy support for loads field
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

double OperationsManager::calculateHoursFromActivity(const QString &role, int count) const
{
    if (count <= 0) return 0;

    double cycleTimeMinutes = 0;

    if (role == "HaulTruck") {
        cycleTimeMinutes = m_truckCycleTimeMinutes;
    } else {
        // Loader, Excavator, or other
        cycleTimeMinutes = m_loaderCycleTimeMinutes;
    }

    // Convert to hours: (count × minutes) / 60
    return (count * cycleTimeMinutes) / 60.0;
}

void OperationsManager::autoCalculateSessionHours(int sessionId)
{
    auto usages = m_database->getEquipmentUsageForSession(sessionId);

    for (auto &usage : usages) {
        // Get activity count based on role
        int activityCount = 0;
        if (usage.role == "HaulTruck") {
            activityCount = usage.dumps;
        } else {
            activityCount = usage.buckets;
        }

        // Calculate hours
        usage.hoursUsed = calculateHoursFromActivity(usage.role, activityCount);

        // Recalculate fuel
        usage.estimatedFuelL = calculateEstimatedFuel(usage);

        // Update in database
        m_database->addOrUpdateEquipmentUsage(usage);
    }

    emit equipmentUsageUpdated(sessionId);
}

} // namespace Frontier
