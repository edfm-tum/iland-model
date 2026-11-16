# Customizing iLand output visualizations (`visualizations.json`)

The **Output visualization** tab in the iLand Viewer reads its chart and spatial map definitions from JSONC metadata files (`visualizations.json`, JSON with `//` and `/* ... */` comments).

---

## 1. File locations & override priority

iLand loads and merges definitions in three tiers (matching entries with the same `"id"` overwrite earlier tiers):

1. **Built-in defaults** (compiled into the executable as `:/visualizations.json`).
2. **Global user file**: `<iland-executable-dir>/visualizations.json` — applies to all projects opened with this iLand executable.
3. **Project user file**: `<project-xml-dir>/visualizations.json` (and `system.path.home/visualizations.json` if different) — applies to that specific project.

Click **Reload** in the **Output options** dock (on the right when **Output visualization** is active) after editing a file; iLand re-reads tiers 2 and 3 immediately without restarting.

To disable a built-in entry in your user file, override its `"id"` with `"enabled": false`:

```jsonc
{
  "version": 1,
  "visualizations": [
    { "id": "nitrogen_pools_landscape", "enabled": false }
  ]
}
```

---

## 2. Schema reference

Every file has the top-level object:

```jsonc
{
  "version": 1,
  "visualizations": [
    { /* VizSpec object */ }
  ]
}
```

### Common fields

| Key | Type | Description |
| :--- | :--- | :--- |
| `id` | string *(required)* | Unique identifier. Matching an existing `id` replaces that visualization. |
| `name` | string *(required)* | Display title shown in the catalog tree, legend, and chart title. |
| `category` | string | Grouping folder in the **Output options** catalog (default `"Custom"`). |
| `description` | string | Short description shown below the chart and as tree tooltip. |
| `table` | string | Primary SQLite table name (used for *"Active outputs only"* filtering and availability hints). Leave empty for multi-table JOINs. |
| `type` | string *(required)* | `"line"`, `"stacked_area"`, `"bar"`, `"stacked_bar"`, `"boxplot"`, or `"spatial"`. |
| `enabled` | boolean | Default `true`. Set `false` to hide an overridden built-in visualization. |
| `sql` | string or string[] *(required)* | SQLite `SELECT` statement. An array of strings is joined with newlines for readability. |

### Axis, grouping & metrics (for chart types)

| Key | Type | Description |
| :--- | :--- | :--- |
| `x` | string | X-axis column name (default `"year"`). For `boxplot`, categorical X column (e.g. `"month"`). |
| `xLabel` / `yLabel` | string | Optional axis titles (defaults to `x` / active metric `label`). |
| `xCategories` | string[] | Optional category names (e.g. `["Jan","Feb",...,"Dec"]` for `month` 1..12). |
| `group` | string | Categorical column splitting rows into multiple series (e.g. `"species"`, `"reason"`, `"stage"`). |
| `colorMode` | string | `"species"` (uses iLand species `displayColor` + full name), or default Brewer qualitative palette. Auto-enabled when `group == "species"`. |
| `groupLabels` | object | Optional `{ "code": "Readable label" }` map for group names and filter dropdowns. |
| `groupColors` | object | Optional `{ "code": "#rrggbb" }` color map per group. |
| `metricsMode` | string | `"select"` (metric dropdown; default) or `"all"` (plots all listed `metrics` simultaneously as separate series). |
| `metrics` | array | Objects `{ "col": "...", "label": "...", "color": "#...", "agg": "sum"|"none" }`. |
| `filters` | string[] | Interactive filter dropdowns: e.g. `["ru"]`, `["species", "ru"]`, `["id"]`, `["agent"]`, `["reason"]`. |
| `allowPercentToggle` | boolean | Adds a **Normalize to 100%** checkbox for `"stacked_area"` and `"stacked_bar"`. |
| `fillMissingYears` | boolean | Fills missing integer years `1..maxYear` with `0` (useful for sparse disturbance tables like `fire`, `wind`). |

