// Persistence.hpp - binary transit logs and manifest audit records (UNIT IV)
//
// Files are written with std::ofstream in binary mode to "<path>.tmp" and then renamed over
// the target, so a crash or write failure can never leave a half-written log behind.
// Integers/doubles are stored in the host's native byte order (same-machine audit files).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "TransportManifest.hpp"

namespace ReliefOps {
namespace Logistics {

struct TransitLogEntry {
    std::int32_t manifestId;
    std::int32_t vehicleId;
    double weightKg;
    double distanceKm;
    double etaHours;
    char status[16];  // fixed width, NUL padded
};

class TransitLog {
public:
    void record(int manifestId, int vehicleId, double weightKg, double distanceKm,
                double etaHours, const std::string& status);

    const std::vector<TransitLogEntry>& entries() const { return entries_; }
    std::size_t size() const { return entries_.size(); }

    // Both throw PersistenceException on any I/O or format problem.
    void save(const std::string& path) const;
    static TransitLog load(const std::string& path);

private:
    std::vector<TransitLogEntry> entries_;
};

void saveManifestAudit(const std::string& path, const std::vector<TransportManifest>& manifests);
std::vector<TransportManifest> loadManifestAudit(const std::string& path);

}  // namespace Logistics
}  // namespace ReliefOps
