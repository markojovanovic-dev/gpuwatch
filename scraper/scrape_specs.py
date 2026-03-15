#!/usr/bin/env python3
"""
scrape_specs.py — Scrape GPU specifications from TechPowerUp's GPU database.

This script pulls structured spec data from TechPowerUp's GPU pages and
updates the SQLite database. It's meant to be run offline to refresh or
verify the hardcoded data from build_db.py.

Usage:
    pip install -r scraper/requirements.txt
    python3 scraper/scrape_specs.py [--update] [--gpu "RTX 5080"]

Options:
    --update    Update existing database entries (default: skip existing)
    --gpu NAME  Only scrape a specific GPU model
    --list      List all configured GPU URLs
    --dry-run   Print scraped data without writing to database
"""

import argparse
import os
import re
import sqlite3
import sys
import time

try:
    import requests
    from bs4 import BeautifulSoup
except ImportError:
    print("Install dependencies: pip install -r scraper/requirements.txt")
    sys.exit(1)

DB_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "gpu_specs.db")

# TechPowerUp GPU page URLs keyed by model name.
# These are the canonical spec pages for each RTX GPU.
GPU_URLS = {
    # Turing — RTX 20 Series
    "GeForce RTX 2060":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-2060.c3310",
    "GeForce RTX 2060 SUPER": "https://www.techpowerup.com/gpu-specs/geforce-rtx-2060-super.c3441",
    "GeForce RTX 2060 12GB":  "https://www.techpowerup.com/gpu-specs/geforce-rtx-2060-12-gb.c3810",
    "GeForce RTX 2070":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-2070.c3252",
    "GeForce RTX 2070 SUPER": "https://www.techpowerup.com/gpu-specs/geforce-rtx-2070-super.c3440",
    "GeForce RTX 2080":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-2080.c3224",
    "GeForce RTX 2080 SUPER": "https://www.techpowerup.com/gpu-specs/geforce-rtx-2080-super.c3439",
    "GeForce RTX 2080 Ti":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-2080-ti.c3305",

    # Ampere — RTX 30 Series
    "GeForce RTX 3060":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-3060.c3682",
    "GeForce RTX 3060 Ti":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-3060-ti.c3681",
    "GeForce RTX 3070":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-3070.c3674",
    "GeForce RTX 3070 Ti":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-3070-ti.c3675",
    "GeForce RTX 3080":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-3080.c3621",
    "GeForce RTX 3080 12GB":  "https://www.techpowerup.com/gpu-specs/geforce-rtx-3080-12-gb.c3834",
    "GeForce RTX 3080 Ti":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-3080-ti.c3735",
    "GeForce RTX 3090":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-3090.c3622",
    "GeForce RTX 3090 Ti":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-3090-ti.c3829",

    # Ada Lovelace — RTX 40 Series
    "GeForce RTX 4060":          "https://www.techpowerup.com/gpu-specs/geforce-rtx-4060.c3916",
    "GeForce RTX 4060 Ti":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-4060-ti.c3915",
    "GeForce RTX 4060 Ti 16GB":  "https://www.techpowerup.com/gpu-specs/geforce-rtx-4060-ti-16-gb.c4015",
    "GeForce RTX 4070":          "https://www.techpowerup.com/gpu-specs/geforce-rtx-4070.c3924",
    "GeForce RTX 4070 SUPER":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-4070-super.c4172",
    "GeForce RTX 4070 Ti":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-4070-ti.c3950",
    "GeForce RTX 4070 Ti SUPER": "https://www.techpowerup.com/gpu-specs/geforce-rtx-4070-ti-super.c4173",
    "GeForce RTX 4080":          "https://www.techpowerup.com/gpu-specs/geforce-rtx-4080.c3888",
    "GeForce RTX 4080 SUPER":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-4080-super.c4174",
    "GeForce RTX 4090":          "https://www.techpowerup.com/gpu-specs/geforce-rtx-4090.c3889",

    # Blackwell — RTX 50 Series
    "GeForce RTX 5070":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-5070.c4194",
    "GeForce RTX 5070 Ti":    "https://www.techpowerup.com/gpu-specs/geforce-rtx-5070-ti.c4193",
    "GeForce RTX 5080":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-5080.c4192",
    "GeForce RTX 5090":       "https://www.techpowerup.com/gpu-specs/geforce-rtx-5090.c4191",
}

