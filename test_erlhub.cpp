// Self-contained test suite (no framework needed). Exit code 0 = all passed.
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "Allocation.hpp"
#include "Dispatch.hpp"
#include "DistributionDepot.hpp"
#include "Exceptions.hpp"
#include "IncidentCommander.hpp"
#include "Manipulators.hpp"
#include "Persistence.hpp"
#include "ReliefCargo.hpp"
#include "TransportManifest.hpp"
#include "Vehicles.hpp"

namespace Log = ReliefOps::Logistics;
namespace Inv = ReliefOps::Inventory;
using namespace ReliefOps;

static int g_checks = 0;
static int g_failed = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failed;                                                               \
            std::cout << "  FAIL  " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; \
        }                                                                             \
    } while (0)

#define CHECK_THROWS(expr, ExType)                                                    \
    do {                                                                              \
        ++g_checks;                                                                   \
        bool caught = false;                                                          \
        try {                                                                         \
            expr;                                                                     \
        } catch (const ExType&) {                                                     \
            caught = true;                                                            \
        } catch (...) {                                                               \
        }                                                                             \
        if (!caught) {                                                                \
            ++g_failed;                                                               \
            std::cout << "  FAIL  " << __FILE__ << ":" << __LINE__                    \
                      << "  expected " #ExType " from: " #expr "\n";                  \
        }                                                                             \
    } while (0)

static Inv::ReliefCargo cargo(int id, Inv::SupplyType t, int qty, double w = 1.0,
                              double v = 0.01, Log::Priority p = Log::Priority::Routine) {
    return Inv::ReliefCargo(id, ("Item" + std::to_string(id)).c_str(), t, qty, w, v, p);
}

static Inv::DistributionDepot makeDepot(double reserve = 0.10) {
    Inv::DistributionDepot d("Test Depot", reserve);
    d += Inv::ReliefCargo(102, "Trauma", Inv::SupplyType::TraumaMedicine, 150, 2.8, 0.006,
                          Log::Priority::Critical);
    d += Inv::ReliefCargo(103, "Water", Inv::SupplyType::CleanWater, 400, 20.5, 0.021,
                          Log::Priority::Urgent);
    d += Inv::ReliefCargo(101, "Rations", Inv::SupplyType::RationKit, 800, 4.5, 0.012);
    return d;
}

static void testCargo() {
    std::cout << "ReliefCargo\n";
    Inv::ReliefCargo a(1, "Original", Inv::SupplyType::RationKit, 10, 2.0, 0.5);
    Inv::ReliefCargo b(a);  // deep copy of the heap label
    CHECK(a.label() != b.label());
    CHECK(std::string(b.label()) == "Original");
    b = Inv::ReliefCargo(2, "Other", Inv::SupplyType::CleanWater, 3, 1.0, 1.0);
    CHECK(std::string(a.label()) == "Original");  // a unaffected
    CHECK(a.totalWeightKg() == 20.0 && a.totalVolumeM3() == 5.0);
    a = a;  // self-assignment must be safe
    CHECK(std::string(a.label()) == "Original");
    CHECK_THROWS(Inv::ReliefCargo(1, "bad", Inv::SupplyType::RationKit, -1, 1, 1), ERLException);
    CHECK(Inv::ReliefCargo(5, "x", Inv::SupplyType::RationKit, 1, 1, 1) ==
          Inv::ReliefCargo(5, "y", Inv::SupplyType::RationKit, 99, 9, 9));  // same id+type
    CHECK(Inv::ReliefCargo(5, "x", Inv::SupplyType::RationKit, 1, 1, 1) !=
          Inv::ReliefCargo(5, "x", Inv::SupplyType::CleanWater, 1, 1, 1));

    std::stringstream wire;
    Inv::ReliefCargo spaced(7, "Label with \"quotes\" and spaces", Inv::SupplyType::ThermalBlanket,
                            12, 0.9, 0.004, Log::Priority::Urgent);
    wire << spaced;
    Inv::ReliefCargo back;
    wire >> back;
    CHECK(wire && std::string(back.label()) == spaced.label() && back.quantity() == 12 &&
          back.priority() == Log::Priority::Urgent);
    std::stringstream garbage("1 \"x\" 99 1 1 1 0");  // supply type 99 is invalid
    Inv::ReliefCargo untouched;
    garbage >> untouched;
    CHECK(garbage.fail());
}

