#!/usr/bin/env python3
"""Tier 2 Simulation Test Runner & SQLite Comparator for iLand.

Discovers declarative YAML simulation tests in tests/simulations/, runs headless
ilandc into isolated temporary output directories, evaluates SQL assertions,
verifies determinism, and compares SQLite query results against human-readable
CSV baselines in tests/simulations/expected/.
"""
from __future__ import annotations

import argparse
import csv
import math
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError as exc:  # pragma: no cover
    sys.stderr.write(
        "ERROR: PyYAML ('yaml') is required to run simulation tests.\n"
        f"Import error: {exc}\n"
    )
    sys.exit(2)


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_SUITE_DIR = Path(__file__).resolve().parent / "simulations"
DEFAULT_ILANDC = REPO_ROOT / "src" / "ilandc" / "build" / "release" / "ilandc"
DEFAULT_UNIT_BIN = REPO_ROOT / "src" / "tests" / "build" / "release" / "iland_tests"

DEFAULT_REL_TOL = 1e-5
DEFAULT_ABS_TOL = 1e-6

SAFE_BUILTINS = {
    "abs": abs,
    "all": all,
    "any": any,
    "bool": bool,
    "float": float,
    "int": int,
    "len": len,
    "max": max,
    "min": min,
    "round": round,
    "set": set,
    "sorted": sorted,
    "str": str,
    "sum": sum,
    "tuple": tuple,
}


@dataclass
class AssertionSpec:
    name: str
    query: str
    expect: str


@dataclass
class BaselineSpec:
    name: str
    query: str
    rel_tol: float = DEFAULT_REL_TOL
    abs_tol: float = DEFAULT_ABS_TOL
    column_tolerances: dict[str, dict[str, float]] = field(default_factory=dict)


@dataclass
class SimTestSpec:
    test_id: str
    source_file: Path
    project: Path
    years: int
    description: str = ""
    compare_with: str | None = None
    params: dict[str, Any] = field(default_factory=dict)
    check_determinism: bool = False
    assertions: list[AssertionSpec] = field(default_factory=list)
    baselines: list[BaselineSpec] = field(default_factory=list)
    expected_dir: Path = field(default_factory=Path)


def format_param_value(val: Any) -> str:
    if isinstance(val, bool):
        return "true" if val else "false"
    return str(val)


def format_csv_cell(val: Any) -> str:
    if val is None:
        return ""
    if isinstance(val, bool):
        return "1" if val else "0"
    if isinstance(val, int):
        return str(val)
    if isinstance(val, float):
        if math.isnan(val):
            return "nan"
        if math.isinf(val):
            return "inf" if val > 0 else "-inf"
        if abs(val) < 1e-15:
            return "0"
        return f"{val:.10g}"
    return str(val)


def try_parse_float(raw: str) -> float | None:
    s = raw.strip()
    if not s:
        return None
    try:
        return float(s)
    except ValueError:
        return None


def resolve_project_path(raw_project: str, yaml_file: Path) -> Path:
    p = Path(raw_project)
    if p.is_absolute():
        return p
    rel_to_yaml = (yaml_file.parent / p).resolve()
    if rel_to_yaml.exists():
        return rel_to_yaml
    return (REPO_ROOT / p).resolve()


