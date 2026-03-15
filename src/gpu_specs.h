#pragma once

#include <string>
#include <cstdint>

// Static hardware specifications loaded from the SQLite database.
// These don't change at runtime — they describe the physical GPU.
struct GpuSpecs {
    std::string name;              // e.g. "NVIDIA GeForce RTX 5080"
    std::string pci_device_id;     // e.g. "2B00" (hex, no vendor prefix)
    std::string architecture;      // e.g. "Blackwell"
    std::string process;           // e.g. "4nm TSMC"
    std::string memory_type;       // e.g. "GDDR7"
    std::string compute_capability;// e.g. "10.0"

    int cuda_cores      = 0;
    int tmus            = 0;
    int rops            = 0;
    int sms             = 0;
    int base_clock_mhz  = 0;
    int boost_clock_mhz = 0;
    int memory_size_mb  = 0;       // in MB (e.g. 16384 for 16 GB)
    int memory_bus_width = 0;      // in bits (e.g. 256)
    int tdp_watts       = 0;

    float memory_bandwidth_gbs = 0.0f; // theoretical, GB/s
    float l2_cache_mb          = 0.0f; // in MB
    float transistors_billion  = 0.0f;
    float die_size_mm2         = 0.0f;

    int rt_cores  = 0;             // ray tracing cores
    int tensor_cores = 0;          // tensor cores

    bool found = false;            // true if matched in database
};

// Live GPU statistics polled from NVML every ~1 second.
struct GpuLiveStats {
    // Utilization (0–100)
    unsigned int gpu_utilization    = 0;
    unsigned int mem_utilization    = 0;

    // Memory (bytes)
    unsigned long long mem_used     = 0;
    unsigned long long mem_free     = 0;
    unsigned long long mem_total    = 0;

    // Thermals & power
    unsigned int temperature        = 0;   // °C
    unsigned int temperature_max    = 0;   // °C (slowdown threshold)
    unsigned int fan_speed          = 0;   // 0–100%
    unsigned int power_draw_mw      = 0;   // milliwatts
    unsigned int power_limit_mw     = 0;   // milliwatts

    // Clocks (MHz)
    unsigned int clock_gpu_mhz     = 0;
    unsigned int clock_mem_mhz     = 0;
    unsigned int clock_gpu_max_mhz = 0;
    unsigned int clock_mem_max_mhz = 0;

    // PCIe
    unsigned int pcie_tx_kbps      = 0;   // KB/s
    unsigned int pcie_rx_kbps      = 0;   // KB/s
    unsigned int pcie_gen          = 0;
    unsigned int pcie_width        = 0;

    // Encoder/Decoder
    unsigned int encoder_util      = 0;   // 0–100%
    unsigned int decoder_util      = 0;   // 0–100%

    // Driver info
    std::string driver_version;
    std::string cuda_version;
    std::string gpu_name;
    std::string pci_bus_id;
    std::string pci_device_id;            // hex device ID

    // Derived
    int oc_delta_mhz               = 0;   // current clock − boost clock
    float theoretical_bandwidth_gbs = 0.0f;

    bool valid = false;
};
