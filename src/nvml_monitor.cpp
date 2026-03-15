#include "nvml_monitor.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>

static const char* nvml_err(nvmlReturn_t r) {
    return nvmlErrorString(r);
}

NvmlMonitor::NvmlMonitor() = default;

NvmlMonitor::~NvmlMonitor() {
    stop_polling();
    shutdown();
}

bool NvmlMonitor::init() {
    nvmlReturn_t r = nvmlInit_v2();
    if (r != NVML_SUCCESS) {
        fprintf(stderr, "nvmlInit failed: %s\n", nvml_err(r));
        return false;
    }
    initialized_ = true;

    char buf[NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE];
    if (nvmlSystemGetDriverVersion(buf, sizeof(buf)) == NVML_SUCCESS)
        driver_version_ = buf;

    int cuda_ver = 0;
    if (nvmlSystemGetCudaDriverVersion(&cuda_ver) == NVML_SUCCESS) {
        int major = cuda_ver / 1000;
        int minor = (cuda_ver % 1000) / 10;
        cuda_version_ = std::to_string(major) + "." + std::to_string(minor);
    }

    unsigned int count = 0;
    r = nvmlDeviceGetCount_v2(&count);
    if (r != NVML_SUCCESS) {
        fprintf(stderr, "nvmlDeviceGetCount failed: %s\n", nvml_err(r));
        return false;
    }

    for (unsigned int i = 0; i < count; i++) {
        nvmlDevice_t dev;
        r = nvmlDeviceGetHandleByIndex_v2(i, &dev);
        if (r != NVML_SUCCESS) continue;

        DeviceInfo info;
        info.handle = dev;

        char name[NVML_DEVICE_NAME_V2_BUFFER_SIZE];
        if (nvmlDeviceGetName(dev, name, sizeof(name)) == NVML_SUCCESS)
            info.name = name;

        nvmlPciInfo_t pci;
        if (nvmlDeviceGetPciInfo_v3(dev, &pci) == NVML_SUCCESS) {
            info.pci_bus_id = pci.busId;
            unsigned int dev_id = (pci.pciDeviceId >> 16) & 0xFFFF;
            char hex[8];
            snprintf(hex, sizeof(hex), "%04X", dev_id);
            info.pci_device_id = hex;
        }

        devices_.push_back(std::move(info));
    }

    cached_stats_.resize(devices_.size());
    return !devices_.empty();
}

void NvmlMonitor::shutdown() {
    if (initialized_) {
        nvmlShutdown();
        initialized_ = false;
    }
}

std::string NvmlMonitor::pci_device_id(int idx) const {
    if (idx < 0 || idx >= (int)devices_.size()) return "";
    return devices_[idx].pci_device_id;
}

std::string NvmlMonitor::gpu_name(int idx) const {
    if (idx < 0 || idx >= (int)devices_.size()) return "";
    return devices_[idx].name;
}

std::string NvmlMonitor::pci_bus_id(int idx) const {
    if (idx < 0 || idx >= (int)devices_.size()) return "";
    return devices_[idx].pci_bus_id;
}

