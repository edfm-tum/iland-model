# `bgd_mini` Test Landscape Fixture (8 ha)

Lightweight, deterministic 8 ha (`400 x 200` m, 8 Resource Units of `100 x 100` m, `100` m buffer) iLand test landscape derived from the Berchtesgaden (Endstal) parameterization.

## Resource Unit Grid (`gis/environment.txt`, `id` 1–8)

| RU (`ix, iy`) | `id` | Climate (`climate.sqlite`) | Soil Depth (cm) | Texture (S/Si/C %) | Avail. N (kg/ha/yr) | SOM C (kg/ha) | Overstory Init |
| :--- | :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| `(0, 0)` | `1` | `climate_valley` | `75` | `24 / 40 / 36` | `75.8` | `95200` | `init_trees.csv` (415 trees) |
| `(1, 0)` | `2` | `climate_valley` | `15` | `65 / 25 / 10` | `38.5` | `32000` | `init_trees.csv` (415 trees) |
| `(2, 0)` | `3` | `climate_valley` | `45` | `35 / 45 / 20` | `58.0` | `68000` | *none* (sapling gap) |
| `(3, 0)` | `4` | `climate_montane` | `55` | `50 / 32 / 18` | `52.4` | `78000` | `init_trees.csv` (415 trees) |
| `(0, 1)` | `5` | `climate_montane` | `12` | `44 / 35 / 21` | `36.2` | `44000` | `init_trees.csv` (415 trees) |
| `(1, 1)` | `6` | `climate_subalpine` | `35` | `60 / 27 / 13` | `40.4` | `115000` | `init_trees.csv` (415 trees) |
| `(2, 1)` | `7` | `climate_subalpine` | `8` | `56 / 29 / 15` | `33.9` | `52000` | `init_trees.csv` (415 trees) |
| `(3, 1)` | `8` | `climate_subalpine` | `30` | `44 / 35 / 21` | `42.0` | `85000` | *none* (sapling gap) |

## Irregular Management Stands (`gis/stand_grid.asc`, `stand_id` 1–6)

Six contiguous, curved polygon stands (`0.58`–`1.95` ha, summing to `8.00` ha) intentionally crossing multiple Resource Unit boundaries for testing ABE forest management and sapling cohort initialization:

| `stand_id` | Area (ha) | Resource Units Crossed | ABE STP (`abe/abe_agents.csv`) | Role |
| :---: | :---: | :--- | :--- | :--- |
| `1` | `1.55` | RUs `1, 2, 5` | `stp_thin` (`age = 60`) | SW stand (thinned in Year 2) |
| `2` | `1.95` | RUs `1, 2, 5, 6` | `stp_thin` (`age = 61`) | West-central stand (thinned in Year 1) |
| `3` | `0.88` | RUs `2, 3, 7` | `stp_regen` (`age = 8`) | Valley sapling stand (`init_saplings.csv`, planted in Year 2) |
| `4` | `1.77` | RUs `2, 3, 4, 6, 7, 8` | `stp_thin` (`age = 98`) | SE mature stand (final harvest in Year 2) |
| `5` | `1.27` | RUs `6, 7, 8` | `stp_thin` (`age = 45`) | North subalpine control stand (unharvested in 5-yr window) |
| `6` | `0.58` | RUs `4, 7, 8` | `stp_regen` (`age = 15`) | NE subalpine sapling pocket (`init_saplings.csv`, undisturbed) |

## Assets
- `database/species_param.sqlite`: 10 active BGD species (`abal`, `acps`, `bepe`, `fasy`, `lade`, `piab`, `pice`, `pimu`, `pisy`, `soau`) + `wind` parameters.
- `database/climate.sqlite`: 20 years (`1990–2009`) across `climate_valley`, `climate_montane`, `climate_subalpine`.
- `lip/`: `readerstamp.bin` + the 9 LIP `.bin` files needed by active species.
- `gis/`: `environment.txt` (`;` delimited, matrix mode), `environment_climate_site.txt` (climate + site columns only for `snapshot` mode), `stand_grid.asc` (10 m, 6 irregular stands), `dem.asc` (10 m covering `100` m buffer), `wind_topo.asc` (10 m).
- `init/`: `init_trees.csv` (415 Endstal trees, `id` omitted) and `init_saplings.csv` (cohorts for irregular stands `3` and `6`).
- `scripts/`: `co2_rcp45.csv` (TimeEvents CO2 pathway), `wind_y2.csv` (TimeEvents year-2 storm), `force_fire_y2.js` (`Fire.onIgnition` prescribed fire in year 2), `save_snapshot_y10.js` (year-10 snapshot export).
- `abe/`: self-contained ABE test scripts (`abe_stp.js`, `abe_agents.csv`), disabled by default (`model.management.abeEnabled=false`) in `project.xml` and enabled directly in `project_abe.xml`.

## Dedicated Scenario Project Files (`project_*.xml`)

Each XML below is standalone and GUI-ready (shares `database/`, `gis/`, `init/`, `lip/`, `abe/`, `scripts/`, writes to its own `output/bgd_mini_<scenario>.sqlite` and `log/bgd_mini_<scenario>.log`):

| Project File | Recommended Years | Tested Scenario / Submodules |
| :--- | :---: | :--- |
| `project.xml` | `5`–`10` | Unmanaged baseline (`core_baseline`) |
| `project_warming.xml` | `5` | `+2.5` °C warming + `co2_rcp45.csv` TimeEvents |
| `project_abe.xml` | `5` | ABE irregular stand management (`thinning`, `planting`, `final harvest`) |
| `project_wind.xml` | `10` | Forced storm in year 2 via `scripts/wind_y2.csv` |
| `project_fire.xml` | `10` | Forced year-2 wildfire via `scripts/force_fire_y2.js` (`onlySimulation=false`) |
| `project_barkbeetle.xml` | `10` | Standalone bark beetle spread & mortality |
| `project_bb_wind.xml` | `10` | Coupled year-2 windthrow + bark beetle outbreak |
| `project_browsing.xml` | `10` | Ungulate browsing (`browsingPressure=2`) |
| `project_permafrost.xml` | `10` | Permafrost + moss dynamics under `-5` °C cooling |
| `project_microclimate.xml` | `10` | Microclimate buffering enabled (`barkbeetle`, `decomposition`, `establishment`) |
| `project_microclimate_off.xml` | `10` | Paired control for `project_microclimate.xml` (microclimate disabled) |
| `project_svd.xml` | `10` | SVD states (`4m`) + indicators (`shannonIndex`, `IBP`, proportions, etc.) |
| `project_external_seeds.xml` | `10` | Uniform background external seed rain (`abal`, `acps`, `pisy`, `pice`) |
| `project_seedbelt.xml` | `10` | Spatially explicit West (`abal`/`acps`) vs East (`pisy`/`pice`) `seedBelt` |
| `project_deadtree.xml` | `20` | Single standing/downed dead trees (`swdDBHSingle=5` cm) + decay classes |
| `project_snapshot_save.xml` | `10` | Exports full model snapshot (`output/snapshot_y10.sqlite`) at year 10 |
| `project_snapshot_load.xml` | `5` | Resumes from `output/snapshot_y10.sqlite` (`mode=snapshot`) |

## Running
```bash
./src/ilandc/build/release/ilandc tests/fixtures/bgd_mini/project.xml 5
```