static void testDepot() {
    std::cout << "DistributionDepot + operators + friend commander\n";
    Inv::DistributionDepot d = makeDepot(0.10);
    CHECK(d.availableQuantity(102) == 135);  // 150 minus a 15-unit reserve
    d -= cargo(102, Inv::SupplyType::TraumaMedicine, 100);
    CHECK(d.findStock(102).quantity() == 50);
    CHECK_THROWS(d -= cargo(102, Inv::SupplyType::TraumaMedicine, 36), InsufficientStockException);
    CHECK(d.findStock(102).quantity() == 50);  // failed deduction changed nothing
    d += cargo(102, Inv::SupplyType::TraumaMedicine, 30);
    CHECK(d.findStock(102).quantity() == 80);
    CHECK_THROWS(d += cargo(102, Inv::SupplyType::CleanWater, 1), ERLException);  // type clash
    CHECK_THROWS(d -= cargo(999, Inv::SupplyType::RationKit, 1), InsufficientStockException);
    CHECK_THROWS(d.findStock(999), ERLException);

    Log::IncidentCommander cmd("test");
    const int reserve = d.findStock(102).quantity() - d.availableQuantity(102);
    CHECK(reserve == 15);
    Inv::ReliefCargo taken = cmd.commandeer(d, 102, d.findStock(102).quantity());  // takes reserve too
    CHECK(d.findStock(102).quantity() == 0);
    CHECK(taken.priority() == Log::Priority::Critical);
    Inv::ReliefCargo& water = d.findStock(103);
    cmd.escalate(water, Log::Priority::Critical);
    CHECK(water.priority() == Log::Priority::Critical);
    cmd.releaseReserve(d);
    CHECK(d.availableQuantity(103) == 400);
    CHECK(cmd.overridesIssued() == 3);

    Inv::ReliefCargo line = cargo(1, Inv::SupplyType::RationKit, 5);
    Inv::adjustStock(line, 3);
    CHECK(line.quantity() == 8);
    CHECK_THROWS(Inv::adjustStock(line, -9), ERLException);
    CHECK(line.quantity() == 8);
}

static void testAllocation() {
    std::cout << "Overloaded allocateSupplies\n";
    {
        Inv::DistributionDepot d = makeDepot();
        Log::TransportManifest m(1, "volume");
        const int moved = Log::allocateSupplies(d, m, 1.44);  // trauma 135 (0.81) + water 30 (0.63)
        CHECK(moved == 165);
        CHECK(m.totalVolumeM3() <= 1.44 + 1e-9);
        CHECK(d.findStock(102).quantity() == 15);  // only the reserve remains
    }
    {
        Inv::DistributionDepot d = makeDepot();
        Log::TransportManifest m(2, "priority");
        const int moved = Log::allocateSupplies(d, m, Log::Priority::Urgent, 5);
        CHECK(moved == 135 + 360);
        CHECK(m.at(0).id() == 102);  // Critical line first
        Log::TransportManifest none(3, "none");
        CHECK(Log::allocateSupplies(d, none, Log::Priority::Critical, 5) == 0);  // reserve only
    }
    {
        Inv::DistributionDepot d = makeDepot();
        Log::TransportManifest m(4, "type");
        CHECK(Log::allocateSupplies(d, m, Inv::SupplyType::CleanWater, 100) == 100);
        CHECK(Log::allocateSupplies(d, m, Inv::SupplyType::CleanWater, 100) == 100);
        CHECK(m.size() == 1 && m.at(0).quantity() == 200);  // duplicate lines merged
        CHECK(Log::allocateSupplies(d, m, Inv::SupplyType::CleanWater, 1000) == 160);  // capped at release
    }
}

