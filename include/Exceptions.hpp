// Exceptions.hpp - exception framework deriving from std::exception (UNIT IV)
#pragma once

#include <exception>
#include <sstream>
#include <string>
#include <utility>

namespace ReliefOps {

// Root of every ERL-HUB failure; catch this to handle any domain error.
class ERLException : public std::exception {
public:
    explicit ERLException(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

// Load exceeds a vehicle's weight or volume limit.
class CargoOverweightException : public ERLException {
public:
    CargoOverweightException(const std::string& vehicle, const std::string& dimension,
                             double load, double limit, const std::string& unit)
        : ERLException(describe(vehicle, dimension, load, limit, unit)),
          excess_(load - limit) {}

    double excess() const noexcept { return excess_; }

private:
    static std::string describe(const std::string& vehicle, const std::string& dimension,
                                double load, double limit, const std::string& unit) {
        std::ostringstream o;
        o.setf(std::ios::fixed);
        o.precision(2);
        o << "Cargo overweight on " << vehicle << ": " << dimension << ' ' << load << ' '
          << unit << " exceeds limit " << limit << ' ' << unit;
        return o.str();
    }
    double excess_;
};

// Not enough fuel to complete a leg.
class FuelDepletionException : public ERLException {
public:
    FuelDepletionException(const std::string& vehicle, double neededLitres,
                           double availableLitres)
        : ERLException(describe(vehicle, neededLitres, availableLitres)),
          shortfall_(neededLitres - availableLitres) {}

    double shortfallLitres() const noexcept { return shortfall_; }

private:
    static std::string describe(const std::string& vehicle, double needed, double available) {
        std::ostringstream o;
        o.setf(std::ios::fixed);
        o.precision(1);
        o << "Fuel depletion on " << vehicle << ": needs " << needed << " L, has "
          << available << " L";
        return o.str();
    }
    double shortfall_;
};

// A route that a given vehicle class cannot traverse (collapsed road, flood, no airspace).
class DamagedCorridorException : public ERLException {
public:
    DamagedCorridorException(std::string corridor, const std::string& reason)
        : ERLException("Corridor '" + corridor + "' impassable: " + reason),
          corridor_(std::move(corridor)) {}

    const std::string& corridor() const noexcept { return corridor_; }

private:
    std::string corridor_;
};

// A depot cannot release the requested stock (shortage or protected reserve).
class InsufficientStockException : public ERLException {
public:
    InsufficientStockException(const std::string& depot, const std::string& item,
                               int requested, int available)
        : ERLException(describe(depot, item, requested, available)),
          requested_(requested), available_(available) {}

    int requested() const noexcept { return requested_; }
    int available() const noexcept { return available_; }

private:
    static std::string describe(const std::string& depot, const std::string& item,
                                int requested, int available) {
        std::ostringstream o;
        o << "Depot '" << depot << "' cannot release " << requested << " x " << item
          << " (releasable: " << available << ")";
        return o.str();
    }
    int requested_;
    int available_;
};

// Binary log / audit file problems (missing, truncated, corrupt, write failure).
class PersistenceException : public ERLException {
public:
    explicit PersistenceException(const std::string& what)
        : ERLException("Persistence error: " + what) {}
};

// pop()/top() on an empty dispatch queue.
class EmptyQueueException : public ERLException {
public:
    EmptyQueueException() : ERLException("PriorityDispatchQueue is empty") {}
};

}  // namespace ReliefOps
