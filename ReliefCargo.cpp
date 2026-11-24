#include "ReliefCargo.hpp"

#include <cstring>
#include <iomanip>
#include <istream>
#include <ostream>
#include <string>
#include <utility>

#include "Exceptions.hpp"

namespace ReliefOps {
namespace Inventory {

namespace {

char* duplicate(const char* text) {
    const char* src = text ? text : "";
    const std::size_t n = std::strlen(src);
    char* out = new char[n + 1];
    std::memcpy(out, src, n + 1);
    return out;
}

}  // namespace

ReliefCargo::ReliefCargo()
    : id_(0),
      label_(duplicate("")),
      type_(SupplyType::RationKit),
      quantity_(0),
      unitWeightKg_(0.0),
      unitVolumeM3_(0.0),
      priority_(Logistics::Priority::Routine) {}

ReliefCargo::ReliefCargo(int id, const char* label, SupplyType type, int quantity,
                         double unitWeightKg, double unitVolumeM3,
                         Logistics::Priority priority)
    : id_(id),
      label_(nullptr),
      type_(type),
      quantity_(quantity),
      unitWeightKg_(unitWeightKg),
      unitVolumeM3_(unitVolumeM3),
      priority_(priority) {
    // Validate before allocating so a throwing constructor can never leak.
    if (quantity < 0 || unitWeightKg < 0.0 || unitVolumeM3 < 0.0) {
        throw ERLException("ReliefCargo: quantity, weight and volume must be non-negative");
    }
    label_ = duplicate(label);
}

ReliefCargo::ReliefCargo(const ReliefCargo& other)
    : id_(other.id_),
      label_(duplicate(other.label_)),
      type_(other.type_),
      quantity_(other.quantity_),
      unitWeightKg_(other.unitWeightKg_),
      unitVolumeM3_(other.unitVolumeM3_),
      priority_(other.priority_) {}

ReliefCargo::ReliefCargo(ReliefCargo&& other) noexcept
    : id_(other.id_),
      label_(other.label_),
      type_(other.type_),
      quantity_(other.quantity_),
      unitWeightKg_(other.unitWeightKg_),
      unitVolumeM3_(other.unitVolumeM3_),
      priority_(other.priority_) {
    other.label_ = nullptr;
    other.quantity_ = 0;
}

ReliefCargo& ReliefCargo::operator=(const ReliefCargo& other) {
    if (this != &other) {
        char* fresh = duplicate(other.label_);  // allocate first: strong guarantee
        delete[] label_;
        label_ = fresh;
        id_ = other.id_;
        type_ = other.type_;
        quantity_ = other.quantity_;
        unitWeightKg_ = other.unitWeightKg_;
        unitVolumeM3_ = other.unitVolumeM3_;
        priority_ = other.priority_;
    }
    return *this;
}

ReliefCargo& ReliefCargo::operator=(ReliefCargo&& other) noexcept {
    if (this != &other) {
        delete[] label_;
        label_ = other.label_;
        other.label_ = nullptr;
        id_ = other.id_;
        type_ = other.type_;
        quantity_ = other.quantity_;
        unitWeightKg_ = other.unitWeightKg_;
        unitVolumeM3_ = other.unitVolumeM3_;
        priority_ = other.priority_;
        other.quantity_ = 0;
    }
    return *this;
}

ReliefCargo::~ReliefCargo() { delete[] label_; }

ReliefCargo ReliefCargo::slice(int quantity) const {
    if (quantity < 0) throw ERLException("ReliefCargo::slice: negative quantity");
    ReliefCargo part(*this);
    part.quantity_ = quantity;
    return part;
}

bool ReliefCargo::operator==(const ReliefCargo& rhs) const {
    return id_ == rhs.id_ && type_ == rhs.type_;
}

std::ostream& operator<<(std::ostream& os, const ReliefCargo& c) {
    const std::ios::fmtflags oldFlags = os.flags();
    const std::streamsize oldPrecision = os.precision(10);
    os << std::defaultfloat;
    os << c.id_ << ' ' << std::quoted(c.label()) << ' ' << static_cast<int>(c.type_) << ' '
       << c.quantity_ << ' ' << c.unitWeightKg_ << ' ' << c.unitVolumeM3_ << ' '
       << static_cast<int>(c.priority_);
    os.precision(oldPrecision);
    os.flags(oldFlags);
    return os;
}

std::istream& operator>>(std::istream& is, ReliefCargo& c) {
    int id = 0, type = 0, quantity = 0, priority = 0;
    double weight = 0.0, volume = 0.0;
    std::string label;
    if (!(is >> id >> std::quoted(label) >> type >> quantity >> weight >> volume >> priority)) {
        return is;  // failbit already set by the failed extraction
    }
    if (type < 0 || type >= kSupplyTypeCount || priority < 0 || priority > 2 ||
        quantity < 0 || weight < 0.0 || volume < 0.0) {
        is.setstate(std::ios::failbit);
        return is;
    }
    c = ReliefCargo(id, label.c_str(), static_cast<SupplyType>(type), quantity, weight, volume,
                    static_cast<Logistics::Priority>(priority));
    return is;
}

void adjustStock(ReliefCargo& stock, int delta) {
    int& quantity = stock.quantityRef();  // return-by-reference in action
    if (quantity + delta < 0) {
        throw ERLException(std::string("adjustStock: would drive '") + stock.label() +
                           "' below zero");
    }
    quantity += delta;
}

}  // namespace Inventory
}  // namespace ReliefOps
