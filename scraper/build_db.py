#!/usr/bin/env python3
"""
build_db.py — Build the GPU specs SQLite database from hardcoded specifications.

This creates data/gpu_specs.db with detailed hardware specs for all NVIDIA
GeForce RTX GPUs from RTX 2060 through RTX 5090.

Usage:
    python3 scraper/build_db.py

The database is keyed by PCI device ID for runtime matching via NVML.
Multiple PCI device IDs can map to the same GPU model (different revisions).
"""

import os
import sqlite3
import sys

DB_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "gpu_specs.db")

SCHEMA = """
CREATE TABLE IF NOT EXISTS gpu_specs (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    name                TEXT NOT NULL,
    pci_device_id       TEXT NOT NULL,       -- Hex device ID, e.g. '1F08'
    architecture        TEXT NOT NULL,
    process             TEXT NOT NULL,
    memory_type         TEXT NOT NULL,
    compute_capability  TEXT NOT NULL,
    cuda_cores          INTEGER NOT NULL,
    tmus                INTEGER NOT NULL,
    rops                INTEGER NOT NULL,
    sms                 INTEGER NOT NULL,
    base_clock_mhz      INTEGER NOT NULL,
    boost_clock_mhz      INTEGER NOT NULL,
    memory_size_mb       INTEGER NOT NULL,
    memory_bus_width     INTEGER NOT NULL,    -- bits
    tdp_watts           INTEGER NOT NULL,
    memory_bandwidth_gbs REAL NOT NULL,
    l2_cache_mb          REAL NOT NULL,
    transistors_billion  REAL NOT NULL,
    die_size_mm2         REAL NOT NULL,
    rt_cores             INTEGER NOT NULL DEFAULT 0,
    tensor_cores         INTEGER NOT NULL DEFAULT 0,
    UNIQUE(pci_device_id)
);

CREATE INDEX IF NOT EXISTS idx_pci_id ON gpu_specs(pci_device_id);
CREATE INDEX IF NOT EXISTS idx_name ON gpu_specs(name);
"""

# ── GPU Specifications ───────────────────────────────────────────────────────
# Format: (name, [pci_device_ids], architecture, process, memory_type,
#          compute_cap, cuda_cores, tmus, rops, sms,
#          base_clock, boost_clock, memory_mb, bus_width, tdp,
#          bandwidth_gbs, l2_cache_mb, transistors_B, die_mm2,
#          rt_cores, tensor_cores)
#
# PCI device IDs are listed as multiple entries for different board revisions.
# Sources: TechPowerUp GPU Database, NVIDIA official specs, GPU-Z validation.

