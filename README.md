<img src="https://github.com/user-attachments/assets/dae76af8-3652-48af-97e2-cc8e4c6b2f02" width=200>

iLand - the individual-based forest landscape and disturbance model
===================================================================

This is the official GitHub repository of iLand.

iLand is a forest landscape model and simulates how individual trees grow, compete, die, and regenerate under various climate, disturbance, and management scenarios.
Spanning scales from watersheds to landscapes (10,000+ hectares), iLand projects forest dynamics over decades and centuries, estimating species composition, stand structure, carbon storage, 
and potential impacts of climate change on disturbances and ecosystem services. 

Used by a growing research community, this open-source tool (GNU GPL) is hosted at the Technical University of Munich but employed by research groups worldwide. The model
has been steadily developed over the last 15 years and has been used on in more than 40 peer reviewed research papers.

![grafik](https://github.com/edfm-tum/iland-model/assets/24245514/33856622-227d-4ede-ae3e-7272a1e4b6a1)

## Resources

* Offical home page: https://iland-model.org
* Scientifc publications: https://iland-model.org/iLand+publications
* Discord-community-channel: https://discord.gg/e5trneUKav or https://tinyurl.com/iland-discord

## Instructions for building
iLand is a C++ application using the Qt toolkit. The easiest way to build iLand is to use Qt Creator. Detailed instructions for building the model are here: https://iland-model.org/iland-book/Building_iLand.html

## GPU build and performance

`ilandc` can optionally run **light competition and seed dispersal on an NVIDIA GPU**
(`qmake CONFIG+=cuda`). Three **thread-safety fixes** also make multithreaded CPU runs faster
and remove a rare crash. Without the CUDA option, iLand builds and runs as before. Details:
[src/gpu/README.md](src/gpu/README.md).

```bash
# Qt 6.8 + FreeImage via pixi (https://pixi.sh); CUDA toolkit from the system
pixi run --manifest-path src/gpu/pixi.toml src/gpu/build_ilandc.sh
build/gpu/ilandc project.xml 300        # uses the GPU if there is one; ILAND_GPU=0 switches it off
```

**Results.** A CPCRW-like boreal spin-up of 300 years. It uses the project file and workflow
script of [iLand-NA-parallel](https://github.com/QuinnAsena/iLand-NA-parallel) (fire,
permafrost, 4 Alaskan species, starting from saplings) on the demo landscape (5.5 x 7.5 km).
It ran on a 28-vCPU Cascade Lake VM with an A100 80GB, with the fire module off for clean
timing.

| Build | Wall time | Model time | Speed-up (wall) |
|---|---|---|---|
| Upstream iLand (CPU) | 2.7 min | 2.5 min | 1.00x |
| CPU fixes | 1.8 min | 1.7 min | 1.44x |
| CPU fixes + GPU | 1.3 min | 1.2 min | 1.98x |

**The official demo project** (`example_bgd`: 32 species, mature forest, wind and bark-beetle
modules on), 50 years, same machine:

| Build | Wall time | Model time | Speed-up (wall) |
|---|---|---|---|
| Upstream iLand (CPU) | 2.2 min | 2.0 min | 1.00x |
| CPU fixes | 1.6 min | 1.4 min | 1.35x |
| CPU fixes + GPU | 1.5 min | 1.3 min | 1.45x |

The gain is smaller here because the wind and bark-beetle modules (30–39 s of each run) and
outputs (15 s) stay on the CPU. Seed dispersal drops from 19 s to 1 s, and establishment from
25 s to 5 s.

**Where the time went.** Profiling upstream with `perf`:

- **Establishment:** took 22% of the wall time but 67% of all CPU samples. Most of that was
  threads queuing for one global random-number index; with the fixes, establishment drops
  from 49 s to 10 s.
- **Seed dispersal:** used one thread per species, so 4 of 28 cores with 4 species. On the
  GPU it takes under 1 s instead of 30 s.
- **Light competition:** on the GPU it matches the CPU to float rounding, checked every year
  with `ILAND_GPU_VALIDATE=1`.
- **What remains:** tree growth, outputs (SQLite) and the disturbance modules stay on the CPU.

**On an HPC, run several replicates side by side.** iLand doesn't use many cores well:

**Thread scaling, 150 years** (model time, seconds)

| Build | 1 | 4 | 8 | all (28) |
|---|---|---|---|---|
| Upstream iLand (CPU) |  | 55.7 | 60.6 | 43.0 |
| CPU fixes | 132.5 | 45.8 | 37.7 | 32.3 |
| CPU fixes + GPU |  | 42.8 | 26.2 | 19.9 |

**Throughput, 150-year runs** (several runs side by side on one 28-core node with one GPU)

| Build | 1 run x 28 threads | 4 runs x 7 threads |
|---|---|---|
| Upstream iLand (CPU) | 61 runs/h | 204 runs/h |
| CPU fixes | 86 runs/h | 273 runs/h |
| CPU fixes + GPU | 118 runs/h | 320 runs/h |

**Same forest?** iLand is stochastic, so the check is whether differences stay within the
run-to-run spread.
- **Single-threaded:** the CPU fixes give output identical to upstream with a fixed seed.
- **Patched CPU vs GPU build:** in the CPCRW-like runs they agree within about 1% in
  stems/ha, basal area and volume.
- **Multithreaded upstream:** 2–6% lower, and it varies more between runs. That's probably an
  effect of its threads racing on the shared random-number index. It also had fewer fires than
  single-threaded upstream (0.38 vs 0.83 per run), while the patched builds had 0.8–1.0. More
  in [benchmark/gpu/](benchmark/gpu/) (`compare_outputs.py`).

**Caveats.**
- **Statistically equivalent, not bit-identical:** GPU runs use the GPU's own random numbers
  for long-distance seed dispersal.
- **Complete height grid:** the GPU builds the height grid before applying the light patterns.
  The CPU code interleaves the two by resource unit, which changes some trees' light sums by up
  to 128% (see [src/gpu/README.md](src/gpu/README.md)).
- **Torus mode** stays on the CPU.

## Contributing
We welcome contributions to iLand! 
* For standard pull request workflows, guidelines, and community links, please see the **[Contributor Guide](CONTRIBUTING.md)**.
* For contributing to the documentation website, check out the **[Documentation Contributor Guide](docs/README.md)**.

