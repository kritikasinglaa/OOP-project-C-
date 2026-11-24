// Dispatch.hpp - dispatch requests, comparator functors, regional board (UNIT IV)
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "PriorityDispatchQueue.hpp"
#include "ReliefCargo.hpp"
#include "Types.hpp"

namespace ReliefOps {
namespace Logistics {

struct DispatchRequest {
    int id;
    std::string region;
    std::string description;
    Inventory::SupplyType need;
    Priority priority;
    int severity;         // 0..100 medical severity score
    int minutesWaiting;   // how long the request has been open
    int quantity;
};

// Functor: orders requests so urgent MEDICAL needs surface first.
//   1. higher Priority
//   2. medical (trauma) supplies ahead of other supplies
//   3. higher severity
//   4. longer waiting time
//   5. lower request id (deterministic tie-break)
// Returns true when `a` is LESS urgent than `b`.
struct MedicalUrgencyComparator {
    bool operator()(const DispatchRequest& a, const DispatchRequest& b) const {
        if (a.priority != b.priority) return a.priority < b.priority;
        const bool aMedical = a.need == Inventory::SupplyType::TraumaMedicine;
        const bool bMedical = b.need == Inventory::SupplyType::TraumaMedicine;
        if (aMedical != bMedical) return bMedical;
        if (a.severity != b.severity) return a.severity < b.severity;
        if (a.minutesWaiting != b.minutesWaiting) return a.minutesWaiting < b.minutesWaiting;
        return a.id > b.id;
    }
};

// Functor: sorts cargo lines Critical -> Urgent -> Routine (ties by id).
struct CargoPriorityDescending {
    bool operator()(const Inventory::ReliefCargo& a, const Inventory::ReliefCargo& b) const {
        if (a.priority() != b.priority()) return a.priority() > b.priority();
        return a.id() < b.id();
    }
};

// One priority queue per region, indexed through an unordered_map.
class DispatchBoard {
public:
    using Queue = PriorityDispatchQueue<DispatchRequest, MedicalUrgencyComparator>;

    void submit(const DispatchRequest& request) { queues_[request.region].push(request); }

    DispatchRequest next(const std::string& region) {
        auto it = queues_.find(region);
        if (it == queues_.end()) throw EmptyQueueException();
        return it->second.pop();
    }

    std::size_t pending(const std::string& region) const {
        auto it = queues_.find(region);
        return it == queues_.end() ? 0 : it->second.size();
    }

    std::vector<std::string> regions() const {
        std::vector<std::string> names;
        names.reserve(queues_.size());
        for (const auto& entry : queues_) names.push_back(entry.first);
        std::sort(names.begin(), names.end());
        return names;
    }

private:
    std::unordered_map<std::string, Queue> queues_;
};

}  // namespace Logistics
}  // namespace ReliefOps
