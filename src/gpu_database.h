#pragma once

#include "gpu_specs.h"
#include <string>
#include <vector>

class GpuDatabase {
public:
    GpuDatabase();
    ~GpuDatabase();

    bool open(const std::string& db_path);
    void close();

    GpuSpecs lookup_by_pci_id(const std::string& pci_device_id);
    GpuSpecs lookup_by_name(const std::string& gpu_name);
    std::vector<GpuSpecs> all_gpus();

    bool is_open() const { return db_ != nullptr; }

private:
    GpuSpecs row_to_specs(void* stmt);
    void* db_ = nullptr;
};
