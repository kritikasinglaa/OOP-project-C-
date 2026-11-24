# ERL-HUB

Post-Earthquake Multi-Agency Emergency Relief Logistics Hub built with C++17. The project models how relief supplies are stored, allocated, loaded onto different types of transport, routed across damaged terrain, prioritized for urgent requests, and persisted across runs.

## Overview

When a disaster strikes, coordinating supplies and transport quickly becomes a logistics problem: medical packs, clean water, rations, and blankets have different weights, volumes, and urgency levels, while roads, rivers, and airspace may be partially damaged or blocked.

**ERL-HUB** simulates this workflow in C++17. It combines inventory management, manifest building, vehicle route evaluation, dispatch prioritization, error handling, and file persistence into a single codebase with multiple ways to build and run it.

## Key Capabilities

### Supply Depot and Manifests

* **Inventory Tracking:** Stores relief items (`TraumaMedicine`, `CleanWater`, `RationKit`, `ThermalBlanket`) with quantities, unit weights, volumes, priority levels, and protected reserve ratios so critical stock is not casually drained.
* **Supply Allocation:** Supports allocating supplies into transport manifests by priority and line limit, by available volume, or by targeted supply type and quantity (merging matching lines when appropriate).
* **Dynamic Manifests & Formatting:** Uses dynamically allocated manifest storage with proper copy/move/cleanup behavior, operator overloads for restocking, deducting, comparing, and streaming manifests, plus priority sorting and formatted table output for manifests and depots.

### Vehicles, Routes, and Dispatch

* **Multi-Mode Fleet:** Models transport options including `AerialTransport`, `GroundConvoy`, `MarineVessel`, and `AmphibiousCraft`, each with capacity, fuel, embedded telemetry/life-support components, and mode-specific behavior.
* **Route Evaluation:** Evaluates routes using distance, corridor damage, flooding, and airspace status—calculating estimated travel time and fuel consumption or flagging blocked corridors when conditions are unsafe.
* **Priority Dispatch & Transit Logs:** Groups incoming requests by region and serves higher-priority/more severe needs first, while recording completed dispatches in a transit log.

### Error Handling and Persistence

* **Custom Exceptions:** Uses dedicated exception types (such as overweight cargo, fuel shortfall, damaged/blocked corridors, insufficient stock, empty queues, and persistence errors) to report problems cleanly without corrupting state.
* **Binary Save & Load:** Persists transit logs and manifest snapshots to binary files and validates them on load so truncated or corrupted files are caught safely.

## Folder Structure

```text
erl-hub/
├── include/          # Class declarations, templates, and interfaces (.hpp)
├── src/              # Implementation files (.cpp) and entry point (main.cpp)
├── tests/            # Test suite (test_erlhub.cpp)
├── CMakeLists.txt    # CMake configuration for building the library, app, and tests
└── Makefile          # Make targets for building, running, testing, and cleaning
```

## Prerequisites

* A C++17-capable compiler (such as `g++`, `clang++`, or MSVC).
* Optional build tools if you prefer them over direct compiler commands: `make` or `cmake` (and `ctest`).

## Building and Running

### 1. Direct compilation with `g++`

Run from the root `erl-hub` directory:

```bash
g++ -std=c++17 -Iinclude src/*.cpp -o main.exe
./main.exe
```

On Windows Command Prompt, run `main.exe` directly:

```cmd
g++ -std=c++17 -Iinclude src\*.cpp -o main.exe && main.exe
```

### 2. Building with `make`

```bash
make        # compile object files and build the main executable
make run    # build (if needed) and run the program
make test   # compile and run tests from tests/test_erlhub.cpp
make clean  # remove generated build artifacts
```

### 3. Building with CMake

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Notes for GitHub / Development

* Avoid committing compiled binaries (such as `main.exe` or `build/` directories) or temporary binary output files unless intentionally needed.
* Update `src/main.cpp` if you want to customize the demo scenario or add new sample routes, stock items, or dispatch requests.