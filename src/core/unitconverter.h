/**
 * @file unitconverter.h
 * @brief Utility class for unit conversions between metric and imperial
 */

#ifndef UNITCONVERTER_H
#define UNITCONVERTER_H

#include <QString>
#include "types.h"

namespace Frontier {

class UnitConverter
{
public:
    // === Volume Conversions ===
    // Base unit: cubic meters (m³)

    static constexpr double CUBIC_YARDS_PER_CUBIC_METER = 1.30795;
    static constexpr double CUBIC_METERS_PER_CUBIC_YARD = 0.764555;

    static double cubicMetersToYards(double m3) {
        return m3 * CUBIC_YARDS_PER_CUBIC_METER;
    }

    static double cubicYardsToMeters(double yd3) {
        return yd3 * CUBIC_METERS_PER_CUBIC_YARD;
    }

    static double volumeToDisplay(double m3, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? cubicMetersToYards(m3) : m3;
    }

    static double volumeToStorage(double value, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? cubicYardsToMeters(value) : value;
    }

    static QString volumeUnitLabel(UnitSystem system) {
        return (system == UnitSystem::Imperial) ? "yd³" : "m³";
    }

    // === Liquid Volume Conversions ===
    // Base unit: liters (L)

    static constexpr double GALLONS_PER_LITER = 0.264172;
    static constexpr double LITERS_PER_GALLON = 3.78541;

    static double litersToGallons(double liters) {
        return liters * GALLONS_PER_LITER;
    }

    static double gallonsToLiters(double gallons) {
        return gallons * LITERS_PER_GALLON;
    }

    static double fuelToDisplay(double liters, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? litersToGallons(liters) : liters;
    }

    static double fuelToStorage(double value, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? gallonsToLiters(value) : value;
    }

    static QString fuelUnitLabel(UnitSystem system) {
        return (system == UnitSystem::Imperial) ? "gal" : "L";
    }

    // === Fuel Consumption ===
    // Base unit: L/hr

    static double fuelRateToDisplay(double lPerHour, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? litersToGallons(lPerHour) : lPerHour;
    }

    static double fuelRateToStorage(double value, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? gallonsToLiters(value) : value;
    }

    static QString fuelRateUnitLabel(UnitSystem system) {
        return (system == UnitSystem::Imperial) ? "gal/hr" : "L/hr";
    }

    // === Distance Conversions (for future use) ===
    // Base unit: kilometers (km)

    static constexpr double MILES_PER_KM = 0.621371;
    static constexpr double KM_PER_MILE = 1.60934;

    static double kmToMiles(double km) {
        return km * MILES_PER_KM;
    }

    static double milesToKm(double miles) {
        return miles * KM_PER_MILE;
    }

    static double distanceToDisplay(double km, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? kmToMiles(km) : km;
    }

    static double distanceToStorage(double value, UnitSystem system) {
        return (system == UnitSystem::Imperial) ? milesToKm(value) : value;
    }

    static QString distanceUnitLabel(UnitSystem system) {
        return (system == UnitSystem::Imperial) ? "mi" : "km";
    }

    // === Formatting Helpers ===

    static QString formatVolume(double m3, UnitSystem system, int decimals = 2) {
        double value = volumeToDisplay(m3, system);
        return QString("%1 %2").arg(value, 0, 'f', decimals).arg(volumeUnitLabel(system));
    }

    static QString formatFuel(double liters, UnitSystem system, int decimals = 1) {
        double value = fuelToDisplay(liters, system);
        return QString("%1 %2").arg(value, 0, 'f', decimals).arg(fuelUnitLabel(system));
    }

    static QString formatFuelRate(double lPerHour, UnitSystem system, int decimals = 1) {
        double value = fuelRateToDisplay(lPerHour, system);
        return QString("%1 %2").arg(value, 0, 'f', decimals).arg(fuelRateUnitLabel(system));
    }
};

} // namespace Frontier

#endif // UNITCONVERTER_H
