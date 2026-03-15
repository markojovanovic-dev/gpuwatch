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

    bool init();
    void shutdown();

    int gpu_count() const { return static_cast<int>(devices_.size()); }
    std::string pci_device_id(int gpu_index) const;
    std::string gpu_name(int gpu_index) const;
    std::string pci_bus_id(int gpu_index) const;

    GpuLiveStats poll(int gpu_index);
    void start_polling(int interval_ms = 1000,
                       std::function<void()> on_update = nullptr);
    void stop_polling();
    GpuLiveStats cached_stats(int gpu_index);

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

    std::thread poll_thread_;
    std::atomic<bool> polling_{false};
    std::mutex stats_mutex_;
    std::vector<GpuLiveStats> cached_stats_;
    int poll_interval_ms_ = 1000;

    bool initialized_ = false;
};
