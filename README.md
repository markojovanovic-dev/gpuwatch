# gpuwatch

A terminal-based GPU inspection and monitoring tool for Linux. Think GPU-Z, but in your terminal.

Combines real-time NVIDIA GPU stats via NVML with deep static hardware specifications from a bundled SQLite database. The TUI shows a split-panel view: static hardware specs on the left, live gauges on the right, refreshing every second.

![NVIDIA](https://img.shields.io/badge/NVIDIA-RTX%2020%2F30%2F40%2F50-76B900?logo=nvidia)
![Platform](https://img.shields.io/badge/Platform-Linux-blue)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B)
![License](https://img.shields.io/badge/License-MIT-yellow)

## Features

### Static Hardware Specifications (left panel)
- Architecture, process node, die size, transistor count
- CUDA cores, SMs, TMUs, ROPs, RT cores, tensor cores
- Base & boost clocks
- VRAM size & type, bus width, theoretical bandwidth, L2 cache
- Compute capability, TDP, PCI device ID

### Live Monitoring (right panel, ~1s refresh)
- GPU & memory controller utilization gauges
- VRAM usage gauge with used/free/total breakdown
- Temperature with color-coded thresholds (green/yellow/red)
- Fan speed, power draw vs limit
- Current GPU & memory clocks with boost delta (current vs spec boost clock)
- Overclock detection — shows OC offset when GPU max clock exceeds spec boost
- PCIe link info (generation, width) and TX/RX throughput
- Video encoder & decoder utilization

### Multi-GPU Support
- Auto-detects all NVIDIA GPUs
- Switch between GPUs with arrow keys or `h`/`l`

### GPU Database
- 31 GPU models covered (39 PCI device ID variants)
- Covers RTX 2060 through RTX 5090 (all Super/Ti/12GB/16GB variants)
- Matched at runtime via PCI device ID detected through NVML
- Fallback name-based matching if PCI ID isn't in the database

## Example

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  gpuwatch v1.0        GeForce RTX 5080        Driver: 590.48.01  CUDA: 13.1  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
╭──────────────────────────────────────────╮╭──────────────────────────────────╮
│ Hardware Specifications                  ││ Live Monitoring                  │
├──────────────────────────────────────────┤├──────────────────────────────────┤
│ GPU Identity                             ││ Utilization                      │
│  Architecture:   Blackwell               ││  GPU Load:     ████▍      44%   │
│  Process:        4nm TSMC                ││  Memory Ctrl:  ██▊        28%   │
│  Transistors:    45.6 B                  ││  VRAM Usage:   █▋         17%   │
│  Die Size:       380 mm²                 ││                                  │
│  Compute Cap:    sm_10.0                 ││ Thermals & Power                 │
│  PCI Device ID:  10DE:2C02               ││  Temperature:  ████▊      58°C  │
│                                          ││  Fan Speed:    ██▍        24%   │
│ Shader Units                             ││  Power Draw:   ███▍       35%   │
│  CUDA Cores:     10752                   ││                                  │
│  SMs:            84                      ││ Clocks                           │
│  TMUs:           336                     ││  GPU Clock:    2850 MHz          │
│  ROPs:           112                     ││  Mem Clock:    7001 MHz          │
│  RT Cores:       84                      ││  Boost Δ:      +233 MHz         │
│  Tensor Cores:   336                     ││  OC Offset:    +150 MHz         │
│                                          ││                                  │
│                                          ││ VRAM Details                     │
│ Clocks                                   ││  Used:         2.7 GB / 16.0 GB │
│  Base Clock:     2295 MHz                ││  Free:         13.3 GB           │
│  Boost Clock:    2617 MHz                ││                                  │
│                                          ││ PCIe                             │
│ Memory                                   ││  Link:         Gen5 x16         │
│  VRAM:           16 GB GDDR7             ││  TX: 0.42 GB/s   RX: 0.18 GB/s │
│  Bus Width:      256-bit                 ││                                  │
│  Bandwidth:      960.0 GB/s              ││ Video Engine                     │
│  L2 Cache:       64 MB                   ││  Encoder:      ░░░░░░     0%    │
│                                          ││  Decoder:      ░░░░░░     0%    │
│ Power                                    ││                                  │
│  TDP:            360 W                   ││                                  │
╰──────────────────────────────────────────╯╰──────────────────────────────────╯
┌──────────────────────────────────────────────────────────────────────────────┐
│ GPU 0                                                 ←/→ Switch GPU  Q Quit│
└──────────────────────────────────────────────────────────────────────────────┘
```

## Supported GPUs (v1.0)

| Series | Models |
|--------|--------|
| **RTX 20 (Turing)** | 2060, 2060 Super, 2060 12GB, 2070, 2070 Super, 2080, 2080 Super, 2080 Ti |
| **RTX 30 (Ampere)** | 3060, 3060 Ti, 3070, 3070 Ti, 3080, 3080 12GB, 3080 Ti, 3090, 3090 Ti |
| **RTX 40 (Ada Lovelace)** | 4060, 4060 Ti, 4060 Ti 16GB, 4070, 4070 Super, 4070 Ti, 4070 Ti Super, 4080, 4080 Super, 4090 |
| **RTX 50 (Blackwell)** | 5070, 5070 Ti, 5080, 5090 |

## Requirements

- **OS**: Any modern Linux distro (Ubuntu, Fedora, Arch, etc.)
- **GPU**: NVIDIA with proprietary drivers installed

## Install

### Debian / Ubuntu

**From .deb package (recommended):**

```bash
curl -sL https://github.com/markojovanovic-dev/gpuwatch/releases/latest/download/gpuwatch-amd64.deb -o /tmp/gpuwatch.deb
sudo apt install /tmp/gpuwatch.deb
```

**Build from source:**

```bash
sudo apt install build-essential cmake pkg-config libsqlite3-dev nvidia-cuda-toolkit
git clone https://github.com/markojovanovic-dev/gpuwatch.git && cd gpuwatch
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

### Arch Linux

```bash
sudo pacman -S base-devel cmake pkgconf sqlite cuda
git clone https://github.com/markojovanovic-dev/gpuwatch.git && cd gpuwatch
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

### Fedora / RHEL

```bash
sudo dnf install gcc-c++ cmake pkgconf-pkg-config sqlite-devel cuda-toolkit
git clone https://github.com/markojovanovic-dev/gpuwatch.git && cd gpuwatch
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

### Run without installing

```bash
./build/gpuwatch
```

## Usage

```
gpuwatch [OPTIONS] [database.db]

Options:
  -h, --help       Show help
  -v, --version    Show version
  -d, --db PATH    Path to gpu_specs.db

Controls:
  Left/Right, h/l  Switch between GPUs
  Tab              Next GPU
  Q / Esc          Quit
```

## Architecture

```
src/
├── main.cpp            Entry point, CLI argument parsing, DB discovery
├── gpu_specs.h         Data structs: GpuSpecs (static), GpuLiveStats (live)
├── nvml_monitor.h/cpp  NVML wrapper: GPU discovery, background polling thread
├── gpu_database.h/cpp  SQLite reader: lookup by PCI device ID or GPU name
└── tui.h/cpp           FTXUI TUI: dual-panel layout, gauges, keyboard input
```

**Key design decisions:**
- PCI device ID is the primary key for matching GPUs at runtime (detected via NVML, looked up in SQLite)
- NVML polling runs in a background thread, pushes updates to the TUI via `PostEvent`
- FTXUI renders the full layout each frame — no incremental updates, which keeps the code simple
- The database is bundled and installed alongside the binary, no network access at runtime

## Roadmap

### v1.1
- GPU process list (which processes are using VRAM)
- Clock speed / temperature history graphs (sparklines)
- Configurable refresh interval
- VRAM allocation breakdown

### v1.2
- Export stats to CSV/JSON
- Alert thresholds (temperature, power, VRAM)
- Custom color themes

### v2.0 — AMD Support
- AMD GPU support via ROCm SMI / sysfs
- Unified TUI for mixed NVIDIA + AMD systems
- AMD GPU database (RX 7000 / 9000 series)

### v2.1
- Intel Arc GPU support via `intel_gpu_top` / sysfs
- Per-GPU process isolation view

## License

MIT

## Contributing

Contributions welcome. If your GPU's PCI device ID isn't recognized (you'll see an empty specs panel), run:

```bash
# Find your GPU's PCI device ID
nvidia-smi --query-gpu=pci.device_id --format=csv,noheader
```

Then open an issue or PR with the device ID and GPU model name.
