#!/bin/bash
# Run one iLand benchmark and append its timings to a CSV file.
#
# Usage:
#   run_benchmark.sh ILANDC PROJECT_XML TAG YEARS THREADS [SEED] [key=value ...]
#
#   ILANDC       ilandc executable (e.g. build/cpu/ilandc or build/gpu/ilandc, see src/gpu/README.md)
#   PROJECT_XML  iLand project file
#   TAG          label for the build/setting, e.g. upstream, cpuopt, gpu
#   YEARS        simulation years
#   THREADS      system.settings.threadCount (-1 = all cores)
#   SEED         system.settings.randomSeed (default 0 = a new seed every run)
#   key=value    further project-file settings, passed to ilandc as given
#
# Every run writes to its own folder PROJECT_DIR/output/TAG_yYEARS_tTHREADS_sSEED_<time>/ (the first
# <output>...</output> path of the project file is replaced in a temporary copy; with
# crownkill/, nFire/ and log/ subfolders, which the CPCRW workflow script expects). Timings
# come from the wall clock and from iLand's own timers in the log file and are appended to
# results.csv next to this script, or to $RESULTS_CSV if set.
set -euo pipefail
exe=$1; xml=$(cd "$(dirname "$2")" && pwd)/$(basename "$2"); tag=$3; years=$4; threads=$5; seed=${6:-0}
shift $(( $# < 6 ? $# : 6 ))
extra=("$@")

here=$(cd "$(dirname "$0")" && pwd)
project=$(dirname "$xml")
run_id="${tag}_y${years}_t${threads}_s${seed}_$(date +%Y%m%d%H%M%S)"
out="${project}/output/${run_id}"
mkdir -p "${out}/crownkill" "${out}/nFire" "${out}/log"

# iLand resolves paths when it loads the project, so the output folder is set in a copy of the
# project file (as scripts/run_iland_csv_cpxml.sh in iLand-NA-parallel does)
run_xml="${project}/${run_id}.xml"
sed "0,/<output>[^<]*<\/output>/s||<output>${out}</output>|" "$xml" > "$run_xml"

start=$(date +%s.%N)
/usr/bin/time -f "%M" -o "${out}/maxrss_kb.txt" "$exe" "$run_xml" "$years" \
    system.database.out="${run_id}.sqlite" \
    system.logging.logTarget=file \
    system.logging.logFile="${out}/log/log.txt" \
    system.settings.threadCount="$threads" \
    system.settings.randomSeed="$seed" \
    ${extra[@]+"${extra[@]}"} > "${out}/stdout.txt" 2>&1 || true
end=$(date +%s.%N)
rm -f "$run_xml"

python3 - "$out" "$tag" "$exe" "$years" "$threads" "$seed" "$start" "$end" "${RESULTS_CSV:-${here}/results.csv}" "$(basename "$xml")" "${extra[*]:-}" <<'EOF'
import csv, os, platform, re, sys, datetime
out, tag, exe, years, threads, seed, start, end, results, project, extra = sys.argv[1:12]
log = open(os.path.join(out, "log", "log.txt"), errors="replace").read() if os.path.exists(os.path.join(out, "log", "log.txt")) else ""

def seconds(name):
    # iLand prints cumulative timers as "name" : "1m 2.5s" / "12.3s" / "450ms"
    m = re.findall(r'"%s" : "([^"]+)"' % re.escape(name), log)
    if not m:
        return ""
    total = 0.0
    for part in m[-1].split():
        if part.endswith("ms"): total += float(part[:-2]) / 1000
        elif part.endswith("s"): total += float(part[:-1])
        elif part.endswith("m"): total += 60 * float(part[:-1])
        elif part.endswith("h"): total += 3600 * float(part[:-1])
    return round(total, 3)

failed = "!!!! ERROR" in log or "model run finished" not in log
row = {
    "tag": tag, "project": project, "years": years, "threads": threads, "seed": seed,
    "wall_s": round(float(end) - float(start), 2),
    "model_s": seconds("Model::runYear()"),
    "light_apply_s": seconds("applyPattern()"), "light_read_s": seconds("readPattern()"),
    "tree_growth_s": seconds("growTrees()"), "production_s": seconds("growRU()"),
    "seed_dispersal_s": seconds("seed dispersal (all species)"),
    "establishment_s": seconds("establishment"), "sapling_growth_s": seconds("sapling growth"),
    "modules_s": seconds("modules"), "outputs_s": seconds("outputs"), "management_s": seconds("management"),
    "max_rss_mb": round(int(open(os.path.join(out, "maxrss_kb.txt")).read().split()[-1]) / 1024)
                  if os.path.exists(os.path.join(out, "maxrss_kb.txt")) else "",
    "status": "failed" if failed else "ok",
    "extra": extra, "executable": os.path.basename(exe),
    "cpu": next((l.split(":", 1)[1].strip() for l in open("/proc/cpuinfo") if l.startswith("model name")), platform.processor()),
    "cores": os.cpu_count(), "date": datetime.date.today().isoformat(),
}
new = not os.path.exists(results)
with open(results, "a", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(row))
    if new:
        w.writeheader()
    w.writerow(row)
print(f"{tag} {project} years={years} threads={threads} seed={seed}: wall {row['wall_s']} s, model {row['model_s']} s "
      f"[{row['status']}]")
EOF
