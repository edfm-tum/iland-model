# iLand Testing Framework

iLand uses a three-tier local test setup:

1. **Tier 1 (C++ Unit & Component Tests)**: `src/tests/` (`iland_tests` binary using QtTest) — fast sub-second tests covering `Expression`, `Grid`, `XmlHelper`, `DataStats`, `RandomStamps`, core classes, and the JS scripting engine.
2. **Tier 2 (End-to-End Simulation Tests & SQLite Comparator)**: `tests/` — runs headless `ilandc` on lightweight landscapes (`tests/fixtures/bgd_mini/`), checks SQLite outputs against invariant assertions and CSV reference baselines, and verifies determinism.
3. **Tier 3 / JS API Tests (`tests/js/`)**: In-engine QJSEngine test suites (`tests/js/test_*.js` + `tests/js/harness.js`) covering `Globals`, `Grid`, `Map`, `TreeList`, `SaplingList`, `DeadTreeList`, `SpatialAnalysis`, `CSVFile`, `ClimateConverter`, `DBHDistribution`, `ABE`, `BITE`, and disturbance plugin JS APIs (`Wind`, `Fire`, `BarkBeetle`).

## Quick Start

```bash
# Run all Tier 2 simulation tests
python3 tests/run_tests.py

# Run all JS API test suites
python3 tests/run_tests.py --js -v

# Run all 3 tiers (Tier 1 C++ unit tests + Tier 2 simulations + JS API tests)
python3 tests/run_tests.py --all -v

# Run Tier 1 C++ unit tests + Tier 2 simulation tests (without JS suites)
python3 tests/run_tests.py --with-unit -v

# List available Tier 2 simulation and JS API suites
python3 tests/run_tests.py --list

# Run a single Tier 2 or JS API suite by name, filename, or substring filter
python3 tests/run_tests.py abe_management -v
python3 tests/run_tests.py --js js_abe_api -v
```

For `--all` or `--js`, pass `--no-js` to skip JS API suites when combining options. Pass `--js-manifest <path>` to point to an alternative JS suite manifest.

## Directory Layout

- `tests/run_tests.py` — Unified runner for Tier 2 simulations, SQLite-vs-CSV comparison, and JS API suites (uses Python 3 stdlib + `PyYAML`). Runs every test in an isolated temporary output directory so fixtures stay untouched.
- `tests/fixtures/bgd_mini/` — 8 ha (`400 x 200` m) deterministic Berchtesgaden/Endstal fixture with 8 heterogeneous ResourceUnits, 3 climate zones, 6 irregular polygon stands (`0.58`–`1.95` ha), single-tree + sapling initialization, and optional ABE scripts.
- `tests/simulations/*.yaml` — Declarative simulation test specs (`core_baseline`, `climate_warming`, `abe_management`, `wind`, `fire`, `barkbeetle`, `bb_wind`, `browsing`, `permafrost`, `microclimate`, `svd`, `external_seeds`, `deadtree`, `snapshot`).
- `tests/simulations/expected/<test_id>/*.csv` — Committed, human-readable CSV reference tables generated from SQLite queries.
- `tests/js/` — In-engine JS API test harness (`harness.js`), declarative manifest (`js_suites.yaml`), helper scripts (`bite_test_agent.js`), and test suites (`test_*.js`).

## Updating Baselines After Expected Model Changes

When a bug fix, logic update, or RNG change alters numeric outputs by an expected amount:

```bash
# Regenerate baselines (prints row/column delta summary before writing)
python3 tests/run_tests.py --update-baseline

# Or update a single suite only:
python3 tests/run_tests.py --update-baseline core_baseline

# Inspect exact changes cleanly in git:
git diff tests/simulations/expected/
```

## How to Add New Runs or Evaluations

### 1. Add an assertion or SQLite baseline to an existing YAML file

Open any `tests/simulations/*.yaml` file and append to `assertions:` (ecological/structural invariants evaluated with Python expressions over `rows`, `row`, and `n`) or `baselines:` (deterministic SQL query with `ORDER BY` checked against `expected/<id>/<name>.csv`):

```yaml
assertions:
  - name: spruce_present_at_y5
    query: "SELECT COUNT(*) AS c, SUM(volume_m3) AS vol FROM landscape WHERE year = 5 AND species = 'piab'"
    expect: "row['c'] == 1 and row['vol'] > 50"

baselines:
  - name: carbonflow_y5
    rel_tol: 1.0e-4   # optional per-table override
    abs_tol: 1.0e-3
    column_tolerances:
      NEP: { rel_tol: 1.0e-2, abs_tol: 1.0 }
    query: |
      SELECT * FROM carbonflow WHERE year = 5 ORDER BY rid
```

Then generate/update its CSV baseline and run:

```bash
python3 tests/run_tests.py --update-baseline <test_id>
python3 tests/run_tests.py <test_id> -v
```

### 2. Add a completely new simulation test run

Create a new file `tests/simulations/my_new_test.yaml` (or add an entry under `tests:` in an existing YAML file) overriding `project.xml` keys via `params:`:

```yaml
id: wind_and_beetle_smoke
description: Test 5-year wind + bark beetle activation via CLI overrides.
project: ../fixtures/bgd_mini/project.xml
years: 5
check_determinism: true
rel_tol: 1.0e-4

params:
  modules.wind.enabled: true
  modules.barkbeetle.enabled: true
  output.wind.enabled: true
  output.barkbeetle.enabled: true

assertions:
  - name: outputs_created
    query: "SELECT COUNT(*) AS n_rows FROM barkbeetle"
    expect: "row['n_rows'] == 5"

baselines:
  - name: landscape
    query: "SELECT * FROM landscape ORDER BY year, species"
```