static void testManifest() {
    std::cout << "TransportManifest (dynamic memory, operators)\n";
    Log::TransportManifest m(10, "route", 1);
    for (int i = 0; i < 9; ++i) m.add(cargo(100 + i, Inv::SupplyType::RationKit, i + 1));
    CHECK(m.size() == 9 && m.capacity() >= 9);
    Log::TransportManifest copy(m);
    CHECK(copy == m);
    CHECK(&copy.at(0) != &m.at(0));
    copy.at(0).quantityRef() = 12345;
    CHECK(m.at(0).quantity() == 1);  // deep copy: original untouched
    CHECK(copy != m);
    Log::TransportManifest assigned;
    assigned = m;
    CHECK(assigned == m);
    Log::TransportManifest moved(std::move(assigned));
    CHECK(moved == m && assigned.size() == 0);
    CHECK(!m.add(cargo(100, Inv::SupplyType::RationKit, 5)));  // duplicate merges
    CHECK(m.at(0).quantity() == 6);
    CHECK_THROWS(m.at(99), std::out_of_range);

    std::stringstream wire;
    wire << m;
    Log::TransportManifest back;
    wire >> back;
    CHECK(wire && back == m);
    std::stringstream bad("NOTAMANIFEST 1 \"r\" 0");
    Log::TransportManifest untouched = m;
    bad >> untouched;
    CHECK(bad.fail() && untouched == m);

    Log::TransportManifest sorted(11, "sort");
    sorted.add(cargo(1, Inv::SupplyType::RationKit, 1, 1, 0.01, Log::Priority::Routine));
    sorted.add(cargo(2, Inv::SupplyType::RationKit, 1, 1, 0.01, Log::Priority::Critical));
    sorted.add(cargo(3, Inv::SupplyType::RationKit, 1, 1, 0.01, Log::Priority::Urgent));
    sorted.sortByPriority();
    CHECK(sorted.at(0).id() == 2 && sorted.at(1).id() == 3 && sorted.at(2).id() == 1);
    CHECK(sorted.highestPriority() == Log::Priority::Critical);
}

static void testVehicles() {
    std::cout << "Vehicle hierarchy, containership, polymorphism, statics\n";
    const int before = Log::TransportVehicle::operationalVehicles();
    {
        std::vector<Log::ReliefVehicle*> fleet;
        fleet.push_back(new Log::AerialTransport({1, "Heli", 1200, 6, 900}));
        fleet.push_back(new Log::GroundConvoy({2, "Truck", 9000, 40, 600}));
        auto* amph = new Log::AmphibiousCraft({3, "Amph", 3500, 18, 500}, 6);
        fleet.push_back(amph);
        CHECK(Log::TransportVehicle::operationalVehicles() == before + 3);

        // Virtual base: exactly one ReliefVehicle shared by both parents.
        const Log::ReliefVehicle* viaGround = static_cast<Log::GroundConvoy*>(amph);
        const Log::ReliefVehicle* viaMarine = static_cast<Log::MarineVessel*>(amph);
        CHECK(viaGround == viaMarine);
        CHECK(static_cast<Log::ReliefVehicle*>(amph) == viaGround);

        const Log::Route highway{"NH", 100, 0.2, false, true};
        const Log::Route flood{"Flood", 100, 0.2, true, true};
        const Log::Route rubble{"Rubble", 50, 0.88, false, true};
        const Log::Route noAir{"Storm", 50, 0.2, false, false};

        CHECK(fleet[0]->evaluateRoute(highway).mode == "air");
        CHECK(fleet[1]->evaluateRoute(highway).mode == "ground");
        CHECK(fleet[2]->evaluateRoute(highway).mode == "amphibious (land mode)");
        CHECK(fleet[2]->evaluateRoute(flood).mode == "amphibious (water mode)");
        CHECK(fleet[0]->evaluateRoute(highway).etaHours < fleet[1]->evaluateRoute(highway).etaHours);
        CHECK_THROWS(fleet[1]->evaluateRoute(flood), DamagedCorridorException);
        CHECK_THROWS(fleet[1]->evaluateRoute(rubble), DamagedCorridorException);
        CHECK(fleet[2]->evaluateRoute(rubble).feasible);  // amphibious tolerates worse roads
        CHECK_THROWS(fleet[0]->evaluateRoute(noAir), DamagedCorridorException);
        CHECK(fleet[0]->kind() == "Aerial transport" && fleet[2]->kind() == "Amphibious medical craft");

        for (Log::ReliefVehicle* v : fleet) delete v;  // virtual destructors via base pointers
    }
    CHECK(Log::TransportVehicle::operationalVehicles() == before);

    // Exception safety of load()/dispatch().
    Log::AerialTransport heli({10, "Heli", 1000, 2.0, 100});
    Log::TransportManifest heavy(1, "heavy");
    heavy.add(cargo(1, Inv::SupplyType::CleanWater, 100, 20.0, 0.001));  // 2000 kg
    CHECK_THROWS(heli.load(heavy), CargoOverweightException);
    Log::TransportManifest bulky(2, "bulky");
    bulky.add(cargo(2, Inv::SupplyType::RationKit, 10, 1.0, 1.0));  // 10 m3
    CHECK_THROWS(heli.load(bulky), CargoOverweightException);
    CHECK(heli.manifest().size() == 0);  // strong guarantee: still empty

    Log::TransportManifest light(3, "light");
    light.add(cargo(3, Inv::SupplyType::RationKit, 10, 10.0, 0.01));  // 100 kg
    heli.load(light);
    CHECK(heli.manifest() == light && heli.loadedKg() == 100.0);

    const double fuelBefore = heli.fuelLitres();
    const double tonnesBefore = Log::TransportVehicle::totalDispatchedTonnes();
    CHECK_THROWS(heli.dispatch({"Far", 5000, 0.0, false, true}), FuelDepletionException);
    CHECK(heli.fuelLitres() == fuelBefore);  // nothing consumed on failure
    CHECK(Log::TransportVehicle::totalDispatchedTonnes() == tonnesBefore);
    CHECK(heli.gps().odometerKm() == 0.0);

    const Log::RouteEvaluation ok = heli.dispatch({"Near", 10, 0.0, false, true});
    CHECK(ok.feasible && heli.fuelLitres() < fuelBefore);
    CHECK(heli.gps().odometerKm() == 10.0 && heli.gps().lastWaypoint() == "Near");
    CHECK(Log::TransportVehicle::totalDispatchedTonnes() > tonnesBefore);
    CHECK(heli.lifeSupport().oxygenHours() < 12.0);
    Log::TransportManifest delivered = heli.unload();
    CHECK(delivered == light && heli.manifest().size() == 0);

    // Operational counter follows setOperational().
    const int n = Log::TransportVehicle::operationalVehicles();
    heli.setOperational(false);
    CHECK(Log::TransportVehicle::operationalVehicles() == n - 1);
    heli.setOperational(false);  // idempotent
    CHECK(Log::TransportVehicle::operationalVehicles() == n - 1);
    heli.setOperational(true);
    CHECK(Log::TransportVehicle::operationalVehicles() == n);
    CHECK_THROWS(Log::AerialTransport({99, "bad", -1, 1, 1}), ERLException);
    CHECK(Log::TransportVehicle::operationalVehicles() == n);  // failed ctor never counted
}

