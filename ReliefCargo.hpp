// ReliefCargo.hpp - one line of relief cargo (UNIT II: ADT, dynamic memory, operators)
#pragma once

#include <iosfwd>

#include "Types.hpp"

namespace ReliefOps {
namespace Inventory {

class ReliefCargo {
public:
    // --- construction / rule of five (owns a heap-allocated label) -----------
    ReliefCargo();
    ReliefCargo(int id, const char* label, SupplyType type, int quantity,
                double unitWeightKg, double unitVolumeM3,
                Logistics::Priority priority = Logistics::Priority::Routine);
    ReliefCargo(const ReliefCargo& other);                 // deep copy
    ReliefCargo(ReliefCargo&& other) noexcept;
    ReliefCargo& operator=(const ReliefCargo& other);      // deep copy, self-safe
    ReliefCargo& operator=(ReliefCargo&& other) noexcept;
    ~ReliefCargo();

    // --- read-only interface (information hiding: no public data) ------------
    int id() const { return id_; }
    const char* label() const { return label_ ? label_ : ""; }
    SupplyType type() const { return type_; }
    int quantity() const { return quantity_; }
    double unitWeightKg() const { return unitWeightKg_; }
    double unitVolumeM3() const { return unitVolumeM3_; }
    Logistics::Priority priority() const { return priority_; }

    // --- inline payload maths ---------------------------------------------------
    inline double totalWeightKg() const { return quantity_ * unitWeightKg_; }
    inline double totalVolumeM3() const { return quantity_ * unitVolumeM3_; }

    // Return by reference: lets callers update the quantity in place (UNIT I).
    int& quantityRef() { return quantity_; }

    // Copy of this line carrying a different quantity (used to split loads).
    ReliefCargo slice(int quantity) const;

    // "Same cargo line" test: identical id and supply type (duplicate detection).
    bool operator==(const ReliefCargo& rhs) const;
    bool operator!=(const ReliefCargo& rhs) const { return !(*this == rhs); }

    // Text serialisation:  id "label" type qty unitWeight unitVolume priority
    friend std::ostream& operator<<(std::ostream& os, const ReliefCargo& cargo);
    friend std::istream& operator>>(std::istream& is, ReliefCargo& cargo);

    // The incident commander may override access barriers during critical triage.
    friend class Logistics::IncidentCommander;

private:
    int id_;
    char* label_;  // dynamically allocated, owned
    SupplyType type_;
    int quantity_;
    double unitWeightKg_;
    double unitVolumeM3_;
    Logistics::Priority priority_;  // only IncidentCommander may rewrite this
};

// Call by reference: adjust a stock line in place (throws if it would go negative).
void adjustStock(ReliefCargo& stock, int delta);

}  // namespace Inventory
}  // namespace ReliefOps