Run `--update-baseline my_new_test` once to create `tests/simulations/expected/wind_and_beetle_smoke/*.csv`, then `python3 tests/run_tests.py` automatically discovers and executes it.

### 3. Cross-Scenario Comparisons (`compare_with` & `{ref_dir}`)

To assert paired differences against another test (e.g., `bb_wind` vs `barkbeetle`, `microclimate_on` vs `microclimate_off`, or `snapshot_load` vs `snapshot_save`), set `compare_with: <other_test_id>` in the dependent test spec:

- **Automatic scheduling**: Filtering to the dependent test (e.g., `python3 tests/run_tests.py bb_wind`) automatically schedules its `compare_with` prerequisite first.
- **SQL `ref` schema**: The prerequisite's `output.sqlite` is attached as SQLite schema `ref`, so assertions can join or compare `main.<table>` vs `ref.<table>`.
- **Artifact reuse (`{ref_dir}`)**: Any `params:` value containing `{ref_dir}` resolves to the prerequisite's run directory (used by `snapshot_load` to pass `model.initialization.file: "{ref_dir}/snapshot_y10.sqlite"`).

## JavaScript API Tests (`tests/js/`)

Each JS suite is a normal iLand script executed inside the full `QJSEngine` runtime (either via headless `ilandc` managed by `tests/run_tests.py` or interactively inside the iLand GUI).

### Structure

- **`tests/js/harness.js`**: Lightweight ES6 test runner (`Test.suite`, `Test.add`, `Test.only`, `Test.skip`, `Test.run`) and assertions (`assert.ok`, `assert.eq`, `assert.neq`, `assert.approx`, `assert.gt`, `assert.gte`, `assert.lt`, `assert.lte`, `assert.contains`, `assert.throws`). Writes structured results to `Globals.defaultDirectory('temp') + 'js_test_results.json'` and throws an `Error` if any test fails or zero tests ran.
- **`tests/js/js_suites.yaml`**: Manifest defining each suite's `id`, `script`, `project` XML, simulation `years` (`0` skips year stepping and runs `onAfterCreate` + `onFinish` only), XML parameter overrides (`params`), optional `assertions` (e.g., SQLite checks), and `skip_determinism` flag.
- **Test suites (`tests/js/test_*.js`)**:
  - `js_globals_io` (`test_globals_io.js`, `years: 0`) — `Globals`, directory paths, XML settings read/write, `CSVFile`, `ClimateConverter`, `DBHDistribution`.
  - `js_grids_maps` (`test_grids_maps.js`, `years: 0`) — `Grid` creation/algebra/resampling/ASCII I/O and `Map` spatial index queries.
  - `js_vegetation_lists` (`test_vegetation_lists.js`, `years: 0`) — `TreeList`, `SaplingList`, `DeadTreeList`, `ResourceUnit` accessors and filtering.
  - `js_spatial` (`test_spatial.js`, `years: 0`) — `SpatialAnalysis` patch/distance/neighborhood calculations.
  - `js_lifecycle_dead` (`test_lifecycle_dead.js`, `years: 6`) — multi-year tree, sapling, and standing/lying deadwood script callbacks (`onYearEnd`, etc.).
  - `js_abe_api` (`test_abe_api.js`, `years: 2`) — ABE `fmengine`, stands, activities, `Patches`, and management lists.
  - `js_bite_api` (`test_bite_api.js`, `years: 2`) — BITE agent/cell scripting and SQLite output verification.
  - `js_disturbance_api` (`test_disturbance_api.js`, `years: 2`) — `Wind`, `Fire`, and `BarkBeetle` JS scripting APIs.

### Adding or Editing a JS Test

1. Open an existing `tests/js/test_<suite>.js` (or create a new file) and register checks inside `Test.add("case name", function() { ... });`.
2. Note that top-level script code runs during `Model::loadProject()` before `setupGlobalScripting()`, so top-level code (including loading the harness) should use `Globals.include(...)` / `Globals.print(...)`.
3. If creating a new suite file, add a corresponding entry to `tests/js/js_suites.yaml` specifying `id`, `script`, `project`, `years`, and any `params` or `assertions`.
4. Run `python3 tests/run_tests.py --js <suite_or_file> -v` to verify.

### Running Interactively in the iLand GUI

1. Launch `iland` and open the suite's project XML from `tests/fixtures/bgd_mini/` (e.g., `project.xml`, `project_abe.xml`, `project_deadtree.xml`, or `project_fire.xml`).
2. In the **Scripting** panel, load `tests/js/test_<suite>.js` (or set `system.javascript.fileName` in the project settings and reload).
3. For `years: 0` suites (`js_globals_io`, `js_grids_maps`, `js_vegetation_lists`, `js_spatial`), initialize/create the landscape and invoke `onAfterCreate()` / `onFinish()` (or run 1 year). For multi-year suites (`js_lifecycle_dead`, `js_abe_api`, `js_bite_api`, `js_disturbance_api`), run the configured number of years; `Test.run()` prints pass/fail details directly to the GUI message log and writes `js_test_results.json` in the project's temp directory.