GPU_DATA = [
    # ═══════════════════════════════════════════════════════════════════════
    # Turing — RTX 20 Series (SM 7.5, 12nm TSMC)
    # ═══════════════════════════════════════════════════════════════════════

    ("GeForce RTX 2060", ["1F08", "1F03"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     1920, 120, 48, 30,   # cores, tmus, rops, sms
     1365, 1680,           # base, boost MHz
     6144, 192, 160,       # mem MB, bus bits, TDP
     336.0, 3.0,           # bandwidth GB/s, L2 MB
     10.8, 445.0,          # transistors B, die mm²
     30, 240),             # RT cores, tensor cores

    ("GeForce RTX 2060 SUPER", ["1F06", "1F47"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     2176, 136, 64, 34,
     1470, 1650,
     8192, 256, 175,
     448.0, 4.0,
     10.8, 445.0,
     34, 272),

    ("GeForce RTX 2060 12GB", ["1F03"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     2176, 136, 64, 34,
     1470, 1650,
     12288, 192, 185,
     336.0, 3.0,
     10.8, 445.0,
     34, 272),

    ("GeForce RTX 2070", ["1F02", "1F07"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     2304, 144, 64, 36,
     1410, 1620,
     8192, 256, 175,
     448.0, 4.0,
     10.8, 445.0,
     36, 288),

    ("GeForce RTX 2070 SUPER", ["1E84"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     2560, 160, 64, 40,
     1605, 1770,
     8192, 256, 215,
     448.0, 4.0,
     13.6, 545.0,
     40, 320),

    ("GeForce RTX 2080", ["1E82", "1E87"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     2944, 184, 64, 46,
     1515, 1710,
     8192, 256, 215,
     448.0, 4.0,
     13.6, 545.0,
     46, 368),

    ("GeForce RTX 2080 SUPER", ["1E81"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     3072, 192, 64, 48,
     1650, 1815,
     8192, 256, 250,
     496.0, 4.0,
     13.6, 545.0,
     48, 384),

    ("GeForce RTX 2080 Ti", ["1E04", "1E07"],
     "Turing", "12nm TSMC", "GDDR6", "7.5",
     4352, 272, 88, 68,
     1350, 1545,
     11264, 352, 250,
     616.0, 5.5,
     18.6, 754.0,
     68, 544),

    # ═══════════════════════════════════════════════════════════════════════
    # Ampere — RTX 30 Series (SM 8.6, Samsung 8nm)
    # ═══════════════════════════════════════════════════════════════════════

    ("GeForce RTX 3060", ["2503", "2504"],
     "Ampere", "8nm Samsung", "GDDR6", "8.6",
     3584, 112, 48, 28,
     1320, 1777,
     12288, 192, 170,
     360.0, 3.0,
     13.25, 276.0,
     28, 112),

    ("GeForce RTX 3060 Ti", ["2486", "2414"],
     "Ampere", "8nm Samsung", "GDDR6", "8.6",
     4864, 152, 80, 38,
     1410, 1665,
     8192, 256, 200,
     448.0, 4.0,
     17.4, 392.0,
     38, 152),

    ("GeForce RTX 3070", ["2484", "2488"],
     "Ampere", "8nm Samsung", "GDDR6", "8.6",
     5888, 184, 96, 46,
     1500, 1725,
     8192, 256, 220,
     448.0, 4.0,
     17.4, 392.0,
     46, 184),

    ("GeForce RTX 3070 Ti", ["2482"],
     "Ampere", "8nm Samsung", "GDDR6X", "8.6",
     6144, 192, 96, 48,
     1580, 1770,
     8192, 256, 290,
     608.3, 4.0,
     17.4, 392.0,
     48, 192),

    ("GeForce RTX 3080", ["2206", "2216"],
     "Ampere", "8nm Samsung", "GDDR6X", "8.6",
     8704, 272, 96, 68,
     1440, 1710,
     10240, 320, 320,
     760.3, 5.0,
     28.3, 628.0,
     68, 272),

    ("GeForce RTX 3080 12GB", ["220A"],
     "Ampere", "8nm Samsung", "GDDR6X", "8.6",
     8960, 280, 96, 70,
     1440, 1710,
     12288, 384, 350,
     912.4, 6.0,
     28.3, 628.0,
     70, 280),

    ("GeForce RTX 3080 Ti", ["2208"],
     "Ampere", "8nm Samsung", "GDDR6X", "8.6",
     10240, 320, 112, 80,
     1365, 1665,
     12288, 384, 350,
     912.4, 6.0,
     28.3, 628.0,
     80, 320),

    ("GeForce RTX 3090", ["2204"],
     "Ampere", "8nm Samsung", "GDDR6X", "8.6",
     10496, 328, 112, 82,
     1395, 1695,
     24576, 384, 350,
     936.2, 6.0,
     28.3, 628.0,
     82, 328),

    ("GeForce RTX 3090 Ti", ["2203"],
     "Ampere", "8nm Samsung", "GDDR6X", "8.6",
     10752, 336, 112, 84,
     1560, 1860,
     24576, 384, 450,
     1008.4, 6.0,
     28.3, 628.0,
     84, 336),

    # ═══════════════════════════════════════════════════════════════════════
    # Ada Lovelace — RTX 40 Series (SM 8.9, TSMC 4N / 5nm)
    # ═══════════════════════════════════════════════════════════════════════

    ("GeForce RTX 4060", ["2882"],
     "Ada Lovelace", "5nm TSMC", "GDDR6", "8.9",
     3072, 96, 48, 24,
     1830, 2460,
     8192, 128, 115,
     272.0, 24.0,
     18.9, 188.0,
     24, 96),

    ("GeForce RTX 4060 Ti", ["2803"],
     "Ada Lovelace", "5nm TSMC", "GDDR6", "8.9",
     4352, 136, 48, 34,
     2310, 2535,
     8192, 128, 160,
     288.0, 32.0,
     22.9, 295.0,
     34, 136),

    ("GeForce RTX 4060 Ti 16GB", ["2805"],
     "Ada Lovelace", "5nm TSMC", "GDDR6", "8.9",
     4352, 136, 48, 34,
     2310, 2535,
     16384, 128, 165,
     288.0, 32.0,
     22.9, 295.0,
     34, 136),

    ("GeForce RTX 4070", ["2786"],
     "Ada Lovelace", "5nm TSMC", "GDDR6X", "8.9",
     5888, 184, 64, 46,
     1920, 2475,
     12288, 192, 200,
     504.2, 36.0,
     35.8, 379.0,
     46, 184),

    ("GeForce RTX 4070 SUPER", ["2783"],
     "Ada Lovelace", "5nm TSMC", "GDDR6X", "8.9",
     7168, 224, 80, 56,
     1980, 2475,
     12288, 192, 220,
     504.2, 48.0,
     35.8, 379.0,
     56, 224),

    ("GeForce RTX 4070 Ti", ["2782"],
     "Ada Lovelace", "5nm TSMC", "GDDR6X", "8.9",
     7680, 240, 80, 60,
     2310, 2610,
     12288, 192, 285,
     504.2, 48.0,
     35.8, 379.0,
     60, 240),

    ("GeForce RTX 4070 Ti SUPER", ["2705"],
     "Ada Lovelace", "5nm TSMC", "GDDR6X", "8.9",
     8448, 264, 80, 66,
     2340, 2610,
     16384, 256, 285,
     672.3, 48.0,
     45.9, 379.0,
     66, 264),

    ("GeForce RTX 4080", ["2704"],
     "Ada Lovelace", "5nm TSMC", "GDDR6X", "8.9",
     9728, 304, 112, 76,
     2205, 2505,
     16384, 256, 320,
     716.8, 64.0,
     45.9, 379.0,
     76, 304),

    ("GeForce RTX 4080 SUPER", ["2702"],
     "Ada Lovelace", "5nm TSMC", "GDDR6X", "8.9",
     10240, 320, 112, 80,
     2295, 2550,
     16384, 256, 320,
     736.3, 64.0,
     45.9, 379.0,
     80, 320),

    ("GeForce RTX 4090", ["2684"],
     "Ada Lovelace", "5nm TSMC", "GDDR6X", "8.9",
     16384, 512, 176, 128,
     2235, 2520,
     24576, 384, 450,
     1008.4, 96.0,
     76.3, 608.0,
     128, 512),

    # ═══════════════════════════════════════════════════════════════════════
    # Blackwell — RTX 50 Series (SM 10.0+, TSMC 4NP / 5nm class)
    # ═══════════════════════════════════════════════════════════════════════

    ("GeForce RTX 5070", ["2B85"],
     "Blackwell", "4nm TSMC", "GDDR7", "10.0",
     6144, 192, 96, 48,
     2162, 2512,
     12288, 192, 250,
     672.0, 48.0,
     26.0, 262.0,
     48, 192),

    ("GeForce RTX 5070 Ti", ["2B02"],
     "Blackwell", "4nm TSMC", "GDDR7", "10.0",
     8960, 280, 112, 70,
     2162, 2452,
     16384, 256, 300,
     896.0, 64.0,
     41.6, 380.0,
     70, 280),

    ("GeForce RTX 5080", ["2B80", "2C02"],
     "Blackwell", "4nm TSMC", "GDDR7", "10.0",
     10752, 336, 112, 84,
     2295, 2617,
     16384, 256, 360,
     960.0, 64.0,
     45.6, 380.0,
     84, 336),

    ("GeForce RTX 5090", ["2B06"],
     "Blackwell", "4nm TSMC", "GDDR7", "10.0",
     21760, 680, 176, 170,
     2017, 2407,
     32768, 512, 575,
     1792.0, 128.0,
     92.2, 744.0,
     170, 680),
]


def build_database():
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)

    # Remove existing database to rebuild from scratch
    if os.path.exists(DB_PATH):
        os.remove(DB_PATH)

    conn = sqlite3.connect(DB_PATH)
    conn.executescript(SCHEMA)

    insert_sql = """
    INSERT OR REPLACE INTO gpu_specs (
        name, pci_device_id, architecture, process, memory_type,
        compute_capability, cuda_cores, tmus, rops, sms,
        base_clock_mhz, boost_clock_mhz, memory_size_mb, memory_bus_width,
        tdp_watts, memory_bandwidth_gbs, l2_cache_mb, transistors_billion,
        die_size_mm2, rt_cores, tensor_cores
    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """

    count = 0
    for entry in GPU_DATA:
        name = entry[0]
        pci_ids = entry[1]
        specs = entry[2:]  # Everything after the PCI IDs

        # Insert one row per PCI device ID (some GPUs have multiple revisions)
        for pci_id in pci_ids:
            conn.execute(insert_sql, (name, pci_id, *specs))
            count += 1

    conn.commit()

    # Verify
    cursor = conn.execute("SELECT COUNT(*) FROM gpu_specs")
    total = cursor.fetchone()[0]
    print(f"Built GPU specs database: {DB_PATH}")
    print(f"  {total} entries ({len(GPU_DATA)} GPU models)")

    # List all entries
    cursor = conn.execute(
        "SELECT name, pci_device_id, cuda_cores, memory_size_mb/1024, architecture "
        "FROM gpu_specs ORDER BY name"
    )
    for row in cursor:
        print(f"  {row[1]:>4s}  {row[0]:<35s}  {row[2]:>5d} CUDA  {row[3]:>2d} GB  {row[4]}")

    conn.close()


if __name__ == "__main__":
    build_database()
