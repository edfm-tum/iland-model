#!/bin/bash
# The CPCRW-like benchmark matrix behind the README tables.
#
# Usage: run_matrix.sh BIN_DIR PROJECT_XML [key=value ...]
#   BIN_DIR      holds ilandc_upstream (unmodified iLand), ilandc_cpuopt (this fork, CPU build)
#                and ilandc_gpu (this fork, GPU build)
#   PROJECT_XML  CPCRW_like.xml made by make_cpcrw_like.py
#   key=value    settings for every run; the README's timing tables add modules.fire.enabled=false
#   RESULTS_CSV / THROUGHPUT_CSV choose the output files.
#
# Every run uses the command-line settings of the CPCRW HPC script (fire return interval 120,
# epsilon 2.7, sapling-detail minimum dbh 2.5).
# 1. 300 years (the CPCRW spin-up) with all cores, seeds 1-3, the three builds interleaved
# 2. thread scaling, 150 years: 4, 8 and all threads for each build, plus 1 thread (CPU build)
# 3. throughput, 150 years: 4 runs x (cores/4) threads side by side vs. 1 run x all threads
set -euo pipefail
bin=$1; xml=$2
shift 2
here=$(cd "$(dirname "$0")" && pwd)
cores=$(nproc)
cpcrw=(modules.fire.fireReturnInterval=120 model.settings.epsilon=2.7 output.saplingdetail.minDbh=2.5 "$@")

for seed in 1 2 3; do
    for build in upstream cpuopt gpu; do
        "${here}/run_benchmark.sh" "${bin}/ilandc_${build}" "$xml" "$build" 300 -1 "$seed" "${cpcrw[@]}"
    done
done
for threads in 4 8 -1; do
    for build in upstream cpuopt gpu; do
        "${here}/run_benchmark.sh" "${bin}/ilandc_${build}" "$xml" "${build}_scaling" 150 "$threads" 1 "${cpcrw[@]}"
    done
done
"${here}/run_benchmark.sh" "${bin}/ilandc_cpuopt" "$xml" cpuopt_scaling 150 1 1 "${cpcrw[@]}"
for build in upstream cpuopt gpu; do
    "${here}/run_throughput.sh" "${bin}/ilandc_${build}" "$xml" "$build" 150 1 "$cores" "${cpcrw[@]}"
    "${here}/run_throughput.sh" "${bin}/ilandc_${build}" "$xml" "$build" 150 4 $((cores / 4)) "${cpcrw[@]}"
done