def parse_test_entry(raw: dict[str, Any], yaml_file: Path, defaults: dict[str, Any], expected_root: Path) -> SimTestSpec:
    merged = dict(defaults)
    merged.update(raw)

    test_id = str(merged.get("id") or yaml_file.stem)
    if "project" not in merged:
        raise ValueError(f"{yaml_file}: test '{test_id}' missing required key 'project'")
    if "years" not in merged:
        raise ValueError(f"{yaml_file}: test '{test_id}' missing required key 'years'")

    project = resolve_project_path(str(merged["project"]), yaml_file)
    years = int(merged["years"])
    base_rel_tol = float(merged.get("rel_tol", DEFAULT_REL_TOL))
    base_abs_tol = float(merged.get("abs_tol", DEFAULT_ABS_TOL))

    params = dict(defaults.get("params") or {})
    params.update(raw.get("params") or {})

    assertions = [
        AssertionSpec(
            name=str(a["name"]),
            query=str(a["query"]),
            expect=str(a["expect"]),
        )
        for a in (merged.get("assertions") or [])
    ]

    baselines = []
    for b in (merged.get("baselines") or []):
        col_tols = {}
        for col, spec in (b.get("column_tolerances") or {}).items():
            col_tols[str(col)] = {
                "rel_tol": float(spec.get("rel_tol", base_rel_tol)),
                "abs_tol": float(spec.get("abs_tol", base_abs_tol)),
            }
        baselines.append(
            BaselineSpec(
                name=str(b["name"]),
                query=str(b["query"]),
                rel_tol=float(b.get("rel_tol", base_rel_tol)),
                abs_tol=float(b.get("abs_tol", base_abs_tol)),
                column_tolerances=col_tols,
            )
        )

    compare_with_raw = merged.get("compare_with")
    compare_with = str(compare_with_raw).strip() if compare_with_raw else None

    return SimTestSpec(
        test_id=test_id,
        source_file=yaml_file,
        project=project,
        years=years,
        description=str(merged.get("description", "")).strip(),
        compare_with=compare_with,
        params=params,
        check_determinism=bool(merged.get("check_determinism", False)),
        assertions=assertions,
        baselines=baselines,
        expected_dir=expected_root / test_id,
    )


def discover_tests(suite_dir: Path) -> list[SimTestSpec]:
    if not suite_dir.is_dir():
        raise FileNotFoundError(f"Simulation suite directory not found: {suite_dir}")

    expected_root = suite_dir / "expected"
    yaml_files = sorted(list(suite_dir.glob("*.yaml")) + list(suite_dir.glob("*.yml")))
    specs: list[SimTestSpec] = []
    seen_ids: set[str] = set()

    for yf in yaml_files:
        data = yaml.safe_load(yf.read_text(encoding="utf-8"))
        if not isinstance(data, dict):
            raise ValueError(f"{yf}: top-level YAML document must be a mapping")
        if "tests" in data and isinstance(data["tests"], list):
            defaults = {k: v for k, v in data.items() if k != "tests"}
            for entry in data["tests"]:
                spec = parse_test_entry(entry, yf, defaults, expected_root)
                if spec.test_id in seen_ids:
                    raise ValueError(f"Duplicate test id '{spec.test_id}' in {yf}")
                seen_ids.add(spec.test_id)
                specs.append(spec)
        else:
            spec = parse_test_entry(data, yf, {}, expected_root)
            if spec.test_id in seen_ids:
                raise ValueError(f"Duplicate test id '{spec.test_id}' in {yf}")
            seen_ids.add(spec.test_id)
            specs.append(spec)

    return specs


def tail_text(path: Path, max_lines: int = 25) -> str:
    if not path.is_file():
        return "(file missing)"
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    if len(lines) <= max_lines:
        return "\n".join(lines)
    return "\n".join(["..."] + lines[-max_lines:])


def run_ilandc(
    ilandc_bin: Path, spec: SimTestSpec, run_dir: Path, ref_dir: Path | None = None
) -> tuple[bool, float, Path, str]:
    db_path = run_dir / "output.sqlite"
    log_path = run_dir / "run.log"
    stdout_path = run_dir / "stdout.txt"

    cmd = [
        str(ilandc_bin),
        str(spec.project),
        str(spec.years),
        f"system.path.output={run_dir}",
        f"system.path.temp={run_dir}",
        "system.database.out=output.sqlite",
        f"system.logging.logFile={log_path}",
    ]
    for key, val in spec.params.items():
        val_str = format_param_value(val)
        if ref_dir is not None:
            val_str = val_str.replace("{ref_dir}", str(ref_dir))
        cmd.append(f"{key}={val_str}")

    t0 = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.perf_counter() - t0

    combined = (proc.stdout or "") + ("\n[STDERR]\n" + proc.stderr if proc.stderr else "")
    stdout_path.write_text(combined, encoding="utf-8")

    if proc.returncode != 0:
        detail = (
            f"ilandc exited with code {proc.returncode}\n"
            f"Command: {' '.join(cmd)}\n"
            f"--- stdout/stderr tail ---\n{tail_text(stdout_path)}\n"
            f"--- run.log tail ---\n{tail_text(log_path)}"
        )
        return False, elapsed, db_path, detail

    if not db_path.is_file():
        detail = (
            f"ilandc exited 0 but output SQLite DB was not created at {db_path}\n"
            f"--- stdout/stderr tail ---\n{tail_text(stdout_path)}\n"
            f"--- run.log tail ---\n{tail_text(log_path)}"
        )
        return False, elapsed, db_path, detail

    return True, elapsed, db_path, ""


