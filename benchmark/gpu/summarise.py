#!/usr/bin/env python3
"""Print the README tables from the benchmark CSV files.

Usage: python3 benchmarks/summarise.py [RESULTS_CSV [THROUGHPUT_CSV]]
       (default: results_cpcrw_like.csv and throughput_cpcrw_like.csv next to this script: the
        CPCRW-like runs with fire off; the *_with_fire.csv files hold the runs with fire on)

Medians over the seeds of each setting (only runs with status ok). Builds:
upstream = iLand at the pinned commit; cpuopt = with the multithreading fixes;
gpu = cpuopt plus the CUDA path.
"""
import csv
import statistics
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).parent
NAMES = {"upstream": "Upstream iLand (CPU)", "cpuopt": "CPU fixes", "gpu": "CPU fixes + GPU"}
PARTS = [("establishment_s", "Establishment"), ("seed_dispersal_s", "Seed dispersal"),
         ("light_apply_s", "Light: apply"), ("light_read_s", "Light: read"),
         ("tree_growth_s", "Tree growth"), ("sapling_growth_s", "Sapling growth"),
         ("production_s", "Production + water"), ("outputs_s", "Outputs"),
         ("management_s", "Management script")]


def load(name):
    path = Path(name) if Path(name).is_absolute() or Path(name).exists() else HERE / name
    return list(csv.DictReader(open(path))) if path.exists() else []


def median(rows, key):
    values = [float(r[key]) for r in rows if r.get(key) not in ("", None)]
    return statistics.median(values) if values else float("nan")


def main(results="results_cpcrw_like.csv", throughput="throughput_cpcrw_like.csv"):
    rows = [r for r in load(results) if r["status"] == "ok"]
    groups = defaultdict(list)
    for r in rows:
        groups[(r["tag"], int(r["years"]), int(r["threads"]))].append(r)

    # 1. the CPCRW spin-up: 300 years, all cores
    main_rows = {b: groups.get((b, 300, -1), []) for b in NAMES}
    base = median(main_rows["upstream"], "wall_s")
    print("**300 years, all 28 cores** (median of %s seeds)\n" % "/".join(str(len(v)) for v in main_rows.values()))
    print("| Build | Wall time | Model time | Speed-up (wall) |")
    print("|---|---|---|---|")
    for b, name in NAMES.items():
        if main_rows[b]:
            wall, model = median(main_rows[b], "wall_s"), median(main_rows[b], "model_s")
            print(f"| {name} | {wall / 60:.1f} min | {model / 60:.1f} min | {base / wall:.2f}x |")
    print("\n**Where the model time goes** (seconds, same runs)\n")
    print("| Part | " + " | ".join(NAMES[b] for b in NAMES if main_rows[b]) + " |")
    print("|---|" + "---|" * sum(1 for b in NAMES if main_rows[b]))
    for key, label in PARTS:
        cells = [f"{median(main_rows[b], key):.1f}" for b in NAMES if main_rows[b]]
        print(f"| {label} | " + " | ".join(cells) + " |")

    # 2. thread scaling (150 years)
    print("\n**Thread scaling, 150 years** (model time, seconds)\n")
    threads = sorted({t for (tag, y, t) in groups if tag.endswith("_scaling") and y == 150},
                     key=lambda t: 10**6 if t == -1 else t)
    print("| Build | " + " | ".join("all (28)" if t == -1 else str(t) for t in threads) + " |")
    print("|---|" + "---|" * len(threads))
    for b, name in NAMES.items():
        cells = []
        for t in threads:
            g = groups.get((f"{b}_scaling", 150, t), [])
            cells.append(f"{median(g, 'model_s'):.1f}" if g else "")
        print(f"| {name} | " + " | ".join(cells) + " |")

    # 3. throughput
    tp = load(throughput)
    if tp:
        print("\n**Throughput, 150-year runs** (several runs side by side on one 28-core node with one GPU)\n")
        print("| Build | 1 run x 28 threads | 4 runs x 7 threads |")
        print("|---|---|---|")
        for b, name in NAMES.items():
            one = [r for r in tp if r["tag"] == b and r["concurrent_runs"] == "1"]
            four = [r for r in tp if r["tag"] == b and r["concurrent_runs"] == "4"]
            fmt = lambda g: f"{statistics.median(float(r['runs_per_hour']) for r in g):.0f} runs/h" if g else ""
            print(f"| {name} | {fmt(one)} | {fmt(four)} |")


if __name__ == "__main__":
    import sys
    main(*sys.argv[1:3])
