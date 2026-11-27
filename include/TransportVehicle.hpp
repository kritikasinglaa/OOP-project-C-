// TransportVehicle.hpp - vehicle abstract data type + static fleet analytics (UNIT II)
#pragma once

#include <string>

namespace ReliefOps {
namespace Logistics {

class TransportVehicle {
public:
    TransportVehicle(int id, std::string name, double capacityKg, double capacityM3,
                     double fuelLitres);
    virtual ~TransportVehicle();  // virtual: fleets are deleted through base pointers

    TransportVehicle(const TransportVehicle&) = delete;
    TransportVehicle& operator=(const TransportVehicle&) = delete;

    // Public interface only: all state is private (information hiding).
    int id() const { return id_; }
    const std::string& name() const { return name_; }
    double capacityKg() const { return capacityKg_; }
    double capacityM3() const { return capacityM3_; }
    double fuelLitres() const { return fuelLitres_; }
    bool operational() const { return operational_; }

    void refuel(double litres);
    void consumeFuel(double litres);  // throws FuelDepletionException, state unchanged on throw
    void setOperational(bool operational);

    // --- static members: real-time logistics analytics ---------------------------
    static int operationalVehicles() { return operationalCount_; }
    static double totalDispatchedTonnes() { return dispatchedKg_ / 1000.0; }
    static void recordDispatch(double kg) { dispatchedKg_ += kg; }
    static void resetAnalytics() { dispatchedKg_ = 0.0; }

private:
    int id_;
    std::string name_;
    double capacityKg_;
    double capacityM3_;
    double fuelLitres_;
    bool operational_;

    static int operationalCount_;
    static double dispatchedKg_;
};

}  // namespace Logistics
}  // namespace ReliefOps
