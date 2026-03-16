#pragma once

#include <string>
#include <cstdint>

struct GpuSpecs {
    std::string name;
    std::string pci_device_id;
    std::string architecture;
    std::string process;
    std::string memory_type;
    std::string compute_capability;

    int cuda_cores      = 0;
    int tmus            = 0;
    int rops            = 0;
    int sms             = 0;
    int base_clock_mhz  = 0;
    int boost_clock_mhz = 0;
    int memory_size_mb  = 0;
    int memory_bus_width = 0;
    int tdp_watts       = 0;

    float memory_bandwidth_gbs = 0.0f;
    float l2_cache_mb          = 0.0f;
    float transistors_billion  = 0.0f;
    float die_size_mm2         = 0.0f;

    int rt_cores     = 0;
    int tensor_cores = 0;

    bool found = false;
};

struct GpuLiveStats {
    unsigned int gpu_utilization    = 0;
    unsigned int mem_utilization    = 0;

    unsigned long long mem_used     = 0;
    unsigned long long mem_free     = 0;
    unsigned long long mem_total    = 0;

    unsigned int temperature        = 0;
    unsigned int temperature_max    = 0;
    unsigned int temp_hotspot       = 0;
    unsigned int temp_mem_junction  = 0;
    unsigned int fan_speed          = 0;
    unsigned int power_draw_mw      = 0;
    unsigned int power_limit_mw     = 0;

    unsigned int clock_gpu_mhz     = 0;
    unsigned int clock_mem_mhz     = 0;
    unsigned int clock_gpu_max_mhz = 0;
    unsigned int clock_mem_max_mhz = 0;

    unsigned int pcie_tx_kbps      = 0;
    unsigned int pcie_rx_kbps      = 0;
    unsigned int pcie_gen          = 0;
    unsigned int pcie_width        = 0;

    unsigned int encoder_util      = 0;
    unsigned int decoder_util      = 0;

    std::string driver_version;
    std::string cuda_version;
    std::string gpu_name;
    std::string pci_bus_id;
    std::string pci_device_id;

    int oc_delta_mhz               = 0;
    float theoretical_bandwidth_gbs = 0.0f;

    bool valid = false;
};