def query_sqlite(conn: sqlite3.Connection, sql: str) -> tuple[list[str], list[tuple[Any, ...]]]:
    cur = conn.execute(sql)
    cols = [desc[0] for desc in (cur.description or [])]
    rows = cur.fetchall()
    return cols, rows


UNORDERED_TABLES = {"landscape_removed"}  # populated by QHash iteration in C++


def check_determinism(db1: Path, db2: Path) -> tuple[bool, str]:
    c1 = sqlite3.connect(str(db1))
    c2 = sqlite3.connect(str(db2))
    try:
        ignored = {"runinfo"}
        tables1 = sorted(
            r[0]
            for r in c1.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'")
            if r[0] not in ignored
        )
        tables2 = sorted(
            r[0]
            for r in c2.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'")
            if r[0] not in ignored
        )
        if tables1 != tables2:
            return False, f"Tables differ between identical runs: {tables1} vs {tables2}"

        for tbl in tables1:
            cols1, rows1 = query_sqlite(c1, f'SELECT * FROM "{tbl}"')
            cols2, rows2 = query_sqlite(c2, f'SELECT * FROM "{tbl}"')
            if cols1 != cols2:
                return False, f"Table '{tbl}' columns differ: {cols1} vs {cols2}"
            if len(rows1) != len(rows2):
                return False, f"Table '{tbl}' row count differs: {len(rows1)} vs {len(rows2)}"
            if tbl in UNORDERED_TABLES:
                rows1 = sorted(rows1, key=lambda r: tuple("" if v is None else str(v) for v in r))
                rows2 = sorted(rows2, key=lambda r: tuple("" if v is None else str(v) for v in r))
            for r_idx, (r1, r2) in enumerate(zip(rows1, rows2)):
                if r1 != r2:
                    for col, v1, v2 in zip(cols1, r1, r2):
                        if v1 != v2:
                            return (
                                False,
                                f"Table '{tbl}' row {r_idx} col '{col}' non-deterministic: {v1!r} vs {v2!r}",
                            )
        return True, f"{len(tables1)} tables bit-identical"
    finally:
        c1.close()
        c2.close()


def evaluate_assertion(conn: sqlite3.Connection, spec: AssertionSpec) -> tuple[bool, str]:
    try:
        cols, raw_rows = query_sqlite(conn, spec.query)
    except sqlite3.Error as exc:
        return False, f"Assertion '{spec.name}' SQL error: {exc}"
    rows = [dict(zip(cols, r)) for r in raw_rows]
    row = rows[0] if rows else None
    env = {"rows": rows, "row": row, "n": len(rows)}

    try:
        passed = bool(eval(spec.expect, {"__builtins__": SAFE_BUILTINS}, env))
    except Exception as exc:
        return False, f"Assertion '{spec.name}' raised {type(exc).__name__}: {exc} (row={row})"

    if not passed:
        preview = rows[0] if len(rows) == 1 else f"{len(rows)} rows; first={row}"
        return False, f"Assertion '{spec.name}' failed: expect ({spec.expect}) was False [got {preview}]"
    return True, f"ok ({len(rows)} row(s))"


def read_csv_baseline(csv_path: Path) -> tuple[list[str], list[list[str]]]:
    with csv_path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.reader(f)
        header = next(reader, None)
        if header is None:
            raise ValueError(f"Empty baseline CSV: {csv_path}")
        rows = list(reader)
    return header, rows


def write_csv_baseline(csv_path: Path, cols: list[str], rows: list[tuple[Any, ...]]) -> None:
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerow(cols)
        for row in rows:
            writer.writerow([format_csv_cell(v) for v in row])


