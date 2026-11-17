// Save full model snapshot (trees, saplings, soil, snags, single dead trees) at end of year 10
function onYearEnd() {
    if (Globals.year === 10) {
        var outDir = Globals.setting("system.path.output");
        var snapFile = Globals.path((outDir ? outDir : "output") + "/snapshot_y10.sqlite");
        Globals.saveModelSnapshot(snapFile);
    }
}
