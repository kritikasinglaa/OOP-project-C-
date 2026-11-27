// Manipulators.hpp - user-defined I/O manipulators + tabular printers (UNIT IV)
#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>

#include "DistributionDepot.hpp"
#include "TransportManifest.hpp"

namespace ReliefOps {
namespace IO {

constexpr int kTableWidth = 79;

// Parameterised manipulator: next field is left-aligned in `width` columns.
struct col {
    int width;
    explicit col(int w) : width(w) {}
};
inline std::ostream& operator<<(std::ostream& os, const col& c) {
    return os << std::left << std::setw(c.width);
}

// Parameterised manipulator: next field is right-aligned in `width` columns.
struct rcol {
    int width;
    explicit rcol(int w) : width(w) {}
};
inline std::ostream& operator<<(std::ostream& os, const rcol& c) {
    return os << std::right << std::setw(c.width);
}

// Value manipulators: format as "123.4 kg" / "1.234 m3" without disturbing stream state.
struct kg {
    double value;
    explicit kg(double v) : value(v) {}
};
inline std::ostream& operator<<(std::ostream& os, const kg& k) {
    std::ostringstream text;
    text << std::fixed << std::setprecision(1) << k.value << " kg";
    return os << text.str();  // width set by col/rcol applies to this single string
}

struct m3 {
    double value;
    explicit m3(double v) : value(v) {}
};
inline std::ostream& operator<<(std::ostream& os, const m3& v) {
    std::ostringstream text;
    text << std::fixed << std::setprecision(3) << v.value << " m3";
    return os << text.str();
}

// Function-style manipulators (usable as `os << tableRule`).
inline std::ostream& tableRule(std::ostream& os) {
    return os << std::setfill('-') << std::setw(kTableWidth) << "" << std::setfill(' ') << '\n';
}

inline std::ostream& manifestHeader(std::ostream& os) {
    os << col(5) << "ID" << col(24) << "Item" << col(15) << "Type" << rcol(6) << "Qty"
       << rcol(11) << "Weight" << rcol(11) << "Volume" << "  " << "Priority" << '\n';
    return os;
}

inline std::ostream& depotHeader(std::ostream& os) {
    os << col(5) << "ID" << col(24) << "Item" << col(15) << "Type" << rcol(6) << "Qty"
       << rcol(8) << "Release" << rcol(11) << "Weight" << "  " << "Priority" << '\n';
    return os;
}

inline void printManifestTable(std::ostream& os, const Logistics::TransportManifest& manifest) {
    const std::ios::fmtflags saved = os.flags();
    os << "Manifest #" << manifest.id() << "  route: " << manifest.route() << '\n'
       << tableRule << manifestHeader << tableRule;
    for (const Inventory::ReliefCargo& c : manifest) {
        os << col(5) << c.id() << col(24) << c.label() << col(15) << toString(c.type())
           << rcol(6) << c.quantity() << rcol(11) << kg(c.totalWeightKg()) << rcol(11)
           << m3(c.totalVolumeM3()) << "  " << toString(c.priority()) << '\n';
    }
    os << tableRule << col(44) << "TOTAL" << rcol(6) << "" << rcol(11)
       << kg(manifest.totalWeightKg()) << rcol(11) << m3(manifest.totalVolumeM3()) << "  "
       << toString(manifest.highestPriority()) << '\n';
    os.flags(saved);
}

inline void printDepotTable(std::ostream& os, const Inventory::DistributionDepot& depot) {
    const std::ios::fmtflags saved = os.flags();
    os << "Depot: " << depot.name() << "  (protected reserve " << std::setprecision(0)
       << std::fixed << depot.reserveRatio() * 100.0 << "%)\n"
       << tableRule << depotHeader << tableRule;
    for (const Inventory::ReliefCargo& c : depot.stock()) {
        os << col(5) << c.id() << col(24) << c.label() << col(15) << toString(c.type())
           << rcol(6) << c.quantity() << rcol(8) << depot.availableQuantity(c.id()) << rcol(11)
           << kg(c.totalWeightKg()) << "  " << toString(c.priority()) << '\n';
    }
    os << tableRule;
    os.flags(saved);
}

}  // namespace IO
}  // namespace ReliefOps