def summarize_baseline_update(
    csv_path: Path, new_cols: list[str], new_rows: list[tuple[Any, ...]]
) -> str:
    if not csv_path.is_file():
        return f"CREATED ({len(new_rows)} rows, {len(new_cols)} cols)"

    old_cols, old_rows = read_csv_baseline(csv_path)
    if old_cols != new_cols or len(old_rows) != len(new_rows):
        return (
            f"UPDATED (shape {len(old_rows)}x{len(old_cols)} -> {len(new_rows)}x{len(new_cols)})"
        )

    max_abs = 0.0
    max_rel = 0.0
    max_col = ""
    changed_cells = 0

    for old_r, new_r in zip(old_rows, new_rows):
        for col, ov_str, nv in zip(new_cols, old_r, new_r):
            nv_str = format_csv_cell(nv)
            if ov_str == nv_str:
                continue
            changed_cells += 1
            ov_f = try_parse_float(ov_str)
            nv_f = float(nv) if isinstance(nv, (int, float)) and not isinstance(nv, bool) else None
            if ov_f is not None and nv_f is not None:
                adiff = abs(nv_f - ov_f)
                denom = max(abs(ov_f), abs(nv_f), 1e-12)
                rdiff = adiff / denom
                if adiff > max_abs:
                    max_abs = adiff
                    max_rel = rdiff
                    max_col = col

    if changed_cells == 0:
        return f"UNCHANGED ({len(new_rows)} rows)"
    if max_col:
        return (
            f"UPDATED ({changed_cells} cells; max delta in '{max_col}': "
            f"abs={max_abs:.4g}, rel={max_rel:.4g})"
        )
    return f"UPDATED ({changed_cells} non-numeric cells)"


def compare_baseline(
    conn: sqlite3.Connection, spec: BaselineSpec, csv_path: Path
) -> tuple[bool, str]:
    if not csv_path.is_file():
        return (
            False,
            f"Baseline '{spec.name}' missing at {csv_path}. Run with --update-baseline to generate.",
        )

    try:
        act_cols, act_rows = query_sqlite(conn, spec.query)
    except sqlite3.Error as exc:
        return False, f"Baseline '{spec.name}' SQL error: {exc}"
    exp_cols, exp_rows = read_csv_baseline(csv_path)

    if exp_cols != act_cols:
        return False, f"Baseline '{spec.name}' columns mismatch: expected {exp_cols}, got {act_cols}"
    if len(exp_rows) != len(act_rows):
        return (
            False,
            f"Baseline '{spec.name}' row count mismatch: expected {len(exp_rows)}, got {len(act_rows)}",
        )

    mismatches: list[str] = []
    total_mismatches = 0

    for r_idx, (exp_r, act_r) in enumerate(zip(exp_rows, act_rows)):
        for c_idx, (col, exp_str, act_val) in enumerate(zip(act_cols, exp_r, act_r)):
            act_is_num = isinstance(act_val, (int, float)) and not isinstance(act_val, bool)
            if act_is_num:
                exp_f = try_parse_float(exp_str)
                if exp_f is None:
                    total_mismatches += 1
                    if len(mismatches) < 10:
                        mismatches.append(
                            f"row {r_idx} '{col}': expected non-numeric {exp_str!r}, got {act_val!r}"
                        )
                    continue

                tols = spec.column_tolerances.get(col)
                rtol = tols["rel_tol"] if tols else spec.rel_tol
                atol = tols["abs_tol"] if tols else spec.abs_tol
                act_f = float(act_val)
                if not math.isclose(act_f, exp_f, rel_tol=rtol, abs_tol=atol):
                    total_mismatches += 1
                    if len(mismatches) < 10:
                        adiff = abs(act_f - exp_f)
                        rdiff = adiff / max(abs(exp_f), abs(act_f), 1e-12)
                        mismatches.append(
                            f"row {r_idx} '{col}': expected {exp_str}, got {format_csv_cell(act_val)} "
                            f"(abs={adiff:.4g}, rel={rdiff:.4g}, tol=rel:{rtol}/abs:{atol})"
                        )
            else:
                act_str = format_csv_cell(act_val)
                if exp_str != act_str:
                    total_mismatches += 1
                    if len(mismatches) < 10:
                        mismatches.append(
                            f"row {r_idx} '{col}': expected {exp_str!r}, got {act_str!r}"
                        )

    if total_mismatches > 0:
        extra = f"\n    ... and {total_mismatches - len(mismatches)} more" if total_mismatches > len(mismatches) else ""
        return (
            False,
            f"Baseline '{spec.name}' failed ({total_mismatches} cell mismatch(es)):\n    "
            + "\n    ".join(mismatches)
            + extra,
        )

    return True, f"ok ({len(act_rows)} rows)"


