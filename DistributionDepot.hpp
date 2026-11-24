// DistributionDepot.hpp - regional field depot holding stock (UNIT II)
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ReliefCargo.hpp"

namespace ReliefOps {
namespace Inventory {

class DistributionDepot {
public:
    // reserveRatio (0..1): share of each line's par level that normal allocation may not touch.
    explicit DistributionDepot(std::string name, double reserveRatio = 0.0);

    const std::string& name() const { return name_; }
    double reserveRatio() const { return reserveRatio_; }

    // += restocks supplies at the depot (merges into an existing line of the same id).
    DistributionDepot& operator+=(const ReliefCargo& cargo);
    // -= deducts cargo when a vehicle departs; respects the protected reserve.
    DistributionDepot& operator-=(const ReliefCargo& cargo);

    // Return by reference so callers can update a stock line in place.
    ReliefCargo& findStock(int id);
    const ReliefCargo& findStock(int id) const;

    bool has(int id) const;
    int availableQuantity(int id) const;  // quantity above the protected reserve
    double totalWeightKg() const;

    const std::vector<ReliefCargo>& stock() const { return stock_; }

    friend class Logistics::IncidentCommander;  // may bypass the reserve

private:
    int reserveUnits(int id) const;

    std::string name_;
    std::vector<ReliefCargo> stock_;
    double reserveRatio_;
    std::unordered_map<int, int> parLevel_;  // highest level seen per stock line
};

}  // namespace Inventory
}  // namespace ReliefOps
