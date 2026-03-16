#pragma once

#include "gpu_specs.h"
#include "nvml_monitor.h"
#include "gpu_database.h"
#include <vector>
#include <memory>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

class GpuWatchTui {
public:
    GpuWatchTui(NvmlMonitor& monitor, GpuDatabase& database);

    void run();
    void request_refresh();

private:
    ftxui::Element render_header();
    ftxui::Element render_specs_panel(const GpuSpecs& specs, const GpuLiveStats& stats);
    ftxui::Element render_live_panel(const GpuLiveStats& stats, const GpuSpecs& specs);
    ftxui::Element render_footer();

    ftxui::Element gauge_row(const std::string& label, float value,
                             const std::string& suffix,
                             ftxui::Color color = ftxui::Color::Green);

    ftxui::Element spec_row(const std::string& label, const std::string& value);

    NvmlMonitor& monitor_;
    GpuDatabase& database_;
    std::vector<GpuSpecs> gpu_specs_;
    int selected_gpu_ = 0;

    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();
};
