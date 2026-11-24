# ERL-HUB: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub

A C++17 object-oriented simulation of a relief logistics hub. Supplies sit in a field depot,
get allocated onto manifests, and are dispatched on aerial, ground and amphibious vehicles
across damaged corridors. Written to cover every Unit I-IV requirement of the project brief.

## Build and run

Requires a C++17 compiler (g++ 9+ or clang 10+; tested with g++ 13).

```bash
make          # build/erl-hub
make run      # run the demo (one section per unit)
make test     # run the 121-check self-test suite
make clean
```

CMake alternative (also works on Windows with MSVC or MinGW):

```bash
cmake -S . -B build-cmake && cmake --build build-cmake
./build-cmake/erl-hub        # or erl-hub.exe
./build-cmake/erl-tests
```

The demo writes two small binary files in the working directory and deletes them at the end.

## Where each requirement lives

### Unit I - Core foundations and call semantics

| Requirement | Where |
|---|---|
| Namespaces `ReliefOps::Logistics`, `ReliefOps::Inventory` | every header; shared types in `Types.hpp` |
| Function overloading for allocation (volume vs priority medical load) | `Allocation.hpp/.cpp`: three `allocateSupplies()` overloads taking `double`, `(Priority, int)`, `(SupplyType, int)` |
| Call by reference / return by reference | `adjustStock(ReliefCargo&, int)`, `DistributionDepot::findStock()`, `ReliefCargo::quantityRef()`, `TransportManifest::at()`, and the `depot`/`manifest` parameters of `allocateSupplies` |
| Inline distance/payload utilities | `Types.hpp`: `travelTimeHours`, `payloadUtilisation`, `fuelRequiredLitres`, `approxDistanceKm`, `kgToTonnes`; inline `totalWeightKg()/totalVolumeM3()` |
| `friend` class `IncidentCommander` overriding access barriers | `IncidentCommander.hpp/.cpp`; it is a friend of `ReliefCargo` (private `priority_`) and `DistributionDepot` (protected reserve) |

### Unit II - Encapsulation, dynamic memory and operator overloading

| Requirement | Where |
|---|---|
| ADTs with information hiding: `ReliefCargo`, `DistributionDepot`, `TransportVehicle` | all data members private; public query interface only |
| Static variables and functions (dispatched tonnage, operational vehicles) | `TransportVehicle::operationalVehicles()`, `totalDispatchedTonnes()`, `recordDispatch()` |
| Dynamic allocation: copy constructors, dynamic constructors, clean destructors | `TransportManifest` (heap array, deep copy, copy-and-swap assignment, move, destructor) and `ReliefCargo` (heap-allocated label) |
| `+=` restock | `DistributionDepot::operator+=` |
| `-=` deduct on departure | `DistributionDepot::operator-=` (respects the protected reserve, throws `InsufficientStockException`) |
| `==` manifest identity and duplicate cargo | `TransportManifest::operator==`, `ReliefCargo::operator==` (same id + type), `TransportManifest::add()` merges duplicates |
| `<<` and `>>` manifest serialisation | `ReliefCargo` and `TransportManifest`; round-trips through any stream |

### Unit III - Inheritance, containership and polymorphism

```
                 TransportVehicle             (Unit II ADT)
                        |
                  ReliefVehicle               (abstract base; owns GPSBeacon + EmergencyLifeSupportUnit)
          /-------------+--------------\      (all inherit VIRTUALLY)
   AerialTransport  GroundConvoy    MarineVessel
                         \              /
                         AmphibiousCraft      (mobile medical unit: ONE shared ReliefVehicle)
```

- `ReliefVehicle` is derived into `AerialTransport`, `GroundConvoy` and `AmphibiousCraft`. To form
  the diamond, `AmphibiousCraft` inherits from `GroundConvoy` and an extra `MarineVessel` class,
  both of which use **virtual inheritance** of `ReliefVehicle`.
- **Containership:** `GPSBeacon` and `EmergencyLifeSupportUnit` are independent classes held as
  members inside `ReliefVehicle` (has-a, not deep inheritance).
- **Runtime polymorphism:** the demo holds the fleet as `ReliefVehicle*`, calls the virtual
  `evaluateRoute()`, then deletes through the base pointers. The destructor trace shows the
  correct chain, including `~AmphibiousCraft -> ~MarineVessel -> ~GroundConvoy -> ~ReliefVehicle`.
- Files: `Vehicles.hpp/.cpp`, `TransportVehicle.hpp/.cpp`.

### Unit IV - Streams, persistence, exception safety and STL

| Requirement | Where |
|---|---|
| Exception framework from `std::exception` | `Exceptions.hpp`: `ERLException` base with `CargoOverweightException`, `FuelDepletionException`, `DamagedCorridorException` (plus `InsufficientStockException`, `PersistenceException`, `EmptyQueueException`) |
| Binary save/restore of transit logs and manifest audits with `std::fstream` | `Persistence.hpp/.cpp`; writes to `<file>.tmp` then renames, so updates are atomic; truncated or corrupt files raise `PersistenceException` |
| Generic `PriorityDispatchQueue<T>` | `PriorityDispatchQueue.hpp` (`template <typename T, typename Compare>`) |
| STL containers | `std::vector` (queue heap, depot stock, manifests), `std::unordered_map` (`DispatchBoard` regions, depot par levels) |
| Custom functor objects | `MedicalUrgencyComparator` (urgent medical needs first) and `CargoPriorityDescending` in `Dispatch.hpp` |
| User-defined I/O manipulators for tabular output | `Manipulators.hpp`: `col(w)`, `rcol(w)`, `kg(x)`, `m3(x)`, `tableRule`, `manifestHeader` |

Exception guarantees: `ReliefVehicle::load()` and `dispatch()` either complete or leave the vehicle
unchanged (fuel, GPS, tonnage and loaded manifest are only touched after every check passes).

## Design notes

- **Reserve stock.** A depot protects a share of each line's par level. Normal `-=` and the
  allocation routines cannot touch it; only `IncidentCommander::commandeer()` can.
- **Vehicle behaviour.** Aircraft ignore ground damage but need open airspace. Convoys slow down and
  burn more fuel as damage rises and refuse floods or damage above their limit. The amphibious craft
  is slower but tolerates worse roads and can switch to water mode.
- **Binary files** use the host's native byte order; they are audit files for the same machine, not an
  interchange format.

## Layout

```
include/   headers (one per class group)
src/       implementations + main.cpp (demo)
tests/     test_erlhub.cpp (self-contained, no framework)
Makefile   CMakeLists.txt   README.md
```