static void testPersistence() {
    std::cout << "Binary persistence\n";
    const std::string logPath = "test_transit.bin";
    const std::string auditPath = "test_audit.bin";

    Log::TransitLog log;
    log.record(1, 2, 123.5, 60.0, 1.5, "DELIVERED");
    log.record(3, 4, 0.0, 5.0, 0.1, "A-VERY-LONG-STATUS-TEXT-THAT-GETS-TRUNCATED");
    log.save(logPath);
    const Log::TransitLog back = Log::TransitLog::load(logPath);
    CHECK(back.size() == 2);
    CHECK(back.entries()[0].manifestId == 1 && back.entries()[0].weightKg == 123.5);
    CHECK(std::string(back.entries()[0].status) == "DELIVERED");
    CHECK(std::string(back.entries()[1].status).size() == 15);  // truncated, still terminated
    {
        std::ifstream tmp(logPath + ".tmp");
        CHECK(!tmp.good());  // atomic write leaves no temp file behind
    }

    Log::TransportManifest m1(1, "A -> B");
    m1.add(Inv::ReliefCargo(1, "Unicode \xE2\x9C\x93 label", Inv::SupplyType::CleanWater, 5, 1.5,
                            0.25, Log::Priority::Critical));
    Log::TransportManifest m2(2, "empty");
    Log::saveManifestAudit(auditPath, {m1, m2});
    const std::vector<Log::TransportManifest> audit = Log::loadManifestAudit(auditPath);
    CHECK(audit.size() == 2 && audit[0] == m1 && audit[1] == m2);
    CHECK(std::string(audit[0].at(0).label()) == m1.at(0).label());
    CHECK(audit[0].at(0).priority() == Log::Priority::Critical);

    // Failure modes must raise PersistenceException, never crash.
    CHECK_THROWS(Log::TransitLog::load("does_not_exist.bin"), PersistenceException);
    CHECK_THROWS(Log::loadManifestAudit(logPath), PersistenceException);  // wrong magic
    std::ifstream in(auditPath, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    for (std::size_t cut : {std::size_t(2), std::size_t(10), bytes.size() / 2, bytes.size() - 1}) {
        std::ofstream out("test_cut.bin", std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(cut));
        out.close();
        CHECK_THROWS(Log::loadManifestAudit("test_cut.bin"), PersistenceException);
    }
    CHECK_THROWS(Log::TransitLog().save("no_such_dir/x.bin"), PersistenceException);

    std::remove(logPath.c_str());
    std::remove(auditPath.c_str());
    std::remove("test_cut.bin");
}

static void testQueueAndFunctors() {
    std::cout << "PriorityDispatchQueue<T> and functors\n";
    Log::PriorityDispatchQueue<int> maxQ;
    for (int v : {5, 1, 9, 3}) maxQ.push(v);
    CHECK(maxQ.top() == 9 && maxQ.size() == 4);
    CHECK(maxQ.pop() == 9 && maxQ.pop() == 5);
    CHECK(maxQ.snapshot() == (std::vector<int>{3, 1}));
    maxQ.clear();
    CHECK(maxQ.empty());
    CHECK_THROWS(maxQ.pop(), EmptyQueueException);
    CHECK_THROWS(maxQ.top(), EmptyQueueException);

    Log::PriorityDispatchQueue<int, std::greater<int>> minQ;
    for (int v : {5, 1, 9, 3}) minQ.push(v);
    CHECK(minQ.pop() == 1 && minQ.pop() == 3);

    using Log::Priority;
    using Inv::SupplyType;
    Log::DispatchBoard board;
    board.submit({1, "R", "critical blankets", SupplyType::ThermalBlanket, Priority::Critical, 70, 20, 1});
    board.submit({2, "R", "critical trauma", SupplyType::TraumaMedicine, Priority::Critical, 40, 5, 1});
    board.submit({3, "R", "urgent trauma severe", SupplyType::TraumaMedicine, Priority::Urgent, 95, 5, 1});
    board.submit({4, "R", "urgent water", SupplyType::CleanWater, Priority::Urgent, 99, 500, 1});
    board.submit({5, "R", "routine", SupplyType::RationKit, Priority::Routine, 100, 900, 1});
    board.submit({6, "R", "critical trauma, worse", SupplyType::TraumaMedicine, Priority::Critical, 80, 5, 1});
    board.submit({7, "Other", "elsewhere", SupplyType::RationKit, Priority::Routine, 1, 1, 1});
    CHECK(board.regions() == (std::vector<std::string>{"Other", "R"}));
    CHECK(board.pending("R") == 6 && board.pending("none") == 0);
    // Priority first, then medical-before-other, then severity.
    const std::vector<int> expected = {6, 2, 1, 3, 4, 5};
    for (int id : expected) CHECK(board.next("R").id == id);
    CHECK_THROWS(board.next("R"), EmptyQueueException);
    CHECK_THROWS(board.next("missing"), EmptyQueueException);

    // Strict weak ordering sanity for the comparator.
    Log::MedicalUrgencyComparator cmp;
    const Log::DispatchRequest x{1, "r", "", SupplyType::TraumaMedicine, Priority::Urgent, 50, 10, 1};
    CHECK(!cmp(x, x));
}

static void testManipulators() {
    std::cout << "I/O manipulators\n";
    std::ostringstream o;
    o << IO::col(6) << "ab" << '|' << IO::rcol(6) << "cd" << '|' << IO::kg(12.34) << '|'
      << IO::m3(0.5);
    CHECK(o.str() == "ab    |    cd|12.3 kg|0.500 m3");
    std::ostringstream rule;
    rule << IO::tableRule;
    CHECK(rule.str() == std::string(IO::kTableWidth, '-') + "\n");

    Log::TransportManifest m(1, "r");
    m.add(cargo(1, Inv::SupplyType::RationKit, 2, 3.0, 0.5));
    std::ostringstream table;
    table << std::fixed << std::setprecision(7);  // caller state must survive printing
    IO::printManifestTable(table, m);
    CHECK(table.str().find("TOTAL") != std::string::npos);
    CHECK(table.str().find("6.0 kg") != std::string::npos);
    CHECK(table.precision() == 7);
}

int main() {
    testCargo();
    testDepot();
    testAllocation();
    testManifest();
    testVehicles();
    testPersistence();
    testQueueAndFunctors();
    testManipulators();

    std::cout << "\n" << (g_checks - g_failed) << "/" << g_checks << " checks passed\n";
    return g_failed == 0 ? 0 : 1;
}
