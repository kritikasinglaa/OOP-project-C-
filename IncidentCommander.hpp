// IncidentCommander.hpp - friend class that overrides access barriers (UNIT I)
#pragma once

#include <string>
#include <utility>

#include "DistributionDepot.hpp"
#include "ReliefCargo.hpp"
#include "Types.hpp"

namespace ReliefOps {
namespace Logistics {

// Declared `friend` by ReliefCargo and DistributionDepot, so during critical triage the
// commander can reach private state that ordinary code can only touch through the
// protected public interface: cargo priority and the depot's protected reserve.
class IncidentCommander {
public:
    explicit IncidentCommander(std::string callsign) : callsign_(std::move(callsign)) {}

    const std::string& callsign() const { return callsign_; }
    int overridesIssued() const { return overrides_; }

    // Rewrites a cargo line's (otherwise immutable) priority.
    void escalate(Inventory::ReliefCargo& cargo, Priority priority);

    // Pulls stock straight out of a depot, ignoring its protected reserve.
    Inventory::ReliefCargo commandeer(Inventory::DistributionDepot& depot, int stockId,
                                      int quantity);

    // Dissolves the depot's reserve entirely.
    void releaseReserve(Inventory::DistributionDepot& depot);

    // Raw view of depot internals (quantity / par level / reserve per line).
    std::string audit(const Inventory::DistributionDepot& depot) const;

private:
    std::string callsign_;
    int overrides_ = 0;
};

}  // namespace Logistics
}  // namespace ReliefOps
