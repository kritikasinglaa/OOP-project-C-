// ERL-HUB: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
// Full combined demo driver without unit/section classification headers.

#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
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
#include "PriorityDispatchQueue.hpp"
#include "ReliefCargo.hpp"
#include "TransportManifest.hpp"
#include "Vehicles.hpp"

namespace Log = ReliefOps::Logistics;
namespace Inv = ReliefOps::Inventory;
namespace IO = ReliefOps::IO;
using ReliefOps::ERLException;

namespace {

std::string pad(const std::string& text, std::size_t width) {
    return text.size() >= width ? text : text + std::string(width - text.size(), ' ');
}

void runFullDemo(Inv::DistributionDepot& depot, Log::TransportManifest& medical,
                 Log::TransportManifest& water, Log::TransportManifest& shelter) {
    std::cout << "Inventory side : ReliefOps::Inventory::DistributionDepot '" << depot.name()
              << "'\n"
              << "Logistics side : ReliefOps::Logistics::TransportManifest #" << medical.id()
              << "\n";

    // Initial stock. Insertion order matters for the volume-based overload below.
    depot += Inv::ReliefCargo(102, "Trauma Medicine Pack", Inv::SupplyType::TraumaMedicine, 150,
                              2.8, 0.006, Log::Priority::Critical);
    depot += Inv::ReliefCargo(103, "Clean Water 20L", Inv::SupplyType::CleanWater, 400, 20.5,
                              0.021, Log::Priority::Urgent);
    depot += Inv::ReliefCargo(101, "Ration Kit (3-day)", Inv::SupplyType::RationKit, 800, 4.5,
                              0.012, Log::Priority::Routine);
    depot += Inv::ReliefCargo(104, "Thermal Blanket Bundle", Inv::SupplyType::ThermalBlanket,
                              500, 0.9, 0.004, Log::Priority::Routine);
    IO::printDepotTable(std::cout, depot);

    const int a = Log::allocateSupplies(depot, medical, Log::Priority::Critical, 1);
    std::cout << "(Priority, maxLines)  -> emergency medical load : " << a << " units\n";
    const int b = Log::allocateSupplies(depot, water, 2.1);
    std::cout << "(double volume m3)    -> volume-based payload   : " << b << " units\n";
    const int c1 = Log::allocateSupplies(depot, shelter, Inv::SupplyType::ThermalBlanket, 100);
    const int c2 = Log::allocateSupplies(depot, shelter, Inv::SupplyType::ThermalBlanket, 100);
    const int c3 = Log::allocateSupplies(depot, shelter, Inv::SupplyType::RationKit, 300);
    std::cout << "(SupplyType, qty)     -> targeted request       : " << c1 << " + " << c2
              << " blankets, " << c3 << " rations\n";
    std::cout << "  (2nd blanket call merged into the existing line: shelter manifest has "
              << shelter.size() << " lines)\n";
    IO::printDepotTable(std::cout, depot);

    Inv::ReliefCargo& waterLine = depot.findStock(103);  // returns a reference into the depot
    std::cout << "findStock(103) returned a reference; water qty = " << waterLine.quantity()
              << "\n";
    Inv::adjustStock(waterLine, +25);  // call by reference: updates the depot's own line
    std::cout << "adjustStock(+25) -> depot now holds " << depot.findStock(103).quantity()
              << " water\n";
    int& slot = water.at(0).quantityRef();  // reference into the manifest's heap array
    slot -= 10;
    std::cout << "int& slot = manifest.at(0).quantityRef(); slot -= 10 -> manifest line is now "
              << water.at(0).quantity() << "\n";

    const double km = Log::approxDistanceKm(26.1445, 91.7362, 25.5788, 91.8933);
    std::cout << std::fixed << std::setprecision(1)
              << "approxDistanceKm(Guwahati, Shillong)   = " << km << " km\n"
              << "travelTimeHours(120 km @ 45 km/h)      = " << Log::travelTimeHours(120, 45)
              << " h\n"
              << "payloadUtilisation(1530 kg of 3500 kg) = "
              << Log::payloadUtilisation(1530, 3500) * 100 << " %\n"
              << "kgToTonnes(2050)                       = " << std::setprecision(2)
              << Log::kgToTonnes(2050) << " t" << std::setprecision(1) << "\n";

    Log::IncidentCommander commander("NDRF-Command");
    std::cout << commander.audit(depot);
    try {
        depot -= depot.findStock(102).slice(5);  // normal path: only reserve stock is left
    } catch (const ReliefOps::InsufficientStockException& e) {
        std::cout << "Normal deduction refused : " << e.what() << "\n";
    }
    const Inv::ReliefCargo taken = commander.commandeer(depot, 102, 12);  // friend access
    std::cout << "Commander commandeered   : " << taken.quantity() << " x " << taken.label()
              << " (now " << Log::toString(taken.priority()) << ")\n";
    Inv::ReliefCargo& blankets = depot.findStock(104);
    std::cout << "Blankets priority before : " << Log::toString(blankets.priority()) << "\n";
    commander.escalate(blankets, Log::Priority::Critical);  // priority_ is private
    std::cout << "Blankets priority after  : " << Log::toString(blankets.priority())
              << "   (overrides issued: " << commander.overridesIssued() << ")\n";

    std::cout << "operational vehicles now : " << Log::TransportVehicle::operationalVehicles()
              << "\ntotal dispatched tonnes  : " << Log::TransportVehicle::totalDispatchedTonnes()
              << "\n";

    auto* growing = new Log::TransportManifest(2001, "Dynamic demo route", 2);
    std::cout << "heap manifest created with capacity " << growing->capacity() << "\n";
    for (int i = 1; i <= 5; ++i) {
        growing->add(Inv::ReliefCargo(900 + i, "Demo item", Inv::SupplyType::RationKit, i, 1.0,
                                      0.01));
    }
    std::cout << "after adding 5 lines: size " << growing->size() << ", capacity "
              << growing->capacity() << " (array regrown on the heap)\n";
    Log::TransportManifest copy(*growing);  // deep copy
    growing->at(0).quantityRef() = 999;
    delete growing;  // destructor releases the array; the copy must be unaffected
    std::cout << "original deleted; deep copy still intact: line 0 qty = " << copy.at(0).quantity()
              << ", size = " << copy.size() << "\n";

    Log::TransportManifest water2(water);  // copy constructor on a real manifest
    std::cout << "copy of water manifest uses different storage: "
              << (&water2.at(0) != &water.at(0) ? "yes" : "NO (bug)") << "\n";

    std::cout << "water == copy            : " << (water == water2 ? "true" : "false") << "\n";
    water2.at(0).quantityRef() += 1;
    std::cout << "after changing one qty   : " << (water == water2 ? "true" : "false") << "\n";
    const Inv::ReliefCargo duplicate = water.at(0).slice(1);
    std::cout << "water.contains(same id/type line) : "
              << (water.contains(duplicate) ? "yes (duplicate)" : "no") << "\n";
    const bool isNew = water.add(duplicate);
    std::cout << "add() of that duplicate  : " << (isNew ? "new line" : "merged into existing line")
              << ", qty now " << water.at(0).quantity() << "\n";

    std::cout << "water stock before       : " << depot.findStock(103).quantity() << "\n";
    depot += Inv::ReliefCargo(103, "Clean Water 20L", Inv::SupplyType::CleanWater, 60, 20.5,
                              0.021, Log::Priority::Urgent);
    std::cout << "after depot += 60 water  : " << depot.findStock(103).quantity() << "\n";
    depot -= Inv::ReliefCargo(103, "Clean Water 20L", Inv::SupplyType::CleanWater, 40, 20.5,
                              0.021, Log::Priority::Urgent);
    std::cout << "after depot -= 40 water  : " << depot.findStock(103).quantity() << "\n";

    std::stringstream wire;
    wire << shelter;
    std::cout << wire.str();
    Log::TransportManifest restored;
    wire >> restored;
    std::cout << "read back from the stream; restored == original : "
              << (restored == shelter ? "true" : "false") << "\n";

    std::vector<Log::ReliefVehicle*> fleet;
    fleet.push_back(new Log::AerialTransport({1, "Heli-1", 1200.0, 6.0, 900.0}));
    fleet.push_back(new Log::GroundConvoy({2, "Truck-7", 9000.0, 40.0, 600.0}));
    fleet.push_back(new Log::AmphibiousCraft({3, "AmphiMed-3", 3500.0, 18.0, 500.0}, 6));

    for (const Log::ReliefVehicle* v : fleet) std::cout << "  " << v->summary() << "\n";
    std::cout << "operational vehicles (static counter): "
              << Log::TransportVehicle::operationalVehicles() << "\n";

    // Downcasting from a virtual base requires dynamic_cast (static_cast is ill-formed).
    auto* amphibious = dynamic_cast<Log::AmphibiousCraft*>(fleet[2]);
    const Log::ReliefVehicle* viaGround = static_cast<Log::GroundConvoy*>(amphibious);
    const Log::ReliefVehicle* viaMarine = static_cast<Log::MarineVessel*>(amphibious);
    std::cout << "ReliefVehicle seen through GroundConvoy : " << static_cast<const void*>(viaGround)
              << "\nReliefVehicle seen through MarineVessel : " << static_cast<const void*>(viaMarine)
              << "\nsame subobject: " << (viaGround == viaMarine ? "yes" : "NO (bug)")
              << "; amphibious->name() is unambiguous: " << amphibious->name()
              << " with " << amphibious->triageBeds() << " triage beds\n";

    for (const Log::ReliefVehicle* v : fleet) {
        std::cout << "  " << pad(v->name(), 11) << v->gps().describe() << " | "
                  << v->lifeSupport().describe() << "\n";
    }

    const std::vector<Log::Route> routes = {
        {"NH-37 Highway", 120.0, 0.30, false, true},
        {"Brahmaputra Flood Plain", 85.0, 0.45, true, true},
        {"Collapsed Hill Road", 60.0, 0.88, false, true},
        {"Storm-Closed Valley", 45.0, 0.50, false, false},
    };
    for (const Log::Route& r : routes) {
        std::cout << "\n" << r.name << "  (" << r.distanceKm << " km, damage "
                  << std::setprecision(2) << r.corridorDamage << std::setprecision(1)
                  << (r.flooded ? ", flooded" : "") << (r.airspaceClear ? "" : ", airspace CLOSED")
                  << ")\n";
        for (const Log::ReliefVehicle* v : fleet) {
            try {
                const Log::RouteEvaluation ev = v->evaluateRoute(r);  // virtual dispatch
                std::cout << "   " << pad(v->name(), 11) << pad(ev.mode, 24)
                          << "ETA " << std::setw(5) << ev.etaHours << " h   fuel " << std::setw(6)
                          << ev.fuelLitres << " L\n";
            } catch (const ReliefOps::DamagedCorridorException& e) {
                std::cout << "   " << pad(v->name(), 11) << "BLOCKED - " << e.what() << "\n";
            }
        }
    }

    Log::ReliefVehicle::setDestructorTrace(true);
    for (Log::ReliefVehicle* v : fleet) {
        std::cout << "  delete " << v->name() << ":\n";
        delete v;  // correct chain only because the destructors are virtual
    }
    Log::ReliefVehicle::setDestructorTrace(false);
    std::cout << "operational vehicles after teardown: "
              << Log::TransportVehicle::operationalVehicles() << "\n";

    Log::AerialTransport heli({11, "Heli-1", 1200.0, 6.0, 900.0});
    Log::GroundConvoy truck({12, "Truck-7", 9000.0, 40.0, 600.0});
    Log::AmphibiousCraft amphi({13, "AmphiMed-3", 3500.0, 18.0, 500.0}, 6);
    Log::AerialTransport lowFuelHeli({14, "Heli-2", 1200.0, 6.0, 40.0});

    try {
        heli.load(water);
    } catch (const ReliefOps::CargoOverweightException& e) {
        std::cout << "CargoOverweightException : " << e.what() << "\n"
                  << "  excess = " << e.excess() << " kg; heli still empty: "
                  << (heli.manifest().size() == 0 ? "yes" : "NO") << " (strong guarantee)\n";
    }
    try {
        lowFuelHeli.dispatch({"NH-37 Highway", 120.0, 0.30, false, true});
    } catch (const ReliefOps::FuelDepletionException& e) {
        std::cout << "FuelDepletionException   : " << e.what() << "\n"
                  << "  shortfall = " << e.shortfallLitres() << " L; fuel untouched: "
                  << lowFuelHeli.fuelLitres() << " L\n";
    }
    const Log::Route brokenRoad{"Collapsed Hill Road", 60.0, 0.88, false, true};
    try {
        truck.dispatch(brokenRoad);
    } catch (const ReliefOps::DamagedCorridorException& e) {
        std::cout << "DamagedCorridorException : " << e.what() << "\n";
        const Log::RouteEvaluation ev = amphi.evaluateRoute(brokenRoad);
        std::cout << "  AmphiMed-3 can take it  : " << ev.mode << ", ETA " << ev.etaHours << " h\n";
    }
    try {
        throw ReliefOps::EmptyQueueException();
    } catch (const std::exception& e) {  // catches the whole family via the std base
        std::cout << "caught as std::exception : " << e.what() << "\n";
    }

    Log::TransitLog transitLog;
    const Log::Route highway{"NH-37 Highway", 120.0, 0.30, false, true};
    const Log::Route floodPlain{"Brahmaputra Flood Plain", 85.0, 0.45, true, true};

    struct Leg {
        Log::ReliefVehicle* vehicle;
        const Log::TransportManifest* manifest;
        const Log::Route* route;
    };
    const std::vector<Leg> legs = {{&heli, &medical, &highway},
                                   {&truck, &water, &highway},
                                   {&amphi, &shelter, &floodPlain}};
    for (const Leg& leg : legs) {
        leg.vehicle->load(*leg.manifest);
        const double kgOnBoard = leg.vehicle->loadedKg();
        const Log::RouteEvaluation ev = leg.vehicle->dispatch(*leg.route);
        transitLog.record(leg.manifest->id(), leg.vehicle->id(), kgOnBoard, leg.route->distanceKm,
                          ev.etaHours, "DELIVERED");
        std::cout << "  " << pad(leg.vehicle->name(), 11) << kgOnBoard << " kg over "
                  << leg.route->distanceKm << " km via " << ev.mode << ", ETA " << ev.etaHours
                  << " h, fuel left " << leg.vehicle->fuelLitres() << " L\n";
        leg.vehicle->unload();
    }
    std::cout << "  total dispatched (static analytics): "
              << std::setprecision(2) << Log::TransportVehicle::totalDispatchedTonnes()
              << " tonnes by " << std::setprecision(1)
              << Log::TransportVehicle::operationalVehicles() << " operational vehicles\n";

    const std::string logPath = "erl_transit.log.bin";
    const std::string auditPath = "erl_manifest_audit.bin";
    transitLog.save(logPath);
    Log::saveManifestAudit(auditPath, {medical, water, shelter});
    const Log::TransitLog reloaded = Log::TransitLog::load(logPath);
    const std::vector<Log::TransportManifest> audit = Log::loadManifestAudit(auditPath);
    std::cout << "transit log restored : " << reloaded.size() << " entries\n";
    for (const Log::TransitLogEntry& e : reloaded.entries()) {
        std::cout << "  manifest " << e.manifestId << " / vehicle " << e.vehicleId << ": "
                  << e.weightKg << " kg, " << e.distanceKm << " km, " << e.status << "\n";
    }
    const bool auditMatches = audit.size() == 3 && audit[0] == medical && audit[1] == water &&
                              audit[2] == shelter;
    std::cout << "manifest audit restored: " << audit.size()
              << " manifests, identical to originals: " << (auditMatches ? "true" : "false")
              << "\n";

    // Corrupt a copy of the file by truncating it: loading must fail cleanly, not crash.
    const std::string brokenPath = "erl_transit.truncated.bin";
    {
        std::ifstream in(logPath, std::ios::binary);
        std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::ofstream out(brokenPath, std::ios::binary);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size() / 2));
    }
    try {
        Log::TransitLog::load(brokenPath);
    } catch (const ReliefOps::PersistenceException& e) {
        std::cout << "truncated file rejected: " << e.what() << "\n";
    }
    std::remove(brokenPath.c_str());

    Log::DispatchBoard board;
    using Log::Priority;
    using Inv::SupplyType;
    board.submit({1, "Riverside", "Crush injuries - field hospital", SupplyType::TraumaMedicine,
                  Priority::Critical, 92, 35, 40});
    board.submit({2, "Riverside", "Drinking water shortage", SupplyType::CleanWater,
                  Priority::Urgent, 55, 120, 80});
    board.submit({3, "Riverside", "Hypothermia cases", SupplyType::ThermalBlanket,
                  Priority::Critical, 70, 20, 60});
    board.submit({4, "Hillside", "Burn victims", SupplyType::TraumaMedicine, Priority::Urgent, 80,
                  50, 25});
    board.submit({5, "Hillside", "Ration shortfall", SupplyType::RationKit, Priority::Routine, 20,
                  300, 200});
    board.submit({6, "Hillside", "Insulin & antibiotics", SupplyType::TraumaMedicine,
                  Priority::Urgent, 88, 15, 30});
    for (const std::string& region : board.regions()) {
        std::cout << "  region " << region << " (" << board.pending(region) << " pending), "
                  << "served in this order:\n";
        int rank = 1;
        while (board.pending(region) > 0) {
            const Log::DispatchRequest r = board.next(region);
            std::cout << "    " << rank++ << ". #" << r.id << " " << pad(r.description, 34)
                      << pad(Log::toString(r.priority), 9) << pad(Inv::toString(r.need), 15)
                      << "severity " << r.severity << "\n";
        }
    }
    Log::PriorityDispatchQueue<int, std::greater<int>> etaMinutes;  // generic: a min-queue of ints
    for (int minutes : {45, 12, 30, 90, 7}) etaMinutes.push(minutes);
    std::cout << "  generic PriorityDispatchQueue<int, greater<int>> pops soonest ETA first:";
    while (!etaMinutes.empty()) std::cout << ' ' << etaMinutes.pop();
    std::cout << " (minutes)\n";

    Log::TransportManifest consolidated(3001, "Consolidated relief convoy", 4);
    consolidated.add(Inv::ReliefCargo(101, "Ration Kit (3-day)", SupplyType::RationKit, 120, 4.5,
                                      0.012, Priority::Routine));
    consolidated.add(Inv::ReliefCargo(103, "Clean Water 20L", SupplyType::CleanWater, 60, 20.5,
                                      0.021, Priority::Urgent));
    consolidated.add(Inv::ReliefCargo(104, "Thermal Blanket Bundle", SupplyType::ThermalBlanket,
                                      90, 0.9, 0.004, Priority::Routine));
    consolidated.add(Inv::ReliefCargo(102, "Trauma Medicine Pack", SupplyType::TraumaMedicine, 40,
                                      2.8, 0.006, Priority::Routine));
    commander.escalate(consolidated.at(3), Priority::Critical);
    commander.escalate(consolidated.at(2), Priority::Critical);
    std::cout << "(before sortByPriority)\n";
    IO::printManifestTable(std::cout, consolidated);
    consolidated.sortByPriority();  // CargoPriorityDescending functor under std::sort
    std::cout << "\n(after sortByPriority: Critical first)\n";
    IO::printManifestTable(std::cout, consolidated);

    std::cout << "\nFinal depot state:\n";
    IO::printDepotTable(std::cout, depot);

    std::remove(logPath.c_str());
    std::remove(auditPath.c_str());
}

}  // namespace

int main() {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "ERL-HUB: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub\n";

    try {
        Inv::DistributionDepot depot("Alpha Field Depot", 0.10);
        Log::TransportManifest medical(1001, "Alpha Depot -> Field Hospital");
        Log::TransportManifest water(1002, "Alpha Depot -> Riverside Camp");
        Log::TransportManifest shelter(1003, "Alpha Depot -> Hillside Shelter");

        runFullDemo(depot, medical, water, shelter);
    } catch (const ERLException& e) {
        std::cerr << "\nUnhandled ERL-HUB error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\nUnexpected error: " << e.what() << "\n";
        return 2;
    }

    std::cout << "\nAll demos finished.\n";
    return 0;
}