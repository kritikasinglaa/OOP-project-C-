#include "Allocation.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "Dispatch.hpp"

namespace ReliefOps {
namespace Logistics {

using Inventory::DistributionDepot;
using Inventory::ReliefCargo;
using Inventory::SupplyType;

namespace {

// Moves `quantity` units of `line` from the depot to the manifest.
void transfer(DistributionDepot& depot, TransportManifest& manifest, const ReliefCargo& line,
              int quantity) {
    const ReliefCargo part = line.slice(quantity);
    depot -= part;       // may throw; manifest is only touched afterwards
    manifest.add(part);
}

}  // namespace

int allocateSupplies(DistributionDepot& depot, TransportManifest& manifest, double maxVolumeM3) {
    double remaining = maxVolumeM3 - manifest.totalVolumeM3();
    int moved = 0;
    const std::vector<ReliefCargo> snapshot = depot.stock();  // depot mutates as we go

    for (const ReliefCargo& line : snapshot) {
        if (remaining <= 1e-9) break;
        const int releasable = depot.availableQuantity(line.id());
        if (releasable <= 0) continue;

        const int fits = line.unitVolumeM3() > 0.0
                             ? static_cast<int>(std::floor(remaining / line.unitVolumeM3() + 1e-9))
                             : releasable;
        const int take = std::min(releasable, fits);
        if (take <= 0) continue;

        transfer(depot, manifest, line, take);
        remaining -= take * line.unitVolumeM3();
        moved += take;
    }
    return moved;
}

int allocateSupplies(DistributionDepot& depot, TransportManifest& manifest, Priority minPriority,
                     int maxLines) {
    std::vector<ReliefCargo> candidates;
    for (const ReliefCargo& line : depot.stock()) {
        if (line.priority() >= minPriority && depot.availableQuantity(line.id()) > 0) {
            candidates.push_back(line);
        }
    }
    std::sort(candidates.begin(), candidates.end(), CargoPriorityDescending());

    int moved = 0;
    int lines = 0;
    for (const ReliefCargo& line : candidates) {
        if (lines >= maxLines) break;
        const int take = depot.availableQuantity(line.id());
        transfer(depot, manifest, line, take);
        moved += take;
        ++lines;
    }
    return moved;
}

int allocateSupplies(DistributionDepot& depot, TransportManifest& manifest, SupplyType type,
                     int quantity) {
    int wanted = quantity;
    int moved = 0;
    const std::vector<ReliefCargo> snapshot = depot.stock();

    for (const ReliefCargo& line : snapshot) {
        if (wanted <= 0) break;
        if (line.type() != type) continue;
        const int take = std::min(wanted, depot.availableQuantity(line.id()));
        if (take <= 0) continue;
        transfer(depot, manifest, line, take);
        wanted -= take;
        moved += take;
    }
    return moved;
}

}  // namespace Logistics
}  // namespace ReliefOps
