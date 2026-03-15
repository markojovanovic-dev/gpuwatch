#include "gpu_database.h"
#include <sqlite3.h>
#include <cstdio>
#include <algorithm>
#include <cctype>

GpuDatabase::GpuDatabase() = default;

GpuDatabase::~GpuDatabase() {
    close();
}

bool GpuDatabase::open(const std::string& db_path) {
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(db_path.c_str(), &db,
                             SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open GPU database '%s': %s\n",
                db_path.c_str(), sqlite3_errmsg(db));
        if (db) sqlite3_close(db);
        return false;
    }
    db_ = db;
    return true;
}

void GpuDatabase::close() {
    if (db_) {
        sqlite3_close(static_cast<sqlite3*>(db_));
        db_ = nullptr;
    }
}

GpuSpecs GpuDatabase::row_to_specs(void* stmt_ptr) {
    auto* stmt = static_cast<sqlite3_stmt*>(stmt_ptr);
    GpuSpecs s;

    auto text_col = [&](int col) -> std::string {
        const unsigned char* t = sqlite3_column_text(stmt, col);
        return t ? reinterpret_cast<const char*>(t) : "";
    };

    s.name                = text_col(0);
    s.pci_device_id       = text_col(1);
    s.architecture        = text_col(2);
    s.process             = text_col(3);
    s.memory_type         = text_col(4);
    s.compute_capability  = text_col(5);
    s.cuda_cores          = sqlite3_column_int(stmt, 6);
    s.tmus                = sqlite3_column_int(stmt, 7);
    s.rops                = sqlite3_column_int(stmt, 8);
    s.sms                 = sqlite3_column_int(stmt, 9);
    s.base_clock_mhz      = sqlite3_column_int(stmt, 10);
    s.boost_clock_mhz     = sqlite3_column_int(stmt, 11);
    s.memory_size_mb      = sqlite3_column_int(stmt, 12);
    s.memory_bus_width     = sqlite3_column_int(stmt, 13);
    s.tdp_watts           = sqlite3_column_int(stmt, 14);
    s.memory_bandwidth_gbs = static_cast<float>(sqlite3_column_double(stmt, 15));
    s.l2_cache_mb          = static_cast<float>(sqlite3_column_double(stmt, 16));
    s.transistors_billion  = static_cast<float>(sqlite3_column_double(stmt, 17));
    s.die_size_mm2         = static_cast<float>(sqlite3_column_double(stmt, 18));
    s.rt_cores             = sqlite3_column_int(stmt, 19);
    s.tensor_cores         = sqlite3_column_int(stmt, 20);
    s.found = true;

    return s;
}

static const char* SELECT_COLS =
    "SELECT name, pci_device_id, architecture, process, memory_type, "
    "compute_capability, cuda_cores, tmus, rops, sms, "
    "base_clock_mhz, boost_clock_mhz, memory_size_mb, memory_bus_width, "
    "tdp_watts, memory_bandwidth_gbs, l2_cache_mb, transistors_billion, "
    "die_size_mm2, rt_cores, tensor_cores FROM gpu_specs";

GpuSpecs GpuDatabase::lookup_by_pci_id(const std::string& pci_device_id) {
    if (!db_) return {};

    // Normalize to uppercase
    std::string id = pci_device_id;
    std::transform(id.begin(), id.end(), id.begin(), ::toupper);

    std::string sql = std::string(SELECT_COLS) +
        " WHERE UPPER(pci_device_id) = ?";

    sqlite3_stmt* stmt = nullptr;
    auto* db = static_cast<sqlite3*>(db_);
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return {};

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    GpuSpecs result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = row_to_specs(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

GpuSpecs GpuDatabase::lookup_by_name(const std::string& gpu_name) {
    if (!db_) return {};

    // Try exact match first, then substring
    std::string sql = std::string(SELECT_COLS) +
        " WHERE LOWER(name) = LOWER(?)";

    auto* db = static_cast<sqlite3*>(db_);
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return {};

    sqlite3_bind_text(stmt, 1, gpu_name.c_str(), -1, SQLITE_TRANSIENT);

    GpuSpecs result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = row_to_specs(stmt);
        sqlite3_finalize(stmt);
        return result;
    }
    sqlite3_finalize(stmt);

    // Fallback: substring match (find the best match)
    // Extract key model identifier from the NVML name
    // e.g. "NVIDIA GeForce RTX 5080" -> try matching "RTX 5080"
    sql = std::string(SELECT_COLS) +
        " WHERE LOWER(name) LIKE '%' || LOWER(?) || '%'"
        " ORDER BY LENGTH(name) ASC LIMIT 1";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return {};

    // Extract the model part (after "GeForce " or just use the whole name)
    std::string search = gpu_name;
    auto pos = search.find("RTX");
    if (pos != std::string::npos)
        search = search.substr(pos);

    sqlite3_bind_text(stmt, 1, search.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = row_to_specs(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<GpuSpecs> GpuDatabase::all_gpus() {
    std::vector<GpuSpecs> results;
    if (!db_) return results;

    std::string sql = std::string(SELECT_COLS) + " ORDER BY name";
    auto* db = static_cast<sqlite3*>(db_);
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return results;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(row_to_specs(stmt));
    }
    sqlite3_finalize(stmt);
    return results;
}
