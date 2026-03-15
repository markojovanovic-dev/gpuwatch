#include "tui.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace ftxui;

// ── Formatting helpers ──────────────────────────────────────────────────────

static std::string fmt_mhz(unsigned int mhz) {
    return std::to_string(mhz) + " MHz";
}

static std::string fmt_watts(unsigned int mw) {
    float w = mw / 1000.0f;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << w << " W";
    return ss.str();
}

static std::string fmt_temp(unsigned int c) {
    return std::to_string(c) + "°C";
}

static std::string fmt_pct(unsigned int pct) {
    return std::to_string(pct) + "%";
}

static std::string fmt_bytes(unsigned long long bytes) {
    double gb = bytes / (1024.0 * 1024.0 * 1024.0);
    std::ostringstream ss;
    if (gb >= 1.0)
        ss << std::fixed << std::setprecision(1) << gb << " GB";
    else {
        double mb = bytes / (1024.0 * 1024.0);
        ss << std::fixed << std::setprecision(0) << mb << " MB";
    }
    return ss.str();
}

static std::string fmt_mem_mb(int mb) {
    if (mb >= 1024)
        return std::to_string(mb / 1024) + " GB";
    return std::to_string(mb) + " MB";
}

static std::string fmt_bandwidth_kbps(unsigned int kbps) {
    double mbps = kbps / 1024.0;
    double gbps = mbps / 1024.0;
    std::ostringstream ss;
    if (gbps >= 1.0)
        ss << std::fixed << std::setprecision(2) << gbps << " GB/s";
    else
        ss << std::fixed << std::setprecision(1) << mbps << " MB/s";
    return ss.str();
}

static std::string fmt_float(float val, int precision = 1) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << val;
    return ss.str();
}

static std::string fmt_l2(float mb) {
    if (mb >= 1.0f) {
        if (mb == std::floor(mb))
            return std::to_string((int)mb) + " MB";
        return fmt_float(mb) + " MB";
    }
    return fmt_float(mb * 1024.0f, 0) + " KB";
}

static Color gauge_color(float value) {
    if (value < 0.5f) return Color::Green;
    if (value < 0.8f) return Color::Yellow;
    return Color::Red;
}

static Color temp_color(unsigned int temp) {
    if (temp < 60) return Color::Green;
    if (temp < 80) return Color::Yellow;
    return Color::Red;
}

// ── TUI Implementation ─────────────────────────────────────────────────────

GpuWatchTui::GpuWatchTui(NvmlMonitor& monitor, GpuDatabase& database)
    : monitor_(monitor), database_(database)
{
    // Look up specs for each detected GPU
    for (int i = 0; i < monitor_.gpu_count(); i++) {
        auto pci_id = monitor_.pci_device_id(i);
        auto specs = database_.lookup_by_pci_id(pci_id);
        if (!specs.found) {
            specs = database_.lookup_by_name(monitor_.gpu_name(i));
        }
        if (!specs.found) {
            // Create a minimal entry from what NVML reports
            specs.name = monitor_.gpu_name(i);
            specs.pci_device_id = pci_id;
        }
        gpu_specs_.push_back(specs);
    }
}

Element GpuWatchTui::spec_row(const std::string& label, const std::string& value) {
    return hbox({
        text(label) | size(WIDTH, EQUAL, 18) | color(Color::GrayDark),
        text(value) | bold,
    });
}

Element GpuWatchTui::gauge_row(const std::string& label, float value,
                                const std::string& suffix, Color col) {
    value = std::clamp(value, 0.0f, 1.0f);
    return hbox({
        text(label) | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
        gauge(value) | flex | color(col),
        text(" " + suffix) | size(WIDTH, EQUAL, 12),
    });
}

