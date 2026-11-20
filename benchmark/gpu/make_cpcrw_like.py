#!/usr/bin/env python3
"""Build a CPCRW-like iLand project from the public iLand demo, for benchmarking.

The real CPCRW inputs (species, climate and GIS databases) are not public, so
this script combines:

  * scripts/CPCRW_hist_spinup_derecho.xml and scripts/saveWorkflow.js from
    https://github.com/QuinnAsena/iLand-NA-parallel (MIT licence; downloaded unless a local
    copy of that scripts/ folder is given), unchanged except for the settings listed below: the same model
    settings, permafrost, fire module, management script and outputs; and
  * the landscape, soils and climate of the official iLand demo project
    (https://iland-model.org/downloads/example_bgd.zip, Berchtesgaden,
    5.5 x 7.5 km, 2,272 one-hectare resource units).

What is changed, and why:

  * world: the demo's size, location, stand grid, DEM and environment
    (soil and climate table per resource unit); latitude 65 as in CPCRW.
  * species: the Alaskan parameter sets from the demo's species database
    (Thom et al. 2024): pima_alaska, pigl_Alaska, Bene, potr_alaska. The demo
    has no light-influence-pattern (LIP) files for the two spruces and the
    birch, so they use the closest available crown shapes (Sitka spruce
    pisi.bin, European birch bepe.bin). This changes shading values slightly,
    not the amount of computation.
  * climate: the demo's 30 years (1980-2009) instead of CPCRW's GCM table, so
    the year filter is dropped and the random-sampling list is taken modulo 30;
    temperatures are shifted by -7 C to approximate interior Alaska (so snow and
    permafrost behave in a boreal way).
  * initialisation: like CPCRW, bare ground plus saplings in every stand
    (stand_id, species, count, height, age), written by this script.
  * small stand-in input files for saveWorkflow.js (perimeters raster, polygon
    list and planned-disturbance list), which it loads every run but whose
    contents only matter for deterministic disturbances (switched off in the
    script).

Usage:
    python3 make_cpcrw_like.py DEMO_DIR OUT_DIR [CPCRW_SCRIPTS_DIR]
DEMO_DIR is the unzipped example_bgd folder; CPCRW_SCRIPTS_DIR a local copy of
iLand-NA-parallel's scripts/ folder (otherwise the two files are downloaded).
Output folders are created per run by run_benchmark.sh, as the HPC run script does.
"""

import shutil
import sqlite3
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

CPCRW_URL = "https://raw.githubusercontent.com/QuinnAsena/iLand-NA-parallel/main/scripts/"
CPCRW_FILES = ("CPCRW_hist_spinup_derecho.xml", "saveWorkflow.js")

SPECIES = {  # shortName: LIP file to use
    "pima_alaska": "pisi.bin",
    "pigl_Alaska": "pisi.bin",
    "Bene": "bepe.bin",
    "potr_alaska": "potr.bin",
}
SAPLINGS_PER_HA = {"pima_alaska": 2000, "pigl_Alaska": 500, "potr_alaska": 800, "Bene": 500}
TEMPERATURE_SHIFT = -7


def set_text(root, path, value):
    node = root.find(path)
    if node is None:
        raise KeyError(path)
    node.text = str(value)


def cpcrw_files(out, local_dir=None):
    """The CPCRW project file and workflow script: from a local folder, or downloaded."""
    import urllib.request
    paths = []
    for name in CPCRW_FILES:
        target = out / "cpcrw_source" / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if local_dir:
            shutil.copyfile(Path(local_dir) / name, target)
        elif not target.exists():
            urllib.request.urlretrieve(CPCRW_URL + name, target)
        paths.append(target)
    return paths


