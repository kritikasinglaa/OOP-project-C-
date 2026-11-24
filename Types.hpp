// Types.hpp - shared enums, forward declarations and inline utilities (UNIT I)
#pragma once

#include <cmath>
#include <cstdint>

namespace ReliefOps {

// ---------------------------------------------------------------------------
// ReliefOps::Logistics  - movement of relief: manifests, vehicles, dispatch
// ---------------------------------------------------------------------------
namespace Logistics {

class IncidentCommander;  // forward declaration: friend of cargo + depot (UNIT I)

enum class Priority : std::uint8_t { Routine = 0, Urgent = 1, Critical = 2 };

inline const char* toString(Priority p) {
    switch (p) {
        case Priority::Routine:  return "Routine";
        case Priority::Urgent:   return "Urgent";
        case Priority::Critical: return "Critical";
    }
    return "Unknown";
}

// --- Inline utilities for rapid distance / payload calculations (UNIT I) ---
inline double kgToTonnes(double kg) { return kg / 1000.0; }

inline double travelTimeHours(double distanceKm, double speedKmh) {
    return speedKmh > 0.0 ? distanceKm / speedKmh : 0.0;
}

inline double payloadUtilisation(double loadKg, double capacityKg) {
    return capacityKg > 0.0 ? loadKg / capacityKg : 0.0;
}

inline double fuelRequiredLitres(double distanceKm, double litresPerKm,
                                 double terrainFactor = 1.0) {
    return distanceKm * litresPerKm * terrainFactor;
}

// Equirectangular approximation: plenty accurate for regional relief routing.
inline double approxDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    constexpr double kKmPerDegree = 111.195;
    constexpr double kPi = 3.14159265358979323846;
    const double meanLat = (lat1 + lat2) * 0.5 * kPi / 180.0;
    const double dx = (lon2 - lon1) * std::cos(meanLat) * kKmPerDegree;
    const double dy = (lat2 - lat1) * kKmPerDegree;
    return std::sqrt(dx * dx + dy * dy);
}

}  // namespace Logistics

// ---------------------------------------------------------------------------
// ReliefOps::Inventory  - what we hold: cargo lines and depots
// ---------------------------------------------------------------------------
namespace Inventory {

enum class SupplyType : std::uint8_t {
    RationKit = 0,
    TraumaMedicine = 1,
    CleanWater = 2,
    ThermalBlanket = 3
};

constexpr int kSupplyTypeCount = 4;

inline const char* toString(SupplyType t) {
    switch (t) {
        case SupplyType::RationKit:      return "RationKit";
        case SupplyType::TraumaMedicine: return "TraumaMedicine";
        case SupplyType::CleanWater:     return "CleanWater";
        case SupplyType::ThermalBlanket: return "ThermalBlanket";
    }
    return "Unknown";
}

}  // namespace Inventory

}  // namespace ReliefOps
