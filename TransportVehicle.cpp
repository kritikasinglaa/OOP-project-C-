#include "TransportVehicle.hpp"

#include <utility>

#include "Exceptions.hpp"

namespace ReliefOps {
namespace Logistics {

int TransportVehicle::operationalCount_ = 0;
double TransportVehicle::dispatchedKg_ = 0.0;

TransportVehicle::TransportVehicle(int id, std::string name, double capacityKg,
                                   double capacityM3, double fuelLitres)
    : id_(id),
      name_(std::move(name)),
      capacityKg_(capacityKg),
      capacityM3_(capacityM3),
      fuelLitres_(fuelLitres),
      operational_(true) {
    if (capacityKg < 0.0 || capacityM3 < 0.0 || fuelLitres < 0.0) {
        throw ERLException("TransportVehicle: capacities and fuel must be non-negative");
    }
    ++operationalCount_;  // last statement: a throwing constructor never inflates the count
}

TransportVehicle::~TransportVehicle() {
    if (operational_) --operationalCount_;
}

void TransportVehicle::refuel(double litres) {
    if (litres < 0.0) throw ERLException("refuel: litres must be non-negative");
    fuelLitres_ += litres;
}

void TransportVehicle::consumeFuel(double litres) {
    if (litres < 0.0) throw ERLException("consumeFuel: litres must be non-negative");
    if (litres > fuelLitres_ + 1e-9) throw FuelDepletionException(name_, litres, fuelLitres_);
    fuelLitres_ -= litres;
    if (fuelLitres_ < 0.0) fuelLitres_ = 0.0;
}

void TransportVehicle::setOperational(bool operational) {
    if (operational == operational_) return;
    operational_ = operational;
    operationalCount_ += operational ? 1 : -1;
}

}  // namespace Logistics
}  // namespace ReliefOps