HEADERS = {
    "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0",
    "Accept": "text/html,application/xhtml+xml",
    "Accept-Language": "en-US,en;q=0.9",
}


def parse_int(text: str) -> int:
    """Extract integer from text like '10752' or '10,752'."""
    m = re.search(r'[\d,]+', text.replace(',', ''))
    return int(m.group()) if m else 0


def parse_float(text: str) -> float:
    """Extract float from text like '960.0' or '45.9'."""
    m = re.search(r'[\d.]+', text)
    return float(m.group()) if m else 0.0


def parse_mhz(text: str) -> int:
    """Extract MHz value from text like '2617 MHz'."""
    m = re.search(r'(\d+)\s*MHz', text)
    return int(m.group(1)) if m else 0


def scrape_gpu(name: str, url: str) -> dict | None:
    """Scrape a single GPU spec page from TechPowerUp."""
    print(f"  Scraping {name}...")

    try:
        resp = requests.get(url, headers=HEADERS, timeout=15)
        resp.raise_for_status()
    except requests.RequestException as e:
        print(f"    ERROR: {e}")
        return None

    soup = BeautifulSoup(resp.text, "lxml")

    # TechPowerUp uses <dl class="clearfix"> with <dt> and <dd> pairs
    specs = {}
    for section in soup.select("section.details"):
        for dl in section.select("dl"):
            dts = dl.select("dt")
            dds = dl.select("dd")
            for dt, dd in zip(dts, dds):
                key = dt.get_text(strip=True).lower()
                val = dd.get_text(strip=True)
                specs[key] = val

    if not specs:
        # Fallback: try table-based layout
        for table in soup.select("table.gpu-specs-table, table"):
            for row in table.select("tr"):
                cells = row.select("td, th")
                if len(cells) >= 2:
                    key = cells[0].get_text(strip=True).lower()
                    val = cells[1].get_text(strip=True)
                    specs[key] = val

    if not specs:
        print(f"    WARNING: Could not parse specs from {url}")
        return None

    # Map TechPowerUp field names to our schema
    result = {"name": name}

    # PCI Device ID
    device_id = specs.get("device id", specs.get("pci device", ""))
    m = re.search(r'([0-9A-Fa-f]{4})', device_id)
    result["pci_device_id"] = m.group(1).upper() if m else ""

    # Architecture
    result["architecture"] = specs.get("gpu variant", specs.get("architecture", ""))

    # Process
    result["process"] = specs.get("process size", specs.get("process", ""))

    # Memory type
    mem_type = specs.get("memory type", "")
    result["memory_type"] = mem_type

    # Compute capability
    cc = specs.get("compute capability", "")
    result["compute_capability"] = cc

    # Shader counts
    result["cuda_cores"] = parse_int(specs.get("shading units", specs.get("cuda cores", "0")))
    result["tmus"] = parse_int(specs.get("tmus", specs.get("texture mapping units", "0")))
    result["rops"] = parse_int(specs.get("rops", specs.get("render output units", "0")))
    result["sms"] = parse_int(specs.get("sms", specs.get("streaming multiprocessors", "0")))
    result["rt_cores"] = parse_int(specs.get("rt cores", "0"))
    result["tensor_cores"] = parse_int(specs.get("tensor cores", "0"))

    # Clocks
    result["base_clock_mhz"] = parse_mhz(specs.get("base clock", specs.get("gpu clock", "0")))
    result["boost_clock_mhz"] = parse_mhz(specs.get("boost clock", "0"))

    # Memory
    mem_size = specs.get("memory size", "0")
    mem_mb = parse_int(mem_size)
    if "GB" in mem_size.upper():
        mem_mb *= 1024
    result["memory_size_mb"] = mem_mb

    bus = specs.get("memory bus", specs.get("bus width", "0"))
    result["memory_bus_width"] = parse_int(bus)

    bw = specs.get("memory bandwidth", specs.get("bandwidth", "0"))
    result["memory_bandwidth_gbs"] = parse_float(bw)

    # L2 cache
    l2 = specs.get("l2 cache", "0")
    l2_val = parse_float(l2)
    if "KB" in l2.upper():
        l2_val /= 1024.0
    result["l2_cache_mb"] = l2_val

    # TDP
    tdp = specs.get("tdp", specs.get("total board power", "0"))
    result["tdp_watts"] = parse_int(tdp)

    # Transistors & die
    trans = specs.get("transistor count", specs.get("transistors", "0"))
    trans_val = parse_float(trans)
    if "billion" in trans.lower() or "B" in trans:
        pass  # already in billions
    elif "million" in trans.lower() or trans_val > 1000:
        trans_val /= 1000.0
    result["transistors_billion"] = trans_val

    die = specs.get("die size", "0")
    result["die_size_mm2"] = parse_float(die)

    print(f"    OK: {result.get('cuda_cores', 0)} CUDA cores, "
          f"{result.get('memory_size_mb', 0)} MB, "
          f"PCI: {result.get('pci_device_id', '?')}")

    return result


