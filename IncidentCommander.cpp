#include "IncidentCommander.hpp"

#include <algorithm>
#include <sstream>

#include "Exceptions.hpp"

namespace ReliefOps {
namespace Logistics {

using Inventory::DistributionDepot;
using Inventory::ReliefCargo;

void IncidentCommander::escalate(ReliefCargo& cargo, Priority priority) {
    cargo.priority_ = priority;  // private member, reachable because we are a friend
    ++overrides_;
}

ReliefCargo IncidentCommander::commandeer(DistributionDepot& depot, int stockId, int quantity) {
    if (quantity <= 0) throw ERLException("commandeer: quantity must be positive");

    auto it = std::find_if(depot.stock_.begin(), depot.stock_.end(),
                           [stockId](const ReliefCargo& line) { return line.id() == stockId; });
    if (it == depot.stock_.end()) {
        throw ERLException("commandeer: depot '" + depot.name_ + "' has no stock line #" +
                           std::to_string(stockId));
    }
    if (quantity > it->quantity_) {  // physical stock is the only remaining limit
        throw InsufficientStockException(depot.name_, it->label(), quantity, it->quantity_);
    }

    ReliefCargo taken = it->slice(quantity);
    it->quantity_ -= quantity;               // bypasses reserve checks in operator-=
    taken.priority_ = Priority::Critical;    // commandeered cargo travels as Critical
    ++overrides_;
    return taken;
}

void IncidentCommander::releaseReserve(DistributionDepot& depot) {
    depot.reserveRatio_ = 0.0;
    ++overrides_;
}

std::string IncidentCommander::audit(const DistributionDepot& depot) const {
    std::ostringstream out;
    out << "Audit by " << callsign_ << " of '" << depot.name_ << "' (reserve ratio "
        << depot.reserveRatio_ << ")\n";
    for (const ReliefCargo& line : depot.stock_) {
        auto par = depot.parLevel_.find(line.id());
        out << "  #" << line.id() << ' ' << line.label() << ": qty " << line.quantity_
            << ", par " << (par == depot.parLevel_.end() ? 0 : par->second)
            << ", protected " << depot.reserveUnits(line.id()) << '\n';
    }
    return out.str();
}

}  // namespace Logistics
}  // namespace ReliefOps