def filter_specs(specs: list[SimTestSpec], filters: list[str]) -> list[SimTestSpec]:
    if not filters:
        return specs
    needles = [f.lower() for f in filters]
    selected = []
    for s in specs:
        haystack = f"{s.test_id.lower()} {s.source_file.stem.lower()}"
        if any(n in haystack for n in needles):
            selected.append(s)
    return selected


def resolve_execution_order(selected: list[SimTestSpec], all_specs: list[SimTestSpec]) -> list[SimTestSpec]:
    """Ensure every compare_with prerequisite is included and scheduled before dependents."""
    by_id = {s.test_id: s for s in all_specs}
    ordered: list[SimTestSpec] = []
    state: dict[str, int] = {}  # 1 = visiting, 2 = visited

    def visit(spec: SimTestSpec) -> None:
        st = state.get(spec.test_id, 0)
        if st == 2:
            return
        if st == 1:
            raise ValueError(f"Cycle detected in compare_with dependencies involving '{spec.test_id}'")
        state[spec.test_id] = 1
        if spec.compare_with:
            if spec.compare_with not in by_id:
                raise ValueError(
                    f"Test '{spec.test_id}' references unknown compare_with '{spec.compare_with}'"
                )
            visit(by_id[spec.compare_with])
        state[spec.test_id] = 2
        ordered.append(spec)

    for s in selected:
        visit(s)
    return ordered