def update_database(gpu_data: list[dict], update_existing: bool = False):
    """Write scraped GPU data to the SQLite database."""
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)

    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS gpu_specs (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            name                TEXT NOT NULL,
            pci_device_id       TEXT NOT NULL,
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
            memory_bus_width     INTEGER NOT NULL,
            tdp_watts           INTEGER NOT NULL,
            memory_bandwidth_gbs REAL NOT NULL,
            l2_cache_mb          REAL NOT NULL,
            transistors_billion  REAL NOT NULL,
            die_size_mm2         REAL NOT NULL,
            rt_cores             INTEGER NOT NULL DEFAULT 0,
            tensor_cores         INTEGER NOT NULL DEFAULT 0,
            UNIQUE(pci_device_id)
        )
    """)

    mode = "REPLACE" if update_existing else "IGNORE"
    sql = f"""
    INSERT OR {mode} INTO gpu_specs (
        name, pci_device_id, architecture, process, memory_type,
        compute_capability, cuda_cores, tmus, rops, sms,
        base_clock_mhz, boost_clock_mhz, memory_size_mb, memory_bus_width,
        tdp_watts, memory_bandwidth_gbs, l2_cache_mb, transistors_billion,
        die_size_mm2, rt_cores, tensor_cores
    ) VALUES (
        :name, :pci_device_id, :architecture, :process, :memory_type,
        :compute_capability, :cuda_cores, :tmus, :rops, :sms,
        :base_clock_mhz, :boost_clock_mhz, :memory_size_mb, :memory_bus_width,
        :tdp_watts, :memory_bandwidth_gbs, :l2_cache_mb, :transistors_billion,
        :die_size_mm2, :rt_cores, :tensor_cores
    )
    """

    for data in gpu_data:
        if data and data.get("pci_device_id"):
            conn.execute(sql, data)

    conn.commit()
    total = conn.execute("SELECT COUNT(*) FROM gpu_specs").fetchone()[0]
    print(f"\nDatabase updated: {total} total entries in {DB_PATH}")
    conn.close()


def main():
    parser = argparse.ArgumentParser(description="Scrape GPU specs from TechPowerUp")
    parser.add_argument("--update", action="store_true",
                        help="Update existing database entries")
    parser.add_argument("--gpu", type=str, help="Only scrape a specific GPU model")
    parser.add_argument("--list", action="store_true", help="List all configured GPUs")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print data without writing to database")
    parser.add_argument("--delay", type=float, default=2.0,
                        help="Delay between requests in seconds (default: 2.0)")
    args = parser.parse_args()

    if args.list:
        print("Configured GPU models:")
        for name, url in sorted(GPU_URLS.items()):
            print(f"  {name:<35s}  {url}")
        return

    # Select GPUs to scrape
    targets = {}
    if args.gpu:
        for name, url in GPU_URLS.items():
            if args.gpu.lower() in name.lower():
                targets[name] = url
        if not targets:
            print(f"No GPU matching '{args.gpu}' found.")
            print("Use --list to see available models.")
            return
    else:
        targets = GPU_URLS

    print(f"Scraping {len(targets)} GPU(s) from TechPowerUp...")
    results = []

    for i, (name, url) in enumerate(targets.items()):
        data = scrape_gpu(name, url)
        if data:
            results.append(data)

        # Rate limiting
        if i < len(targets) - 1:
            time.sleep(args.delay)

    print(f"\nSuccessfully scraped {len(results)}/{len(targets)} GPUs")

    if args.dry_run:
        print("\nDry run — not writing to database. Scraped data:")
        for r in results:
            print(f"\n  {r['name']}:")
            for k, v in r.items():
                if k != "name":
                    print(f"    {k}: {v}")
    else:
        update_database(results, update_existing=args.update)


if __name__ == "__main__":
    main()