### Named SQL placeholders (`:param`)

Filter columns listed in `"filters"` are bound automatically as `:filterName` parameters (e.g. `:ru`, `:species`, `:id`, `:agent`, `:reason`):

- **Recommended:** reference them directly in your SQL `WHERE` clause (e.g., `WHERE ru = :ru AND species = :species`).
- **Auto-wrapping:** if you omit `:filterName` from `sql`, iLand wraps your query as `SELECT * FROM (<sql>) WHERE <filterName> = :<filterName>`.
- **RU dropdown auto-hide:** if `"filters"` includes `"ru"`, and the database table only contains landscape aggregate rows (`ru = -1`), iLand automatically hides the RU dropdown.

### Spatial map fields (`"type": "spatial"`)

| Key | Type | Description |
| :--- | :--- | :--- |
| `spatialKey` | string | `"rid"` (environment ID, default) or `"ru"` (0-based RU index). Your SQL should select both `ru` and `rid` if available. |
| `palette` | string | Default color ramp: `"turbo"`, `"greens"`, `"rainbow"`, `"blues"`, `"reds"`, `"heat"`, `"gray"`. |
| `speciesMode` | string | `""` (no species filter), `"select"` (per-species), `"select_with_aggregates"` (adds `<Total (all species)>`), or `"dominant"` (dedicated dominant-species map). |
| `metrics[].agg` | string | `"sum"` (default, enables `<Total (all species)>`) or `"none"` (disables `<Total>` for non-additive metrics like mean DBH). |

For spatial queries, select `year, ru, rid, ...` without hardcoding `WHERE year = :year` if you want fast `Global (all years)` color range calculation; iLand automatically queries available snapshot years (`ru >= 0`) and filters to the selected year on the slider.

---

## 3. Examples for custom output tables

### Example A: custom `dynamicstand` timeline (`custom_dynstand`)

Suppose your project XML configures `<output><dynamicstand>` with `columns` `dbh.mean, dbh.p90, volume.sum`:

```jsonc
{
  "version": 1,
  "visualizations": [
    {
      "id": "custom_dynamicstand_dbh",
      "name": "Stand diameter distribution (dynamic)",
      "category": "Custom project charts",
      "description": "Mean and 90th-percentile DBH per species on a selected RU.",
      "table": "dynamicstand",
      "type": "line",
      "x": "year",
      "xLabel": "Year",
      "group": "species",
      "colorMode": "species",
      "filters": ["ru"],
      "metricsMode": "select",
      "metrics": [
        { "col": "dbh_mean",   "label": "Mean DBH (cm)" },
        { "col": "dbh_p90",    "label": "90th percentile DBH (cm)" },
        { "col": "volume_sum", "label": "Volume sum (m³)" }
      ],
      "sql": "SELECT year, species, dbh_mean, dbh_p90, volume_sum FROM dynamicstand WHERE ru = :ru ORDER BY year, species"
    }
  ]
}
```

### Example B: custom spatial map from `svdindicator`

```jsonc
{
  "version": 1,
  "visualizations": [
    {
      "id": "custom_svd_shannon_map",
      "name": "Shannon diversity & IBP map",
      "category": "Spatial maps (RU)",
      "description": "Spatial map of SVD biodiversity and structural indicators.",
      "table": "svdindicator",
      "type": "spatial",
      "spatialKey": "rid",
      "palette": "turbo",
      "metricsMode": "select",
      "metrics": [
        { "col": "shannonIndex",      "label": "Shannon index" },
        { "col": "IBP",               "label": "Index of biodiversity potential" },
        { "col": "abovegroundCarbon", "label": "Aboveground C (t C/ha)" }
      ],
      "sql": "SELECT year, ru, rid, shannonIndex, IBP, abovegroundCarbon FROM svdindicator"
    }
  ]
}
```
