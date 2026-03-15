#include "nvml_monitor.h"
#include "gpu_database.h"
#include "tui.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

#ifndef GPUWATCH_VERSION
#define GPUWATCH_VERSION "1.0.0"
#endif

#ifndef GPUWATCH_DATADIR
#define GPUWATCH_DATADIR "/usr/local/share/gpuwatch"
#endif

static std::string find_database() {
    // Search order:
    // 1. Next to the executable (for build-dir testing)
    // 2. ../share/gpuwatch/ relative to executable (standard FHS install)
    // 3. Compile-time GPUWATCH_DATADIR (cmake install prefix)
    // 4. Common system paths
    // 5. Relative to cwd (development fallback)

    auto exe_path = fs::read_symlink("/proc/self/exe").parent_path();

    std::vector<fs::path> candidates = {
        exe_path / "gpu_specs.db",
        exe_path / ".." / "share" / "gpuwatch" / "gpu_specs.db",
        fs::path(GPUWATCH_DATADIR) / "gpu_specs.db",
        "/usr/local/share/gpuwatch/gpu_specs.db",
        "/usr/share/gpuwatch/gpu_specs.db",
        fs::current_path() / "data" / "gpu_specs.db",
        fs::current_path() / "gpu_specs.db",
    };

    for (auto& p : candidates) {
        if (fs::exists(p))
            return fs::canonical(p).string();
    }
    return "";
}

static void print_help(const char* prog) {
    printf(
        "gpuwatch v" GPUWATCH_VERSION " — NVIDIA GPU monitoring TUI\n\n"
        "Usage: %s [OPTIONS] [database.db]\n\n"
        "Options:\n"
        "  -h, --help       Show this help message\n"
        "  -v, --version    Show version\n"
        "  -d, --db PATH    Path to gpu_specs.db\n\n"
        "Controls:\n"
        "  Left/Right, h/l  Switch between GPUs\n"
        "  Tab              Next GPU\n"
        "  Q / Esc          Quit\n\n"
        "Database:\n"
        "  Run 'python3 scraper/build_db.py' to generate the GPU specs database.\n"
        "  The database is auto-detected in the following locations:\n"
        "    - Next to the gpuwatch binary\n"
        "    - " GPUWATCH_DATADIR "/gpu_specs.db\n"
        "    - /usr/local/share/gpuwatch/gpu_specs.db\n"
        "    - ./data/gpu_specs.db\n",
        prog);
}

int main(int argc, char* argv[]) {
    std::string db_path;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            printf("gpuwatch v" GPUWATCH_VERSION "\n");
            return 0;
        }
        if ((arg == "-d" || arg == "--db") && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg[0] != '-') {
            db_path = arg;
        }
    }

    if (db_path.empty())
        db_path = find_database();

    // Initialize NVML
    NvmlMonitor monitor;
    if (!monitor.init()) {
        fprintf(stderr,
            "ERROR: Failed to initialize NVML.\n"
            "Make sure NVIDIA drivers are installed and an NVIDIA GPU is present.\n"
            "  sudo apt install nvidia-driver-XXX\n");
        return 1;
    }

    printf("Detected %d NVIDIA GPU(s):\n", monitor.gpu_count());
    for (int i = 0; i < monitor.gpu_count(); i++) {
        printf("  [%d] %s (PCI ID: 10DE:%s)\n",
               i, monitor.gpu_name(i).c_str(),
               monitor.pci_device_id(i).c_str());
    }

    // Open GPU specs database
    GpuDatabase database;
    if (db_path.empty()) {
        fprintf(stderr,
            "WARNING: GPU specs database not found.\n"
            "Run 'python3 scraper/build_db.py' to generate it, then reinstall.\n"
            "Hardware specifications panel will be limited.\n\n");
    } else if (!database.open(db_path)) {
        fprintf(stderr,
            "WARNING: Failed to open GPU specs database: %s\n"
            "Hardware specifications panel will be limited.\n\n",
            db_path.c_str());
    } else {
        printf("Loaded GPU specs database: %s\n", db_path.c_str());
    }

    printf("Starting gpuwatch TUI...\n");

    // Run the TUI
    GpuWatchTui tui(monitor, database);
    tui.run();

    return 0;
}
