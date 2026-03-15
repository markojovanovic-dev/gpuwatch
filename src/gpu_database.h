#pragma once

#include "gpu_specs.h"
#include <string>
#include <vector>

class GpuDatabase {
public:
    GpuDatabase();
    ~GpuDatabase();

    // Open the SQLite database at the given path. Returns false on failure.
    bool open(const std::string& db_path);

    // Close the database.
    void close();

    // Look up GPU specs by PCI device ID (hex string, e.g. "2B00").
    // Returns a GpuSpecs with found=true if matched.
    GpuSpecs lookup_by_pci_id(const std::string& pci_device_id);

    // Look up GPU specs by name substring match (case-insensitive).
    // Useful as a fallback when PCI ID isn't in the database.
    GpuSpecs lookup_by_name(const std::string& gpu_name);

    // Get all GPU entries in the database.
    std::vector<GpuSpecs> all_gpus();

    bool is_open() const { return db_ != nullptr; }

private:
    GpuSpecs row_to_specs(void* stmt);
    void* db_ = nullptr; // sqlite3*
};
