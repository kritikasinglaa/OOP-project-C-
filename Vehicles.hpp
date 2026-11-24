// Vehicles.hpp - relief vehicle hierarchy (UNIT III)
//
//                 TransportVehicle            (Unit II ADT)
//                        |
//                  ReliefVehicle              (abstract; owns GPSBeacon + life-support)
//          /-------------+--------------\        all three inherit VIRTUALLY
//   AerialTransport  GroundConvoy    MarineVessel
//                         \              /
//                         AmphibiousCraft      (diamond: ONE shared ReliefVehicle)
#pragma once

#include <iosfwd>
#include <string>

#include "TransportManifest.hpp"
#include "TransportVehicle.hpp"
#include "Types.hpp"

namespace ReliefOps {
namespace Logistics {

struct Route {
    std::string name;
    double distanceKm;
    double corridorDamage;  // 0 (intact) .. 1 (destroyed)
    bool flooded;
    bool airspaceClear;
};

struct RouteEvaluation {
    bool feasible;  // can be completed with the fuel currently on board
    double etaHours;
    double fuelLitres;
    std::string mode;
};

struct VehicleSpec {
    int id;
    std::string name;
    double capacityKg;
    double capacityM3;
    double fuelLitres;
};

// --- Containership: independent components COMPOSED inside vehicles (has-a) ---------
class GPSBeacon {
public:
    explicit GPSBeacon(std::string unitId = "GPS-000");
    void advance(const std::string& waypoint, double km);
    const std::string& unitId() const { return unitId_; }
    const std::string& lastWaypoint() const { return lastWaypoint_; }
    double odometerKm() const { return odometerKm_; }
    std::string describe() const;

private:
    std::string unitId_;
    std::string lastWaypoint_;
    double odometerKm_;
};

class EmergencyLifeSupportUnit {
public:
    explicit EmergencyLifeSupportUnit(double oxygenHours = 12.0, bool defibrillator = true);
    void consume(double hours);
    double oxygenHours() const { return oxygenHours_; }
    bool hasDefibrillator() const { return defibrillator_; }
    bool lowOxygen() const { return oxygenHours_ < 2.0; }
    std::string describe() const;

private:
    double oxygenHours_;
    bool defibrillator_;
};

// --- Abstract base ---------------------------------------------------------------------
class ReliefVehicle : public TransportVehicle {
public:
    explicit ReliefVehicle(const VehicleSpec& spec);
    ~ReliefVehicle() override;

    virtual std::string kind() const = 0;
    // Runtime polymorphism: each vehicle class evaluates a route its own way.
    virtual RouteEvaluation evaluateRoute(const Route& route) const = 0;

    // Loads a manifest (deep-copied). Strong guarantee: throws CargoOverweightException
    // and leaves the vehicle untouched if weight or volume limits would be exceeded.
    void load(const TransportManifest& manifest);
    TransportManifest unload();
    const TransportManifest& manifest() const { return loaded_; }
    double loadedKg() const { return loaded_.totalWeightKg(); }

    // Evaluates (virtually), then burns fuel, drains life support, advances the GPS and
    // records dispatched tonnage. Throws before changing any state if the leg fails.
    RouteEvaluation dispatch(const Route& route);

    GPSBeacon& gps() { return gps_; }
    const GPSBeacon& gps() const { return gps_; }
    EmergencyLifeSupportUnit& lifeSupport() { return life_; }
    const EmergencyLifeSupportUnit& lifeSupport() const { return life_; }

    std::string summary() const;

    // Prints a line from each destructor so deallocation order is visible in the demo.
    static void setDestructorTrace(bool enabled);

protected:
    // Heavier loads burn more fuel (up to +15% at full payload).
    double payloadFuelFactor() const;

private:
    GPSBeacon gps_;
    EmergencyLifeSupportUnit life_;
    TransportManifest loaded_;
};

// --- Concrete vehicles --------------------------------------------------------------------
class AerialTransport : public virtual ReliefVehicle {
public:
    explicit AerialTransport(const VehicleSpec& spec, double cruiseKmh = 220.0,
                             double litresPerKm = 2.4);
    ~AerialTransport() override;
    std::string kind() const override;
    RouteEvaluation evaluateRoute(const Route& route) const override;

private:
    double cruiseKmh_;
    double litresPerKm_;
};

class GroundConvoy : public virtual ReliefVehicle {
public:
    explicit GroundConvoy(const VehicleSpec& spec, double roadSpeedKmh = 45.0,
                          double litresPerKm = 0.6, double maxPassableDamage = 0.8);
    ~GroundConvoy() override;
    std::string kind() const override;
    RouteEvaluation evaluateRoute(const Route& route) const override;

private:
    double roadSpeedKmh_;
    double litresPerKm_;
    double maxPassableDamage_;
};

class MarineVessel : public virtual ReliefVehicle {
public:
    explicit MarineVessel(const VehicleSpec& spec, double waterSpeedKmh = 28.0,
                          double litresPerKm = 1.1);
    ~MarineVessel() override;
    std::string kind() const override;
    RouteEvaluation evaluateRoute(const Route& route) const override;

private:
    double waterSpeedKmh_;
    double litresPerKm_;
};

// Hybrid mobile medical unit. Both parents share ONE ReliefVehicle thanks to virtual bases.
class AmphibiousCraft : public GroundConvoy, public MarineVessel {
public:
    AmphibiousCraft(const VehicleSpec& spec, int triageBeds);
    ~AmphibiousCraft() override;
    std::string kind() const override;
    RouteEvaluation evaluateRoute(const Route& route) const override;  // resolves the diamond
    int triageBeds() const { return triageBeds_; }

private:
    int triageBeds_;
};

}  // namespace Logistics
}  // namespace ReliefOps