def run_unit_tests(unit_bin: Path) -> bool:
    print(f"=== Tier 1 C++ Unit Tests: {unit_bin} ===")
    if not unit_bin.is_file():
        print(f"ERROR: C++ test binary not found: {unit_bin}")
        return False
    proc = subprocess.run([str(unit_bin)])
    print()
    return proc.returncode == 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run iLand Tier 2 simulation tests and SQLite baseline comparisons."
    )
    parser.add_argument(
        "filters",
        nargs="*",
        help="Optional substring filter(s) matching test id or YAML file stem.",
    )
    parser.add_argument(
        "--suite-dir",
        type=Path,
        default=DEFAULT_SUITE_DIR,
        help=f"Directory containing *.yaml test specs (default: {DEFAULT_SUITE_DIR}).",
    )
    parser.add_argument(
        "--ilandc",
        type=Path,
        default=DEFAULT_ILANDC,
        help=f"Path to ilandc binary (default: {DEFAULT_ILANDC}).",
    )
    parser.add_argument(
        "--unit-bin",
        type=Path,
        default=DEFAULT_UNIT_BIN,
        help=f"Path to Tier 1 iland_tests binary (default: {DEFAULT_UNIT_BIN}).",
    )
    parser.add_argument(
        "--with-unit",
        "--all",
        dest="with_unit",
        action="store_true",
        help="Also run Tier 1 C++ unit tests before simulation tests.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List discovered simulation tests and exit.",
    )
    parser.add_argument(
        "--update-baseline",
        action="store_true",
        help="Generate or overwrite expected/*.csv baselines from current ilandc output.",
    )
    parser.add_argument(
        "--keep-temp",
        action="store_true",
        help="Keep temporary run directories (output.sqlite, run.log, stdout.txt).",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Print details for passing checks as well as failures.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    specs = discover_tests(args.suite_dir.resolve())
    selected = filter_specs(specs, args.filters)

    if args.list:
        print(f"Discovered {len(selected)} simulation test(s) in {args.suite_dir}:")
        for s in selected:
            rel_proj = s.project.relative_to(REPO_ROOT) if s.project.is_relative_to(REPO_ROOT) else s.project
            print(
                f"  - {s.test_id} ({s.source_file.name}): project={rel_proj}, "
                f"years={s.years}, assertions={len(s.assertions)}, "
                f"baselines={len(s.baselines)}, determinism={s.check_determinism}"
            )
        return 0

    if not selected:
        print(f"ERROR: No simulation tests matched filter(s): {args.filters}")
        return 1

    selected = resolve_execution_order(selected, specs)

    overall_ok = True
    if args.with_unit:
        if not run_unit_tests(args.unit_bin.resolve()):
            overall_ok = False

    ilandc_bin = args.ilandc.resolve()
    if not ilandc_bin.is_file():
        print(f"ERROR: ilandc binary not found: {ilandc_bin}")
        return 1

    print(f"=== Tier 2 Simulation Tests ({len(selected)} suite(s), binary={ilandc_bin.name}) ===")

    passed_tests = 0
    all_temp_dirs: list[Path] = []
    completed_runs: dict[str, tuple[Path, Path]] = {}
    try:
        for spec in selected:
            test_ok = True
            ref_dir: Path | None = None
            ref_db: Path | None = None
            if spec.compare_with:
                if spec.compare_with not in completed_runs:
                    print(
                        f"[FAIL] {spec.test_id}: prerequisite '{spec.compare_with}' did not complete successfully"
                    )
                    overall_ok = False
                    continue
                ref_dir, ref_db = completed_runs[spec.compare_with]

            run1_dir = Path(tempfile.mkdtemp(prefix=f"iland_t2_{spec.test_id}_1_"))
            all_temp_dirs.append(run1_dir)

            ok, elapsed, db1, err_detail = run_ilandc(ilandc_bin, spec, run1_dir, ref_dir=ref_dir)
            if not ok:
                print(f"[FAIL] {spec.test_id}: ilandc run failed ({elapsed:.2f}s)")
                print(err_detail)
                overall_ok = False
                continue

            completed_runs[spec.test_id] = (run1_dir, db1)

            if args.verbose:
                print(f"[RUN ] {spec.test_id} ({spec.years} yr, {elapsed*1000:.0f} ms)")

            # Determinism check
            if spec.check_determinism:
                run2_dir = Path(tempfile.mkdtemp(prefix=f"iland_t2_{spec.test_id}_2_"))
                all_temp_dirs.append(run2_dir)
                ok2, elapsed2, db2, err2 = run_ilandc(ilandc_bin, spec, run2_dir, ref_dir=ref_dir)
                if not ok2:
                    print(f"  [FAIL] determinism second run failed ({elapsed2:.2f}s)\n{err2}")
                    test_ok = False
                else:
                    det_ok, det_msg = check_determinism(db1, db2)
                    if not det_ok:
                        print(f"  [FAIL] determinism: {det_msg}")
                        test_ok = False
                    elif args.verbose:
                        print(f"  [PASS] determinism: {det_msg}")

            conn = sqlite3.connect(str(db1))
            try:
                if ref_db is not None:
                    conn.execute("ATTACH DATABASE ? AS ref", (str(ref_db),))

                # SQL assertions
                for a in spec.assertions:
                    a_ok, a_msg = evaluate_assertion(conn, a)
                    if not a_ok:
                        print(f"  [FAIL] {a_msg}")
                        test_ok = False
                    elif args.verbose:
                        print(f"  [PASS] assertion '{a.name}': {a_msg}")

                # CSV baselines
                for b in spec.baselines:
                    csv_path = spec.expected_dir / f"{b.name}.csv"
                    if args.update_baseline:
                        cols, rows = query_sqlite(conn, b.query)
                        summary = summarize_baseline_update(csv_path, cols, rows)
                        write_csv_baseline(csv_path, cols, rows)
                        print(f"  [BASE] {spec.test_id}/{b.name}.csv -> {summary}")
                    else:
                        b_ok, b_msg = compare_baseline(conn, b, csv_path)
                        if not b_ok:
                            print(f"  [FAIL] {b_msg}")
                            test_ok = False
                        elif args.verbose:
                            print(f"  [PASS] baseline '{b.name}': {b_msg}")
            finally:
                conn.close()

            if test_ok:
                passed_tests += 1
                if not args.update_baseline:
                    print(
                        f"[PASS] {spec.test_id} "
                        f"(assertions={len(spec.assertions)}, baselines={len(spec.baselines)}, {elapsed*1000:.0f} ms)"
                    )
            else:
                print(f"[FAIL] {spec.test_id}")
                overall_ok = False
    finally:
        if args.keep_temp:
            for td in all_temp_dirs:
                print(f"  [TEMP] {td}")
        else:
            for td in all_temp_dirs:
                shutil.rmtree(td, ignore_errors=True)

    print(f"\nSummary: {passed_tests}/{len(selected)} simulation suite(s) passed.")
    return 0 if overall_ok else 1


if __name__ == "__main__":
    sys.exit(main())
