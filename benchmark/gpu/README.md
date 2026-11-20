# GPU and multithreading benchmark

The scripts and results behind the [GPU section of the README](../../README.md#gpu-build-and-performance).

| File | What it is |
|---|---|
| `make_cpcrw_like.py` | Builds the CPCRW-like test project from the [iLand demo](https://iland-model.org/downloads/example_bgd.zip). It takes the project file and workflow script of [iLand-NA-parallel](https://github.com/QuinnAsena/iLand-NA-parallel) and the demo's landscape, soils, climate (-7 °C) and Alaskan species parameters. The docstring lists every change. |
| `run_benchmark.sh` | Runs one simulation (any project file) in its own output folder and appends timings from the wall clock and iLand's timers to a CSV. |
| `run_throughput.sh` | Runs N copies side by side; records runs per hour. |
| `run_matrix.sh` | The CPCRW-like matrix: 300 years x 3 builds x 3 seeds, thread scaling, throughput. |
| `summarise.py` | Prints the README tables from the CSVs. |
| `compare_outputs.py` | Compares the simulated forests (landscape means over years 280–300) and fire statistics between builds. |
| `results_cpcrw_like.csv`, `throughput_cpcrw_like.csv` | CPCRW-like runs with the fire module off (the timing tables). |
| `results_cpcrw_like_with_fire.csv`, `throughput_cpcrw_like_with_fire.csv` | The same with fire on. Four runs are marked `disturbed`: a build ran on the same machine at the time. In these runs the patched builds did not yet have the `DebugTimer` fix, which changes no simulation results; one GPU run crashed on that race. |
| `results_cpcrw_like_upstream_single_thread.csv` | Upstream with `system.settings.multithreading=false`: the reference without thread effects for the forest comparison. |
| `results_demo.csv` | The official demo project (`example_bgd`), 50 years. |

Builds compared:
- `upstream`: iLand at commit `4dd4e6f`, plus the console `debugOutput` commit, which only
  affects logging.
- `cpuopt`: this fork built without CUDA.
- `gpu`: this fork with `CONFIG+=cuda`.

Machine: a 28-vCPU Intel Xeon (Cascade Lake) VM, 93 GB RAM, NVIDIA A100 80GB PCIe. Other
heavy jobs on the machine were paused during the timing runs.

## Reproduce

```bash
curl -O https://iland-model.org/downloads/example_bgd.zip && unzip example_bgd.zip   # 715 MB
python3 benchmark/gpu/make_cpcrw_like.py example_bgd cpcrw_like
# bin/: ilandc_upstream (unmodified iLand), ilandc_cpuopt and ilandc_gpu (build/cpu, build/gpu)
RESULTS_CSV=$PWD/results.csv THROUGHPUT_CSV=$PWD/throughput.csv \
    benchmark/gpu/run_matrix.sh bin cpcrw_like/CPCRW_like.xml modules.fire.enabled=false
python3 benchmark/gpu/summarise.py results.csv throughput.csv
python3 benchmark/gpu/compare_outputs.py cpcrw_like --fire-off

# the official demo
for b in upstream cpuopt gpu; do
    RESULTS_CSV=$PWD/demo.csv benchmark/gpu/run_benchmark.sh bin/ilandc_$b example_bgd/projectFile_bgd_baseline.xml $b 50 -1 1
done
```
