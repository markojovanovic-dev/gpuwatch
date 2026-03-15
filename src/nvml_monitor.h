#pragma once

#include "gpu_specs.h"
#include <nvml.h>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

class NvmlMonitor {
public:
    NvmlMonitor();
    ~NvmlMonitor();

    // Initialize NVML and discover GPUs. Returns false on failure.
    bool init();

    // Shutdown NVML.
    void shutdown();

    // Number of detected GPUs.
    int gpu_count() const { return static_cast<int>(devices_.size()); }

    // Get the PCI device ID (hex string, e.g. "2B00") for a GPU index.
    std::string pci_device_id(int gpu_index) const;

    // Get the GPU name reported by NVML for a GPU index.
    std::string gpu_name(int gpu_index) const;

    // Get the PCI bus ID string for a GPU index.
    std::string pci_bus_id(int gpu_index) const;

    // Poll live stats for a specific GPU. Thread-safe.
    GpuLiveStats poll(int gpu_index);

    // Start background polling thread (calls on_update after each poll cycle).
    void start_polling(int interval_ms = 1000,
                       std::function<void()> on_update = nullptr);

    // Stop background polling.
    void stop_polling();

    // Get the latest cached stats for a GPU (thread-safe).
    GpuLiveStats cached_stats(int gpu_index);

    // Driver and CUDA version (global, not per-GPU).
    std::string driver_version() const { return driver_version_; }
    std::string cuda_version() const { return cuda_version_; }

private:
    struct DeviceInfo {
        nvmlDevice_t handle;
        std::string name;
        std::string pci_device_id;
        std::string pci_bus_id;
    };

    std::vector<DeviceInfo> devices_;
    std::string driver_version_;
    std::string cuda_version_;

    // Background polling
    std::thread poll_thread_;
    std::atomic<bool> polling_{false};
    std::mutex stats_mutex_;
    std::vector<GpuLiveStats> cached_stats_;
    int poll_interval_ms_ = 1000;

    bool initialized_ = false;
};
