#include "TransportManifest.hpp"

#include <algorithm>
#include <iomanip>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "Dispatch.hpp"
#include "Exceptions.hpp"

namespace ReliefOps {
namespace Logistics {

using Inventory::ReliefCargo;

TransportManifest::TransportManifest()
    : id_(0), route_(), items_(nullptr), size_(0), capacity_(0) {}

TransportManifest::TransportManifest(int id, std::string route, std::size_t initialCapacity)
    : id_(id),
      route_(std::move(route)),
      items_(initialCapacity ? new ReliefCargo[initialCapacity] : nullptr),
      size_(0),
      capacity_(initialCapacity) {}

TransportManifest::TransportManifest(const TransportManifest& other)
    : id_(other.id_),
      route_(other.route_),
      items_(other.capacity_ ? new ReliefCargo[other.capacity_] : nullptr),
      size_(0),
      capacity_(other.capacity_) {
    try {
        for (; size_ < other.size_; ++size_) items_[size_] = other.items_[size_];  // deep copy
    } catch (...) {
        delete[] items_;  // constructor body threw: destructor will not run
        throw;
    }
}

TransportManifest::TransportManifest(TransportManifest&& other) noexcept
    : id_(other.id_),
      route_(std::move(other.route_)),
      items_(other.items_),
      size_(other.size_),
      capacity_(other.capacity_) {
    other.items_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

TransportManifest& TransportManifest::operator=(const TransportManifest& other) {
    if (this != &other) {
        TransportManifest copy(other);  // may throw: *this stays untouched
        swap(copy);
    }
    return *this;
}

TransportManifest& TransportManifest::operator=(TransportManifest&& other) noexcept {
    if (this != &other) {
        delete[] items_;
        id_ = other.id_;
        route_ = std::move(other.route_);
        items_ = other.items_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.items_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

TransportManifest::~TransportManifest() { delete[] items_; }

void TransportManifest::swap(TransportManifest& other) noexcept {
    std::swap(id_, other.id_);
    route_.swap(other.route_);
    std::swap(items_, other.items_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

void TransportManifest::grow() {
    const std::size_t newCapacity = capacity_ ? capacity_ * 2 : 4;
    ReliefCargo* fresh = new ReliefCargo[newCapacity];
    for (std::size_t i = 0; i < size_; ++i) fresh[i] = std::move(items_[i]);
    delete[] items_;
    items_ = fresh;
    capacity_ = newCapacity;
}

bool TransportManifest::add(const ReliefCargo& cargo) {
    for (std::size_t i = 0; i < size_; ++i) {
        if (items_[i] == cargo) {  // duplicate cargo line: merge instead of listing twice
            items_[i].quantityRef() += cargo.quantity();
            return false;
        }
    }
    if (size_ == capacity_) grow();
    items_[size_++] = cargo;
    return true;
}

bool TransportManifest::contains(const ReliefCargo& cargo) const {
    return std::any_of(begin(), end(), [&](const ReliefCargo& line) { return line == cargo; });
}

ReliefCargo& TransportManifest::at(std::size_t index) {
    if (index >= size_) throw std::out_of_range("TransportManifest::at: index out of range");
    return items_[index];
}

const ReliefCargo& TransportManifest::at(std::size_t index) const {
    if (index >= size_) throw std::out_of_range("TransportManifest::at: index out of range");
    return items_[index];
}

double TransportManifest::totalWeightKg() const {
    double total = 0.0;
    for (const ReliefCargo& line : *this) total += line.totalWeightKg();
    return total;
}

double TransportManifest::totalVolumeM3() const {
    double total = 0.0;
    for (const ReliefCargo& line : *this) total += line.totalVolumeM3();
    return total;
}

Priority TransportManifest::highestPriority() const {
    Priority highest = Priority::Routine;
    for (const ReliefCargo& line : *this) {
        if (line.priority() > highest) highest = line.priority();
    }
    return highest;
}

void TransportManifest::sortByPriority() {
    std::sort(items_, items_ + size_, CargoPriorityDescending());
}

bool TransportManifest::operator==(const TransportManifest& rhs) const {
    if (id_ != rhs.id_ || route_ != rhs.route_ || size_ != rhs.size_) return false;
    for (const ReliefCargo& line : *this) {
        const ReliefCargo* match = std::find(rhs.begin(), rhs.end(), line);
        if (match == rhs.end() || match->quantity() != line.quantity()) return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const TransportManifest& m) {
    os << "MANIFEST " << m.id_ << ' ' << std::quoted(m.route_) << ' ' << m.size_ << '\n';
    for (const ReliefCargo& line : m) os << line << '\n';
    return os;
}

std::istream& operator>>(std::istream& is, TransportManifest& m) {
    std::string tag, route;
    int id = 0;
    std::size_t count = 0;
    if (!(is >> tag >> id >> std::quoted(route) >> count)) return is;
    if (tag != "MANIFEST" || count > 1000000) {
        is.setstate(std::ios::failbit);
        return is;
    }
    TransportManifest parsed(id, route, count ? count : 1);
    for (std::size_t i = 0; i < count; ++i) {
        ReliefCargo line;
        if (!(is >> line)) return is;  // stream already flagged; m is left untouched
        parsed.add(line);
    }
    m = std::move(parsed);
    return is;
}

}  // namespace Logistics
}  // namespace ReliefOps