Element GpuWatchTui::render_header() {
    auto stats = monitor_.cached_stats(selected_gpu_);
    std::string gpu_name = gpu_specs_.empty() ? "No GPU" :
        (gpu_specs_[selected_gpu_].found ? gpu_specs_[selected_gpu_].name
                                         : stats.gpu_name);

    return hbox({
        text("  gpuwatch") | bold | color(Color::Cyan),
        text(" v1.0") | color(Color::GrayDark),
        filler(),
        text(gpu_name) | bold | color(Color::White),
        filler(),
        text("Driver: ") | color(Color::GrayDark),
        text(stats.driver_version) | color(Color::White),
        text("  CUDA: ") | color(Color::GrayDark),
        text(stats.cuda_version) | color(Color::White),
        text("  "),
    }) | borderHeavy | color(Color::Cyan);
}

Element GpuWatchTui::render_specs_panel(const GpuSpecs& specs) {
    Elements rows;

    rows.push_back(text(" Hardware Specifications") | bold | color(Color::Cyan));
    rows.push_back(separator());

    // GPU Identity
    rows.push_back(text(" GPU Identity") | bold | color(Color::Yellow));
    if (!specs.architecture.empty())
        rows.push_back(spec_row("  Architecture:", specs.architecture));
    if (!specs.process.empty())
        rows.push_back(spec_row("  Process:", specs.process));
    if (specs.transistors_billion > 0)
        rows.push_back(spec_row("  Transistors:", fmt_float(specs.transistors_billion) + " B"));
    if (specs.die_size_mm2 > 0)
        rows.push_back(spec_row("  Die Size:", fmt_float(specs.die_size_mm2, 0) + " mm²"));
    if (!specs.compute_capability.empty())
        rows.push_back(spec_row("  Compute Cap:", "sm_" + specs.compute_capability));
    if (!specs.pci_device_id.empty())
        rows.push_back(spec_row("  PCI Device ID:", "10DE:" + specs.pci_device_id));

    rows.push_back(text(""));

    // Shader Units
    rows.push_back(text(" Shader Units") | bold | color(Color::Yellow));
    if (specs.cuda_cores > 0)
        rows.push_back(spec_row("  CUDA Cores:", std::to_string(specs.cuda_cores)));
    if (specs.sms > 0)
        rows.push_back(spec_row("  SMs:", std::to_string(specs.sms)));
    if (specs.tmus > 0)
        rows.push_back(spec_row("  TMUs:", std::to_string(specs.tmus)));
    if (specs.rops > 0)
        rows.push_back(spec_row("  ROPs:", std::to_string(specs.rops)));
    if (specs.rt_cores > 0)
        rows.push_back(spec_row("  RT Cores:", std::to_string(specs.rt_cores)));
    if (specs.tensor_cores > 0)
        rows.push_back(spec_row("  Tensor Cores:", std::to_string(specs.tensor_cores)));

    rows.push_back(text(""));

    // Clocks
    rows.push_back(text(" Clocks") | bold | color(Color::Yellow));
    if (specs.base_clock_mhz > 0)
        rows.push_back(spec_row("  Base Clock:", fmt_mhz(specs.base_clock_mhz)));
    if (specs.boost_clock_mhz > 0)
        rows.push_back(spec_row("  Boost Clock:", fmt_mhz(specs.boost_clock_mhz)));

    rows.push_back(text(""));

    // Memory
    rows.push_back(text(" Memory") | bold | color(Color::Yellow));
    if (specs.memory_size_mb > 0)
        rows.push_back(spec_row("  VRAM:", fmt_mem_mb(specs.memory_size_mb) +
                                 " " + specs.memory_type));
    if (specs.memory_bus_width > 0)
        rows.push_back(spec_row("  Bus Width:", std::to_string(specs.memory_bus_width) + "-bit"));
    if (specs.memory_bandwidth_gbs > 0)
        rows.push_back(spec_row("  Bandwidth:", fmt_float(specs.memory_bandwidth_gbs) + " GB/s"));
    if (specs.l2_cache_mb > 0)
        rows.push_back(spec_row("  L2 Cache:", fmt_l2(specs.l2_cache_mb)));

    rows.push_back(text(""));

    // Power
    rows.push_back(text(" Power") | bold | color(Color::Yellow));
    if (specs.tdp_watts > 0)
        rows.push_back(spec_row("  TDP:", std::to_string(specs.tdp_watts) + " W"));

    return vbox(std::move(rows)) | yframe | flex;
}

