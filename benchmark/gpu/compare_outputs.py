#!/usr/bin/env python3
"""Do the builds simulate the same forest? Compare model outputs of the 300-year benchmark runs.

Usage: python3 benchmarks/compare_outputs.py PROJECT_DIR [--fire-off]
       (--fire-off: compare the runs made with modules.fire.enabled=false instead)

For every finished 300-year run with fire in PROJECT_DIR/output (folders named BUILD_y300_...,
made by run_benchmark.sh; runs with modules.fire.enabled=false are skipped), the landscape
means per species are averaged over years 280-300 from the stand output: stems/ha, basal area
(m2/ha), volume (m3/ha) and LAI, area-weighted over resource units. Each build is summarised as
mean +- standard deviation over its runs (seeds). The number of fires and the burned area per
run are listed too, since a large fire changes the forest (and the run time) a lot.

iLand is stochastic, and multithreaded runs are not reproducible even with a fixed seed, so
"the same" means: the differences between builds are within the run-to-run spread.
"st_upstream" runs (upstream with system.settings.multithreading=false, made with
run_benchmark.sh ILANDC CPCRW_like.xml st_upstream 300 1 SEED system.settings.multithreading=false
and the CPCRW settings of run_matrix.sh) are the
reference without any thread effects: single-threaded, upstream and the patched CPU build draw
exactly the same random numbers.
"""
import sqlite3
import statistics
import sys
from collections import defaultdict
from pathlib import Path

BUILDS = ["upstream", "st_upstream", "cpuopt", "gpu"]
LABELS = {"upstream": "upstream", "st_upstream": "upstream, 1 thread", "cpuopt": "CPU fixes", "gpu": "CPU fixes + GPU"}
MEASURES = [("count_ha", "stems/ha"), ("basal_area_m2", "basal area m2/ha"),
            ("volume_m3", "volume m3/ha"), ("LAI", "LAI")]


def run_means(db_path):
    con = sqlite3.connect(f"file:{db_path}?mode=ro", uri=True)
    total_area = {y: a for y, a in con.execute(
        "select year, sum(area_ha) from (select year, ru, max(area_ha) area_ha from stand "
        "where year between 280 and 300 group by year, ru) group by year")}
    out = defaultdict(dict)
    for species in [r[0] for r in con.execute("select distinct species from stand")]:
        for column, _ in MEASURES:
            per_year = [s / total_area[y] for y, s in con.execute(
                f"select year, sum({column} * area_ha) from stand where species = ? and year between 280 and 300 "
                "group by year", (species,))]
            out[species][column] = statistics.mean(per_year) if per_year else 0.0
    return out


def fire_stats(db_path):
    con = sqlite3.connect(f"file:{db_path}?mode=ro", uri=True)
    if not con.execute("select 1 from sqlite_master where name = 'fire'").fetchone():
        return 0, 0.0, 0   # fire module switched off
    areas = [a / 10000. for (a,) in con.execute("select area_m2 from fire")]
    return len(areas), sum(areas), sum(1 for a in areas if a > 100)


def main(project, use_fire_off=False):
    runs = defaultdict(list)
    fires = defaultdict(list)
    for folder in sorted(Path(project, "output").glob("*_y300_*")):
        build = folder.name.split("_y300_")[0]
        db = folder / f"{folder.name}.sqlite"
        log = folder / "log" / "log.txt"
        stdout = folder / "stdout.txt"
        fire_off = stdout.exists() and "'modules.fire.enabled' to value 'false'" in stdout.read_text(errors="replace")
        if build in BUILDS and fire_off == use_fire_off and db.exists() and log.exists() and "model run finished" in log.read_text(errors="replace"):
            runs[build].append(run_means(db))
            fires[build].append(fire_stats(db))
    species = sorted({s for b in runs for r in runs[b] for s in r})
    print("| Build | Runs | Fires per run | Burned area per run (median) | Runs with a fire > 100 ha |")
    print("|---|---|---|---|---|")
    for b in BUILDS:
        f = fires[b]
        if f:
            print(f"| {LABELS[b]} | {len(f)} | {statistics.mean(n for n, _, _ in f):.2f} | "
                  f"{statistics.median(a for _, a, _ in f):.0f} ha | {sum(1 for _, _, big in f if big)} |")
    print()
    print("| Species | Measure | " + " | ".join(LABELS[b] for b in BUILDS) + " |")
    print("|---|---|" + "---|" * len(BUILDS))
    for sp in species:
        for column, label in MEASURES:
            cells = []
            for b in BUILDS:
                values = [r.get(sp, {}).get(column, 0.0) for r in runs[b]]
                if len(values) > 1:
                    cells.append(f"{statistics.mean(values):.3g} ± {statistics.stdev(values):.2g}")
                elif values:
                    cells.append(f"{values[0]:.3g}")
                else:
                    cells.append("")
            print(f"| {sp} | {label} | " + " | ".join(cells) + " |")


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    main(sys.argv[1], "--fire-off" in sys.argv[2:])
