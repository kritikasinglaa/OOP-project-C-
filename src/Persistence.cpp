#include "Persistence.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <type_traits>

#include "Exceptions.hpp"

namespace ReliefOps {
namespace Logistics {

using Inventory::kSupplyTypeCount;
using Inventory::ReliefCargo;
using Inventory::SupplyType;

namespace {

namespace fs = std::filesystem;

constexpr std::uint32_t kLogMagic = 0x4C484C45;       // "ELHL"
constexpr std::uint32_t kManifestMagic = 0x4D484C45;  // "ELHM"
constexpr std::uint32_t kVersion = 1;
constexpr std::uint32_t kMaxStringBytes = 1u << 20;
constexpr std::uint64_t kMaxRecords = 10000000;

template <typename T>
void writePod(std::ostream& os, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "POD only");
    os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
T readPod(std::istream& is) {
    static_assert(std::is_trivially_copyable<T>::value, "POD only");
    T value{};
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!is) throw PersistenceException("unexpected end of file (truncated or corrupt)");
    return value;
}

void writeString(std::ostream& os, const std::string& text) {
    writePod<std::uint32_t>(os, static_cast<std::uint32_t>(text.size()));
    os.write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::string readString(std::istream& is) {
    const std::uint32_t length = readPod<std::uint32_t>(is);
    if (length > kMaxStringBytes) throw PersistenceException("implausible string length");
    std::string text(length, '\0');
    if (length > 0) {
        is.read(&text[0], static_cast<std::streamsize>(length));
        if (!is) throw PersistenceException("unexpected end of file inside a string");
    }
    return text;
}

// Writes via a temporary file and renames over the target: all-or-nothing.
void atomicWrite(const std::string& path, const std::function<void(std::ostream&)>& writer) {
    const std::string temp = path + ".tmp";
    try {
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            if (!out) throw PersistenceException("cannot open '" + temp + "' for writing");
            writer(out);
            out.flush();
            if (!out) throw PersistenceException("write failed for '" + temp + "'");
        }  // closed before the rename
        std::error_code ec;
        fs::rename(temp, path, ec);
        if (ec) throw PersistenceException("cannot replace '" + path + "': " + ec.message());
    } catch (...) {
        std::error_code ignored;
        fs::remove(temp, ignored);  // never leave the temp file behind
        throw;
    }
}

std::ifstream openForReading(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw PersistenceException("cannot open '" + path + "' for reading");
    return in;
}

void checkHeader(std::istream& is, std::uint32_t expectedMagic, const char* what) {
    const std::uint32_t magic = readPod<std::uint32_t>(is);
    if (magic != expectedMagic) throw PersistenceException(std::string("not a ") + what + " file");
    const std::uint32_t version = readPod<std::uint32_t>(is);
    if (version != kVersion) throw PersistenceException("unsupported file version");
}

}  // namespace

// ================================ TransitLog =================================

void TransitLog::record(int manifestId, int vehicleId, double weightKg, double distanceKm,
                        double etaHours, const std::string& status) {
    TransitLogEntry entry{};
    entry.manifestId = manifestId;
    entry.vehicleId = vehicleId;
    entry.weightKg = weightKg;
    entry.distanceKm = distanceKm;
    entry.etaHours = etaHours;
    const std::size_t n = std::min(status.size(), sizeof(entry.status) - 1);
    std::memcpy(entry.status, status.data(), n);
    entries_.push_back(entry);
}

void TransitLog::save(const std::string& path) const {
    atomicWrite(path, [this](std::ostream& os) {
        writePod<std::uint32_t>(os, kLogMagic);
        writePod<std::uint32_t>(os, kVersion);
        writePod<std::uint64_t>(os, entries_.size());
        for (const TransitLogEntry& e : entries_) {
            writePod(os, e.manifestId);
            writePod(os, e.vehicleId);
            writePod(os, e.weightKg);
            writePod(os, e.distanceKm);
            writePod(os, e.etaHours);
            os.write(e.status, sizeof(e.status));
        }
    });
}

TransitLog TransitLog::load(const std::string& path) {
    std::ifstream in = openForReading(path);
    checkHeader(in, kLogMagic, "transit log");
    const std::uint64_t count = readPod<std::uint64_t>(in);
    if (count > kMaxRecords) throw PersistenceException("implausible record count");

    TransitLog log;
    for (std::uint64_t i = 0; i < count; ++i) {
        TransitLogEntry e{};
        e.manifestId = readPod<std::int32_t>(in);
        e.vehicleId = readPod<std::int32_t>(in);
        e.weightKg = readPod<double>(in);
        e.distanceKm = readPod<double>(in);
        e.etaHours = readPod<double>(in);
        in.read(e.status, sizeof(e.status));
        if (!in) throw PersistenceException("unexpected end of file (truncated or corrupt)");
        e.status[sizeof(e.status) - 1] = '\0';
        log.entries_.push_back(e);
    }
    return log;
}

// ============================== Manifest audit ===============================

void saveManifestAudit(const std::string& path, const std::vector<TransportManifest>& manifests) {
    atomicWrite(path, [&manifests](std::ostream& os) {
        writePod<std::uint32_t>(os, kManifestMagic);
        writePod<std::uint32_t>(os, kVersion);
        writePod<std::uint64_t>(os, manifests.size());
        for (const TransportManifest& m : manifests) {
            writePod<std::int32_t>(os, m.id());
            writeString(os, m.route());
            writePod<std::uint32_t>(os, static_cast<std::uint32_t>(m.size()));
            for (const ReliefCargo& c : m) {
                writePod<std::int32_t>(os, c.id());
                writeString(os, c.label());
                writePod<std::uint8_t>(os, static_cast<std::uint8_t>(c.type()));
                writePod<std::int32_t>(os, c.quantity());
                writePod<double>(os, c.unitWeightKg());
                writePod<double>(os, c.unitVolumeM3());
                writePod<std::uint8_t>(os, static_cast<std::uint8_t>(c.priority()));
            }
        }
    });
}

std::vector<TransportManifest> loadManifestAudit(const std::string& path) {
    std::ifstream in = openForReading(path);
    checkHeader(in, kManifestMagic, "manifest audit");
    const std::uint64_t count = readPod<std::uint64_t>(in);
    if (count > kMaxRecords) throw PersistenceException("implausible manifest count");

    std::vector<TransportManifest> manifests;
    for (std::uint64_t i = 0; i < count; ++i) {
        const int id = readPod<std::int32_t>(in);
        const std::string route = readString(in);
        const std::uint32_t lines = readPod<std::uint32_t>(in);
        if (lines > kMaxRecords) throw PersistenceException("implausible line count");

        TransportManifest manifest(id, route, lines ? lines : 1);
        for (std::uint32_t j = 0; j < lines; ++j) {
            const int cargoId = readPod<std::int32_t>(in);
            const std::string label = readString(in);
            const int type = readPod<std::uint8_t>(in);
            const int quantity = readPod<std::int32_t>(in);
            const double weight = readPod<double>(in);
            const double volume = readPod<double>(in);
            const int priority = readPod<std::uint8_t>(in);
            if (type >= kSupplyTypeCount || priority > 2) {
                throw PersistenceException("invalid enumeration value in manifest audit");
            }
            manifest.add(ReliefCargo(cargoId, label.c_str(), static_cast<SupplyType>(type),
                                     quantity, weight, volume,
                                     static_cast<Priority>(priority)));
        }
        manifests.push_back(std::move(manifest));
    }
    return manifests;
}

}  // namespace Logistics
}  // namespace ReliefOps
