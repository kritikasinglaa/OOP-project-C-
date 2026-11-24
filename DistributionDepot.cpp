#include "DistributionDepot.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Exceptions.hpp"

namespace ReliefOps {
namespace Inventory {

DistributionDepot::DistributionDepot(std::string name, double reserveRatio)
    : name_(std::move(name)), reserveRatio_(reserveRatio) {
    if (reserveRatio < 0.0 || reserveRatio > 1.0) {
        throw ERLException("DistributionDepot: reserve ratio must be within [0, 1]");
    }
}

DistributionDepot& DistributionDepot::operator+=(const ReliefCargo& cargo) {
    if (cargo.quantity() <= 0) throw ERLException("Restock quantity must be positive");

    auto it = std::find_if(stock_.begin(), stock_.end(),
                           [&](const ReliefCargo& line) { return line.id() == cargo.id(); });
    if (it == stock_.end()) {
        stock_.push_back(cargo);
        parLevel_[cargo.id()] = cargo.quantity();
    } else {
        if (it->type() != cargo.type()) {
            throw ERLException("Restock type mismatch for stock id " + std::to_string(cargo.id()));
        }
        it->quantityRef() += cargo.quantity();
        int& par = parLevel_[cargo.id()];
        par = std::max(par, it->quantity());
    }
    return *this;
}

DistributionDepot& DistributionDepot::operator-=(const ReliefCargo& cargo) {
    if (cargo.quantity() <= 0) throw ERLException("Deduction quantity must be positive");

    auto it = std::find_if(stock_.begin(), stock_.end(),
                           [&](const ReliefCargo& line) { return line.id() == cargo.id(); });
    if (it == stock_.end()) {
        throw InsufficientStockException(name_, cargo.label(), cargo.quantity(), 0);
    }
    const int releasable = availableQuantity(cargo.id());
    if (cargo.quantity() > releasable) {
        throw InsufficientStockException(name_, it->label(), cargo.quantity(), releasable);
    }
    it->quantityRef() -= cargo.quantity();
    return *this;
}

ReliefCargo& DistributionDepot::findStock(int id) {
    auto it = std::find_if(stock_.begin(), stock_.end(),
                           [id](const ReliefCargo& line) { return line.id() == id; });
    if (it == stock_.end()) {
        throw ERLException("Depot '" + name_ + "' has no stock line #" + std::to_string(id));
    }
    return *it;
}

const ReliefCargo& DistributionDepot::findStock(int id) const {
    auto it = std::find_if(stock_.begin(), stock_.end(),
                           [id](const ReliefCargo& line) { return line.id() == id; });
    if (it == stock_.end()) {
        throw ERLException("Depot '" + name_ + "' has no stock line #" + std::to_string(id));
    }
    return *it;
}

bool DistributionDepot::has(int id) const {
    return std::any_of(stock_.begin(), stock_.end(),
                       [id](const ReliefCargo& line) { return line.id() == id; });
}

int DistributionDepot::reserveUnits(int id) const {
    auto par = parLevel_.find(id);
    if (par == parLevel_.end()) return 0;
    return static_cast<int>(std::ceil(par->second * reserveRatio_ - 1e-9));
}

int DistributionDepot::availableQuantity(int id) const {
    auto it = std::find_if(stock_.begin(), stock_.end(),
                           [id](const ReliefCargo& line) { return line.id() == id; });
    if (it == stock_.end()) return 0;
    return std::max(0, it->quantity() - reserveUnits(id));
}

double DistributionDepot::totalWeightKg() const {
    double total = 0.0;
    for (const ReliefCargo& line : stock_) total += line.totalWeightKg();
    return total;
}

}  // namespace Inventory
}  // namespace ReliefOps