def main(demo, out, local_cpcrw=None):
    demo, out = Path(demo).resolve(), Path(out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    CPCRW_XML, WORKFLOW_JS = cpcrw_files(out, local_cpcrw)
    for sub in ("databases", "gis", "init", "scripts", "output", "log", "temp"):   # CPCRW calls it "databases"
        (out / sub).mkdir(parents=True, exist_ok=True)

    # LIP files and the climate database are used read-only: link them
    if not (out / "lip").exists():
        (out / "lip").symlink_to(demo / "lip")
    climate = out / "databases" / "historic_climate.sqlite"
    if not climate.exists():
        climate.symlink_to(demo / "database" / "historic_climate.sqlite")

    # Species database: a copy with only the four Alaskan species active
    species_db = out / "databases" / "species_cpcrw_like.sqlite"
    shutil.copyfile(demo / "database" / "all_species_database.sqlite", species_db)
    with sqlite3.connect(species_db) as db:
        db.execute("update species set active = 0")
        for short_name, lip in SPECIES.items():
            changed = db.execute("update species set active = 1, LIPfile = ? where shortName = ?",
                                 (lip, short_name)).rowcount
            if changed != 1:
                raise ValueError(f"species {short_name} not found")

    # GIS: the demo's grids, plus a stand-in perimeters raster for saveWorkflow.js
    for name in ("objectid.asc", "soid.asc", "environment.txt", "npbg_dem50fix.asc"):
        shutil.copyfile(demo / "gis" / name, out / "gis" / name)
    shutil.copyfile(demo / "gis" / "soid.asc", out / "gis" / "perimeters_raster.txt")

    # saveWorkflow.js unchanged, with empty polygon and planned-disturbance lists
    shutil.copyfile(WORKFLOW_JS, out / "scripts" / "saveWorkflow.js")
    (out / "scripts" / "polygon_list.txt").write_text("id,minx,maxx,miny,maxy,area\n")
    (out / "scripts" / "planned_disturbance.txt").write_text("year,id\n")

    # Saplings in every stand of the stand grid (negative ids are outside the forest)
    header, values = (demo / "gis" / "soid.asc").read_text().split("\n", 6)[:6], None
    values = (demo / "gis" / "soid.asc").read_text().split("\n", 6)[6].split()
    stands = sorted({int(float(v)) for v in values} - {-9999})
    stands = [s for s in stands if s >= 0]
    lines = ["stand_id,species,count,height,age"]
    for stand in stands:
        for species, count in SAPLINGS_PER_HA.items():
            lines.append(f"{stand},{species},{count},0.3,3")
    (out / "init" / "saplings_cpcrw_like.txt").write_text("\n".join(lines) + "\n")

    # Project file: CPCRW's, with the world, species, climate and init changes above
    tree = ET.parse(CPCRW_XML, parser=ET.XMLParser(target=ET.TreeBuilder(insert_comments=True)))
    root = tree.getroot()
    set_text(root, "system/path/output", "output")
    set_text(root, "system/database/in", species_db.name)
    set_text(root, "system/database/climate", climate.name)
    set_text(root, "system/logging/logFile", "log/log.txt")
    world = "model/world/"
    for key, value in (("width", 5500), ("height", 7500), ("buffer", 200), ("latitude", 65),
                       ("environmentGrid", "gis/objectid.asc"), ("environmentFile", "gis/environment.txt"),
                       ("location/x", 4559008), ("location/y", 5267625),
                       ("standGrid/fileName", "gis/soid.asc"), ("DEM", "gis/npbg_dem50fix.asc")):
        set_text(root, world + key, value)
    clim = root.find("model/climate")
    set_text(root, "model/climate/tableName", "climate1")   # per resource unit from environment.txt
    set_text(root, "model/climate/filter", "")
    set_text(root, "model/climate/batchYears", 30)
    set_text(root, "model/climate/temperatureShift", TEMPERATURE_SHIFT)
    years = [int(v) % 30 for v in clim.find("randomSamplingList").text.replace('"', "").split(",")]
    set_text(root, "model/climate/randomSamplingList", ",".join(map(str, years)))
    set_text(root, "model/initialization/saplingFile", "saplings_cpcrw_like.txt")
    tree.write(out / "CPCRW_like.xml", encoding="utf-8", xml_declaration=True)
    print(f"wrote {out / 'CPCRW_like.xml'}: {len(stands)} stands, species {', '.join(SPECIES)}")


if __name__ == "__main__":
    if len(sys.argv) not in (3, 4):
        sys.exit(__doc__)
    main(*sys.argv[1:4])