Element GpuWatchTui::render_live_panel(const GpuLiveStats& stats, const GpuSpecs& specs) {
    Elements rows;

    rows.push_back(text(" Live Monitoring") | bold | color(Color::Cyan));
    rows.push_back(separator());

    // Utilization
    rows.push_back(text(" Utilization") | bold | color(Color::Yellow));
    float gpu_pct = stats.gpu_utilization / 100.0f;
    float mem_pct = stats.mem_utilization / 100.0f;
    rows.push_back(gauge_row("  GPU Load:", gpu_pct,
                              fmt_pct(stats.gpu_utilization), gauge_color(gpu_pct)));
    rows.push_back(gauge_row("  Memory Ctrl:", mem_pct,
                              fmt_pct(stats.mem_utilization), gauge_color(mem_pct)));

    // VRAM usage
    float vram_pct = stats.mem_total > 0 ?
        (float)stats.mem_used / (float)stats.mem_total : 0.0f;
    rows.push_back(gauge_row("  VRAM Usage:", vram_pct,
                              fmt_pct((unsigned int)(vram_pct * 100)),
                              gauge_color(vram_pct)));
    rows.push_back(text(""));

    // Thermals
    rows.push_back(text(" Thermals & Power") | bold | color(Color::Yellow));
    float temp_pct = stats.temperature_max > 0 ?
        (float)stats.temperature / (float)stats.temperature_max : 0.0f;
    rows.push_back(gauge_row("  Temperature:", temp_pct,
                              fmt_temp(stats.temperature),
                              temp_color(stats.temperature)));

    float fan_pct = stats.fan_speed / 100.0f;
    rows.push_back(gauge_row("  Fan Speed:", fan_pct,
                              fmt_pct(stats.fan_speed),
                              Color::Cyan));

    float power_pct = stats.power_limit_mw > 0 ?
        (float)stats.power_draw_mw / (float)stats.power_limit_mw : 0.0f;
    rows.push_back(gauge_row("  Power Draw:", power_pct,
                              fmt_watts(stats.power_draw_mw),
                              gauge_color(power_pct)));
    rows.push_back(hbox({
        text("") | size(WIDTH, EQUAL, 16),
        text("  Limit: " + fmt_watts(stats.power_limit_mw) +
             (specs.tdp_watts > 0 ? "  TDP: " + std::to_string(specs.tdp_watts) + " W" : ""))
            | color(Color::GrayDark),
    }));
    rows.push_back(text(""));

    // Clocks
    rows.push_back(text(" Clocks") | bold | color(Color::Yellow));
    rows.push_back(hbox({
        text("  GPU Clock:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
        text(fmt_mhz(stats.clock_gpu_mhz)) | bold | color(Color::White),
    }));
    rows.push_back(hbox({
        text("  Mem Clock:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
        text(fmt_mhz(stats.clock_mem_mhz)) | bold | color(Color::White),
    }));

    // OC delta (current vs boost clock from specs)
    if (specs.boost_clock_mhz > 0) {
        int delta = (int)stats.clock_gpu_mhz - specs.boost_clock_mhz;
        std::string delta_str = (delta >= 0 ? "+" : "") + std::to_string(delta) + " MHz";
        Color delta_color = delta > 0 ? Color::Green :
                           (delta < -100 ? Color::Red : Color::Yellow);
        rows.push_back(hbox({
            text("  Boost Δ:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
            text(delta_str) | bold | color(delta_color),
            text("  (vs " + fmt_mhz(specs.boost_clock_mhz) + " spec)") | color(Color::GrayDark),
        }));
    }
    rows.push_back(text(""));

    // Memory details
    rows.push_back(text(" VRAM Details") | bold | color(Color::Yellow));
    rows.push_back(hbox({
        text("  Used:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
        text(fmt_bytes(stats.mem_used)) | bold,
        text(" / " + fmt_bytes(stats.mem_total)) | color(Color::GrayDark),
    }));
    rows.push_back(hbox({
        text("  Free:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
        text(fmt_bytes(stats.mem_free)) | color(Color::White),
    }));

    // Theoretical bandwidth
    if (specs.memory_bandwidth_gbs > 0) {
        rows.push_back(hbox({
            text("  Bandwidth:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
            text(fmt_float(specs.memory_bandwidth_gbs) + " GB/s (theoretical)") | color(Color::White),
        }));
    }
    rows.push_back(text(""));

    // PCIe
    rows.push_back(text(" PCIe") | bold | color(Color::Yellow));
    rows.push_back(hbox({
        text("  Link:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
        text("Gen" + std::to_string(stats.pcie_gen) +
             " x" + std::to_string(stats.pcie_width)) | bold | color(Color::White),
    }));
    rows.push_back(hbox({
        text("  TX:") | size(WIDTH, EQUAL, 16) | color(Color::GrayDark),
        text(fmt_bandwidth_kbps(stats.pcie_tx_kbps)) | color(Color::White),
        text("    RX: ") | color(Color::GrayDark),
        text(fmt_bandwidth_kbps(stats.pcie_rx_kbps)) | color(Color::White),
    }));
    rows.push_back(text(""));

    // Encoder / Decoder
    rows.push_back(text(" Video Engine") | bold | color(Color::Yellow));
    float enc_pct = stats.encoder_util / 100.0f;
    float dec_pct = stats.decoder_util / 100.0f;
    rows.push_back(gauge_row("  Encoder:", enc_pct,
                              fmt_pct(stats.encoder_util), Color::Magenta));
    rows.push_back(gauge_row("  Decoder:", dec_pct,
                              fmt_pct(stats.decoder_util), Color::Magenta));

    return vbox(std::move(rows)) | yframe | flex;
}

Element GpuWatchTui::render_footer() {
    Elements tabs;
    for (int i = 0; i < monitor_.gpu_count(); i++) {
        std::string label = " GPU " + std::to_string(i) + " ";
        if (i == selected_gpu_) {
            tabs.push_back(text(label) | bold | inverted | color(Color::Cyan));
        } else {
            tabs.push_back(text(label) | color(Color::GrayDark));
        }
        if (i + 1 < monitor_.gpu_count())
            tabs.push_back(text(" │ ") | color(Color::GrayDark));
    }

    return hbox({
        hbox(std::move(tabs)),
        filler(),
        text("←/→ Switch GPU") | color(Color::GrayDark),
        text("  "),
        text("Q") | bold | color(Color::Red),
        text(" Quit") | color(Color::GrayDark),
        text("  "),
    }) | borderLight;
}

void GpuWatchTui::request_refresh() {
    screen_.Post(ftxui::Event::Custom);
}

void GpuWatchTui::run() {
    auto renderer = Renderer([this]() {
        if (gpu_specs_.empty()) {
            return vbox({
                text("No NVIDIA GPUs detected!") | bold | center | color(Color::Red),
                text("Make sure NVIDIA drivers are installed.") | center,
            }) | border | center;
        }

        auto& specs = gpu_specs_[selected_gpu_];
        auto stats = monitor_.cached_stats(selected_gpu_);

        return vbox({
            render_header(),
            hbox({
                render_specs_panel(specs) | size(WIDTH, EQUAL, 42) | borderRounded,
                render_live_panel(stats, specs) | flex | borderRounded,
            }) | flex,
            render_footer(),
        });
    });

    // Handle keyboard input
    auto component = CatchEvent(renderer, [this](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q') ||
            event == Event::Escape) {
            screen_.Exit();
            return true;
        }
        if (event == Event::ArrowRight || event == Event::Character('l') ||
            event == Event::Tab) {
            if (monitor_.gpu_count() > 1) {
                selected_gpu_ = (selected_gpu_ + 1) % monitor_.gpu_count();
            }
            return true;
        }
        if (event == Event::ArrowLeft || event == Event::Character('h')) {
            if (monitor_.gpu_count() > 1) {
                selected_gpu_ = (selected_gpu_ - 1 + monitor_.gpu_count()) % monitor_.gpu_count();
            }
            return true;
        }
        return false;
    });

    // Start NVML polling in background
    monitor_.start_polling(1000, [this]() {
        request_refresh();
    });

    screen_.Loop(component);

    monitor_.stop_polling();
}
