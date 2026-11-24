#include "Vehicles.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "Exceptions.hpp"

namespace ReliefOps {
namespace Logistics {

namespace {

bool g_traceDestructors = false;

void traceDtor(const char* className, const std::string& vehicleName) {
    if (g_traceDestructors) {
        std::cout << "    [dtor] ~" << className << "(" << vehicleName << ")\n";
    }
}

}  // namespace

// ============================== GPSBeacon ====================================

GPSBeacon::GPSBeacon(std::string unitId)
    : unitId_(std::move(unitId)), lastWaypoint_("(depot)"), odometerKm_(0.0) {}

void GPSBeacon::advance(const std::string& waypoint, double km) {
    lastWaypoint_ = waypoint;
    odometerKm_ += km;
}

std::string GPSBeacon::describe() const {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << unitId_ << " @ " << lastWaypoint_
      << ", odometer " << odometerKm_ << " km";
    return o.str();
}

// ======================== EmergencyLifeSupportUnit ===========================

EmergencyLifeSupportUnit::EmergencyLifeSupportUnit(double oxygenHours, bool defibrillator)
    : oxygenHours_(oxygenHours), defibrillator_(defibrillator) {}

void EmergencyLifeSupportUnit::consume(double hours) {
    oxygenHours_ = std::max(0.0, oxygenHours_ - hours);
}

std::string EmergencyLifeSupportUnit::describe() const {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << "O2 " << oxygenHours_ << " h"
      << (defibrillator_ ? ", defibrillator on board" : ", no defibrillator")
      << (lowOxygen() ? " [LOW OXYGEN]" : "");
    return o.str();
}

// ============================== ReliefVehicle ================================

ReliefVehicle::ReliefVehicle(const VehicleSpec& spec)
    : TransportVehicle(spec.id, spec.name, spec.capacityKg, spec.capacityM3, spec.fuelLitres),
      gps_("GPS-" + std::to_string(spec.id)),
      life_(),
      loaded_() {}

ReliefVehicle::~ReliefVehicle() { traceDtor("ReliefVehicle", name()); }

void ReliefVehicle::setDestructorTrace(bool enabled) { g_traceDestructors = enabled; }

void ReliefVehicle::load(const TransportManifest& manifest) {
    const double weight = manifest.totalWeightKg();
    const double volume = manifest.totalVolumeM3();
    if (weight > capacityKg() + 1e-9) {
        throw CargoOverweightException(name(), "weight", weight, capacityKg(), "kg");
    }
    if (volume > capacityM3() + 1e-9) {
        throw CargoOverweightException(name(), "volume", volume, capacityM3(), "m3");
    }
    loaded_ = manifest;  // deep copy via copy-and-swap
}

TransportManifest ReliefVehicle::unload() {
    TransportManifest delivered(std::move(loaded_));
    loaded_ = TransportManifest();
    return delivered;
}

double ReliefVehicle::payloadFuelFactor() const {
    return 1.0 + 0.15 * payloadUtilisation(loadedKg(), capacityKg());
}

RouteEvaluation ReliefVehicle::dispatch(const Route& route) {
    const RouteEvaluation evaluation = evaluateRoute(route);  // virtual call; may throw
    consumeFuel(evaluation.fuelLitres);                       // may throw; nothing changed yet
    life_.consume(evaluation.etaHours);
    gps_.advance(route.name, route.distanceKm);
    recordDispatch(loadedKg());
    return evaluation;
}

std::string ReliefVehicle::summary() const {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << kind() << " '" << name() << "' (#" << id()
      << "): capacity " << capacityKg() << " kg / " << capacityM3() << " m3, fuel "
      << fuelLitres() << " L";
    return o.str();
}

// ============================== AerialTransport ==============================

AerialTransport::AerialTransport(const VehicleSpec& spec, double cruiseKmh, double litresPerKm)
    : ReliefVehicle(spec), cruiseKmh_(cruiseKmh), litresPerKm_(litresPerKm) {}

AerialTransport::~AerialTransport() { traceDtor("AerialTransport", name()); }

std::string AerialTransport::kind() const { return "Aerial transport"; }

RouteEvaluation AerialTransport::evaluateRoute(const Route& route) const {
    if (!route.airspaceClear) {
        throw DamagedCorridorException(route.name, "airspace closed (storm / no-fly)");
    }
    // Flies over damage and floods; only airspace matters.
    const double fuel = fuelRequiredLitres(route.distanceKm, litresPerKm_, payloadFuelFactor());
    return {fuel <= fuelLitres() + 1e-9, travelTimeHours(route.distanceKm, cruiseKmh_), fuel,
            "air"};
}

// ============================== GroundConvoy =================================

GroundConvoy::GroundConvoy(const VehicleSpec& spec, double roadSpeedKmh, double litresPerKm,
                           double maxPassableDamage)
    : ReliefVehicle(spec),
      roadSpeedKmh_(roadSpeedKmh),
      litresPerKm_(litresPerKm),
      maxPassableDamage_(maxPassableDamage) {}

GroundConvoy::~GroundConvoy() { traceDtor("GroundConvoy", name()); }

std::string GroundConvoy::kind() const { return "Ground convoy"; }

RouteEvaluation GroundConvoy::evaluateRoute(const Route& route) const {
    if (route.flooded) {
        throw DamagedCorridorException(route.name, "flooded - wheeled vehicles cannot cross");
    }
    if (route.corridorDamage > maxPassableDamage_) {
        std::ostringstream why;
        why << std::fixed << std::setprecision(2) << "damage index " << route.corridorDamage
            << " exceeds passable limit " << maxPassableDamage_;
        throw DamagedCorridorException(route.name, why.str());
    }
    const double speed = roadSpeedKmh_ * (1.0 - 0.6 * route.corridorDamage);
    const double terrain = (1.0 + 2.0 * route.corridorDamage) * payloadFuelFactor();
    const double fuel = fuelRequiredLitres(route.distanceKm, litresPerKm_, terrain);
    return {fuel <= fuelLitres() + 1e-9, travelTimeHours(route.distanceKm, speed), fuel,
            "ground"};
}

// ============================== MarineVessel =================================

MarineVessel::MarineVessel(const VehicleSpec& spec, double waterSpeedKmh, double litresPerKm)
    : ReliefVehicle(spec), waterSpeedKmh_(waterSpeedKmh), litresPerKm_(litresPerKm) {}

MarineVessel::~MarineVessel() { traceDtor("MarineVessel", name()); }

std::string MarineVessel::kind() const { return "Marine vessel"; }

RouteEvaluation MarineVessel::evaluateRoute(const Route& route) const {
    if (!route.flooded) {
        throw DamagedCorridorException(route.name, "no navigable water on this corridor");
    }
    const double speed = waterSpeedKmh_ * (1.0 - 0.3 * route.corridorDamage);  // debris
    const double terrain = (1.0 + 0.5 * route.corridorDamage) * payloadFuelFactor();
    const double fuel = fuelRequiredLitres(route.distanceKm, litresPerKm_, terrain);
    return {fuel <= fuelLitres() + 1e-9, travelTimeHours(route.distanceKm, speed), fuel,
            "water"};
}

// ============================== AmphibiousCraft ==============================

AmphibiousCraft::AmphibiousCraft(const VehicleSpec& spec, int triageBeds)
    : ReliefVehicle(spec),                  // virtual base: initialised ONCE, by the most derived class
      GroundConvoy(spec, 30.0, 1.0, 0.95),  // slower than a truck, but tolerates worse roads
      MarineVessel(spec, 22.0, 1.4),
      triageBeds_(triageBeds) {}

AmphibiousCraft::~AmphibiousCraft() { traceDtor("AmphibiousCraft", name()); }

std::string AmphibiousCraft::kind() const { return "Amphibious medical craft"; }

// GroundConvoy and MarineVessel both override evaluateRoute, so AmphibiousCraft must supply
// the final overrider; it picks the parent that matches the terrain.
RouteEvaluation AmphibiousCraft::evaluateRoute(const Route& route) const {
    RouteEvaluation evaluation = route.flooded ? MarineVessel::evaluateRoute(route)
                                               : GroundConvoy::evaluateRoute(route);
    evaluation.mode = route.flooded ? "amphibious (water mode)" : "amphibious (land mode)";
    return evaluation;
}

}  // namespace Logistics
}  // namespace ReliefOps
