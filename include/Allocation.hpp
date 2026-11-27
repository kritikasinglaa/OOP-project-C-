// Allocation.hpp - overloaded supply allocation routines (UNIT I: function overloading)
#pragma once

#include "DistributionDepot.hpp"
#include "TransportManifest.hpp"
#include "Types.hpp"

namespace ReliefOps {
namespace Logistics {

// All three overloads move stock from `depot` into `manifest` (both taken by reference and
// updated in place) and return the number of units moved. Only stock above the depot's
// protected reserve is released.

// (1) Volume-based payload: fill the manifest up to `maxVolumeM3` in total, in stock order.
int allocateSupplies(Inventory::DistributionDepot& depot, TransportManifest& manifest,
                     double maxVolumeM3);

// (2) Priority-driven emergency load: take every line at or above `minPriority`,
//     most urgent first, limited to `maxLines` lines.
int allocateSupplies(Inventory::DistributionDepot& depot, TransportManifest& manifest,
                     Priority minPriority, int maxLines);

// (3) Targeted request: up to `quantity` units of one supply type.
int allocateSupplies(Inventory::DistributionDepot& depot, TransportManifest& manifest,
                     Inventory::SupplyType type, int quantity);

}  // namespace Logistics
}  // namespace ReliefOps
