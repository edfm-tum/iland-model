# iLand Testing Framework

iLand uses a two-tier local test setup:

1. **Tier 1 (C++ Unit & Component Tests)**: `src/tests/` (`iland_tests` binary using QtTest) — fast sub-second tests covering `Expression`, `Grid`, `XmlHelper`, `DataStats`, `RandomStamps`, core classes, and the JS scripting engine.
2. **Tier 2 (End-to-End Simulation Tests & SQLite Comparator)**: `tests/` — runs headless `ilandc` on lightweight landscapes (`tests/fixtures/bgd_mini/`), checks SQLite outputs against invariant assertions and CSV reference baselines, and verifies determinism.

## Quick Start

```bash
# Run all Tier 2 simulation tests
python3 tests/run_tests.py

# Run Tier 1 C++ unit tests + Tier 2 simulation tests with detailed output
python3 tests/run_tests.py --with-unit -v

# List available simulation suites
python3 tests/run_tests.py --list

# Run a single suite by name/substring filter
python3 tests/run_tests.py abe_management -v
```

## Directory Layout

- `tests/run_tests.py` — Tier 2 runner and SQLite-vs-CSV comparator (uses Python 3 stdlib + `PyYAML`). Runs every test in an isolated temporary output directory so fixtures stay untouched.
- `tests/fixtures/bgd_mini/` — 8 ha (`400 x 200` m) deterministic Berchtesgaden/Endstal fixture with 8 heterogeneous ResourceUnits, 3 climate zones, 6 irregular polygon stands (`0.58`–`1.95` ha), single-tree + sapling initialization, and optional ABE scripts.
- `tests/simulations/*.yaml` — Declarative simulation test specs (`core_baseline`, `climate_warming`, `abe_management`, `wind`, `fire`, `barkbeetle`, `bb_wind`, `browsing`, `permafrost`, `microclimate`, `svd`, `external_seeds`, `deadtree`, `snapshot`).
- `tests/simulations/expected/<test_id>/*.csv` — Committed, human-readable CSV reference tables generated from SQLite queries.

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