GpuLiveStats NvmlMonitor::poll(int idx) {
    GpuLiveStats s;
    if (idx < 0 || idx >= (int)devices_.size()) return s;

    auto& dev = devices_[idx].handle;
    s.gpu_name = devices_[idx].name;
    s.pci_bus_id = devices_[idx].pci_bus_id;
    s.pci_device_id = devices_[idx].pci_device_id;
    s.driver_version = driver_version_;
    s.cuda_version = cuda_version_;

    nvmlUtilization_t util;
    if (nvmlDeviceGetUtilizationRates(dev, &util) == NVML_SUCCESS) {
        s.gpu_utilization = util.gpu;
        s.mem_utilization = util.memory;
    }

    nvmlMemory_t mem;
    if (nvmlDeviceGetMemoryInfo(dev, &mem) == NVML_SUCCESS) {
        s.mem_used  = mem.used;
        s.mem_free  = mem.free;
        s.mem_total = mem.total;
    }

    unsigned int temp;
    if (nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS)
        s.temperature = temp;

    unsigned int temp_max;
    if (nvmlDeviceGetTemperatureThreshold(dev, NVML_TEMPERATURE_THRESHOLD_SLOWDOWN, &temp_max) == NVML_SUCCESS)
        s.temperature_max = temp_max;

    unsigned int fan;
    if (nvmlDeviceGetFanSpeed(dev, &fan) == NVML_SUCCESS)
        s.fan_speed = fan;

    unsigned int power;
    if (nvmlDeviceGetPowerUsage(dev, &power) == NVML_SUCCESS)
        s.power_draw_mw = power;

    unsigned int power_limit;
    if (nvmlDeviceGetPowerManagementLimit(dev, &power_limit) == NVML_SUCCESS)
        s.power_limit_mw = power_limit;

    unsigned int clk;
    if (nvmlDeviceGetClockInfo(dev, NVML_CLOCK_GRAPHICS, &clk) == NVML_SUCCESS)
        s.clock_gpu_mhz = clk;
    if (nvmlDeviceGetClockInfo(dev, NVML_CLOCK_MEM, &clk) == NVML_SUCCESS)
        s.clock_mem_mhz = clk;
    if (nvmlDeviceGetMaxClockInfo(dev, NVML_CLOCK_GRAPHICS, &clk) == NVML_SUCCESS)
        s.clock_gpu_max_mhz = clk;
    if (nvmlDeviceGetMaxClockInfo(dev, NVML_CLOCK_MEM, &clk) == NVML_SUCCESS)
        s.clock_mem_max_mhz = clk;

    unsigned int tx, rx;
    if (nvmlDeviceGetPcieThroughput(dev, NVML_PCIE_UTIL_TX_BYTES, &tx) == NVML_SUCCESS)
        s.pcie_tx_kbps = tx;
    if (nvmlDeviceGetPcieThroughput(dev, NVML_PCIE_UTIL_RX_BYTES, &rx) == NVML_SUCCESS)
        s.pcie_rx_kbps = rx;

    unsigned int gen, width;
    if (nvmlDeviceGetCurrPcieLinkGeneration(dev, &gen) == NVML_SUCCESS)
        s.pcie_gen = gen;
    if (nvmlDeviceGetCurrPcieLinkWidth(dev, &width) == NVML_SUCCESS)
        s.pcie_width = width;

    unsigned int enc_util, enc_period;
    if (nvmlDeviceGetEncoderUtilization(dev, &enc_util, &enc_period) == NVML_SUCCESS)
        s.encoder_util = enc_util;
    unsigned int dec_util, dec_period;
    if (nvmlDeviceGetDecoderUtilization(dev, &dec_util, &dec_period) == NVML_SUCCESS)
        s.decoder_util = dec_util;

    s.valid = true;
    return s;
}

void NvmlMonitor::start_polling(int interval_ms, std::function<void()> on_update) {
    if (polling_) return;
    poll_interval_ms_ = interval_ms;
    polling_ = true;

    poll_thread_ = std::thread([this, on_update]() {
        while (polling_) {
            for (int i = 0; i < gpu_count(); i++) {
                auto stats = poll(i);
                {
                    std::lock_guard<std::mutex> lock(stats_mutex_);
                    cached_stats_[i] = stats;
                }
            }
            if (on_update) on_update();

            auto end = std::chrono::steady_clock::now() +
                       std::chrono::milliseconds(poll_interval_ms_);
            while (polling_ && std::chrono::steady_clock::now() < end) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    });
}

void NvmlMonitor::stop_polling() {
    polling_ = false;
    if (poll_thread_.joinable())
        poll_thread_.join();
}

GpuLiveStats NvmlMonitor::cached_stats(int idx) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (idx < 0 || idx >= (int)cached_stats_.size()) return {};
    return cached_stats_[idx];
}
