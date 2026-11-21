# iLand on the GPU

This fork can run two parts of every simulated year on an NVIDIA GPU: light competition and
seed dispersal. It also fixes three thread-safety problems that slow down (and occasionally
crash) multithreaded runs on the CPU. Results are in the main [README](../../README.md#gpu-build-and-performance);
the benchmark scripts are in [`benchmark/gpu/`](../../benchmark/gpu/).

| Commit | What it changes |
|---|---|
| `ilandc: apply system.settings.debugOutput` | The console version applies `system.settings.debugOutput`, as the GUI does. Without it, `ilandc` never writes the per-year performance table (512). |
| `Fix three thread-safety problems` | CPU only: (1) Every thread incremented one global random-number index, so threads queued for it. Now the main thread keeps upstream's sequence and every other thread has its own Mersenne Twister. (2) A debug counter written by every seed-dispersal thread is compiled out of the console build. (3) `DebugTimer` looked up its global table without a lock, which crashed one run in about 45. Single-threaded runs give exactly the same results as before. |
| `Optional CUDA path` | `qmake CONFIG+=cuda` builds `ilandc` with `src/gpu/iland_gpu.cu`. |

## Build

```bash
# Qt 6.8 + FreeImage via pixi (or your own Qt 6.5-6.8); nvcc from the system (e.g. module load cuda)
pixi run --manifest-path src/gpu/pixi.toml src/gpu/build_ilandc.sh
# -> build/cpu/ilandc and, if nvcc is found, build/gpu/ilandc
```

Or by hand, in a build folder:
`qmake6 src/ilandc/ilandc.pro CONFIG+=cuda [CUDA_DIR=...] [CUDA_ARCH="-arch=sm_80"]` (build
the plugins first, as `build_iland.sh` does).

- **Architectures:** builds for all major GPU generations by default; set `CUDA_ARCH` to
  compile for one only.
- **Runtime libraries:** the CUDA runtime is linked statically, so compute nodes need only
  the NVIDIA driver.
- **Console version only:** only `ilandc` has the CUDA option.
- **Qt version:** Qt 6.12 is too new for iLand (moc fails on `bite/biteagent.h`).

## Run

Use the GPU build like any `ilandc`; project files and command-line settings are unchanged.

- **Device selection:** with a CUDA device present, it runs light competition and seed
  dispersal there, and prints the device at the start of the log.
- **Fallback:** without a device, or with the environment variable `ILAND_GPU=0`, it runs
  everything on the CPU. On a CUDA error it logs a warning and continues on the CPU.
- **Several GPUs:** choose one per process with `CUDA_VISIBLE_DEVICES`.
- **Sharing one GPU:** several runs can share a GPU. A 300-year run of the 5.5 x 7.5 km demo
  landscape used at most 490 MiB of GPU memory, mostly the CUDA context; the 2 m light grid
  adds about 4 bytes per cell, so a 25 x 25 km landscape needs roughly 1.2 GB.
- **Torus mode** stays on the CPU.

## What runs on the GPU

- **Light competition** (`Model::applyPattern`, `Model::readPattern`; `core/gpulight.cpp`,
  `gpu/iland_gpu.cu`), every year:
  - initialise the 2 m light (LIF) grid;
  - build the 10 m height grid (tree counts, dominant and stem heights, using atomics);
  - multiply every tree's light influence pattern into the grid (atomic multiply);
  - compute each tree's raw light sum (`Tree::lightSum()`).

  The species-specific LRI correction (a project-file expression) and the resource-unit sums
  stay on the CPU (`Tree::setLightResourceIndex()`). Both grids are copied back every year
  for establishment, sapling growth and outputs.
- **Seed dispersal** (`SeedDispersal::distributeSeeds`), per species and year: the kernel
  convolution, long-distance dispersal (using Philox random numbers seeded from iLand's
  generator) and the final scaling.

Everything else stays on the CPU: tree growth, 3-PG, water and permafrost, establishment,
sapling growth, disturbance modules and outputs. That's branchy per-tree or per-cell code that
creates and removes trees, which doesn't map well onto a GPU.

## How close GPU results are to the CPU

**Light competition.** Run with `ILAND_GPU_VALIDATE=1` and every year the light competition
is recomputed on the CPU from the same trees and compared (slow; for testing). In a 150-year
CPCRW-like run:

| Year (trees) | GPU vs CPU, same order: light grid, max abs. difference | Raw light sum, max rel. difference | Height grid | Upstream order vs same order: raw light sum, max rel. difference |
|---|---|---|---|---|
| 28 (21,014) | 2.4e-7 | 2.5e-7 | identical | 0.21 |
| 88 (114,305) | 3.0e-7 | 2.9e-7 | identical | 0.82 |
| 148 (280,764) | 4.2e-7 | 4.7e-7 | identical | 1.28 |

- **GPU vs CPU:** the GPU matches the CPU to float rounding. It's compiled without fused
  multiply-add (`--fmad=false`), and the remaining differences come from the order of
  multiplications and sums.
- **What really changes the numbers is the order of the CPU code.** `nc_applyPattern` builds
  the height grid one resource unit at a time, interleaved with applying the patterns, so
  trees near a unit's edge are shaded against neighbours that haven't entered their heights
  yet. For some trees that changes the light sum by up to 128%. The GPU completes the height
  grid first. In multithreaded CPU runs the interleaving also depends on thread timing.

**Seed dispersal.** The GPU uses its own random numbers for long-distance dispersal, so GPU
runs are statistically equivalent to CPU runs, not bit-identical (multithreaded CPU runs are
not reproducible either). `benchmark/gpu/compare_outputs.py` compares the simulated forests;
see the main README.
