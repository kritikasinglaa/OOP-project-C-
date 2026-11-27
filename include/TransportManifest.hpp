// TransportManifest.hpp - dynamically allocated cargo manifest (UNIT II)
#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>

#include "ReliefCargo.hpp"
#include "Types.hpp"

namespace ReliefOps {
namespace Logistics {

class TransportManifest {
public:
    TransportManifest();  // empty manifest, no allocation
    // "Dynamic constructor": allocates storage for `initialCapacity` lines on the heap.
    TransportManifest(int id, std::string route, std::size_t initialCapacity = 4);
    TransportManifest(const TransportManifest& other);  // deep copy
    TransportManifest(TransportManifest&& other) noexcept;
    TransportManifest& operator=(const TransportManifest& other);  // copy-and-swap
    TransportManifest& operator=(TransportManifest&& other) noexcept;
    ~TransportManifest();  // releases the heap array

    int id() const { return id_; }
    const std::string& route() const { return route_; }
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }

    // Adds a line. Returns true for a new line, false if it was a duplicate cargo
    // line (same id + type) whose quantity was merged into the existing one.
    bool add(const Inventory::ReliefCargo& cargo);
    bool contains(const Inventory::ReliefCargo& cargo) const;

    Inventory::ReliefCargo& at(std::size_t index);              // return by reference
    const Inventory::ReliefCargo& at(std::size_t index) const;

    const Inventory::ReliefCargo* begin() const { return items_; }
    const Inventory::ReliefCargo* end() const { return items_ + size_; }

    double totalWeightKg() const;
    double totalVolumeM3() const;
    Priority highestPriority() const;

    // Sorts lines Critical -> Routine using the CargoPriorityDescending functor.
    void sortByPriority();

    // Identity check: same id, route and the same lines with the same quantities.
    bool operator==(const TransportManifest& rhs) const;
    bool operator!=(const TransportManifest& rhs) const { return !(*this == rhs); }

    // Text serialisation:  MANIFEST id "route" count \n  <cargo line> \n ...
    friend std::ostream& operator<<(std::ostream& os, const TransportManifest& m);
    friend std::istream& operator>>(std::istream& is, TransportManifest& m);

private:
    void grow();
    void swap(TransportManifest& other) noexcept;

    int id_;
    std::string route_;
    Inventory::ReliefCargo* items_;  // heap array, owned
    std::size_t size_;
    std::size_t capacity_;
};

}  // namespace Logistics
}  // namespace ReliefOps
