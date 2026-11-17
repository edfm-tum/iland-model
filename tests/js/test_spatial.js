// JS API Test Suite: SpatialAnalysis (rumpleIndex, patches, saveCrownCoverGrid)
Globals.include('../../js/harness.js');
Test.suite('js_spatial');

function tempFile(name) {
  return Globals.defaultDirectory('temp') + name;
}

Test.add('SpatialAnalysis.rumpleIndex and saveRumpleGrid()', function(assert) {
  const sa = new SpatialAnalysis();
  assert.gt(sa.rumpleIndex, 1.0, 'rumple index > 1 on forested landscape');

  const rumpleFile = tempFile('js_rumple.asc');
  sa.saveRumpleGrid(rumpleFile);
  assert.eq(Globals.fileExists(rumpleFile), true);

  const g = new Grid(rumpleFile);
  assert.eq(g.isValid, true);
  assert.eq(g.width, 4);
  assert.eq(g.height, 2);
  assert.eq(g.cellsize, 100);
  assert.gt(g.sum(g.name), 8.0, 'each RU rumple >= 1.0');

  const sa2 = Factory.newSpatialAnalysis();
  assert.approx(sa2.rumpleIndex, sa.rumpleIndex, 1e-9);
});

Test.add('SpatialAnalysis.patches() extracts connected components with min_size filter', function(assert) {
  const g = new Grid();
  g.create(10, 10, 10);
  g.clear();

  // Patch A: 3x2 = 6 cells
  for (let ix = 1; ix <= 3; ++ix) {
    for (let iy = 1; iy <= 2; ++iy) {
      g.setValue(ix, iy, 1);
    }
  }
  // Patch B: 2 cells
  g.setValue(8, 8, 1);
  g.setValue(8, 9, 1);

  const sa = new SpatialAnalysis();
  const resAll = sa.patches(g, 1);
  assert.eq(resAll.grid.isValid, true);
  assert.eq(resAll.areas.length, 2, '2 patches found with min_size=1');
  assert.eq(sa.patchsizes.length, 2);

  const areasSorted = resAll.areas.slice().sort(function(a, b) { return a - b; });
  assert.eq(areasSorted[0], 2);
  assert.eq(areasSorted[1], 6);

  const resFiltered = sa.patches(g, 4);
  assert.eq(resFiltered.areas.length, 1, '1 patch remains with min_size=4');
  assert.eq(resFiltered.areas[0], 6);
  assert.gt(resFiltered.grid.value(1, 1), 0, 'large patch kept');
  assert.eq(resFiltered.grid.value(8, 8), 0, 'small 2-cell patch pruned');
});

// Keep saveCrownCoverGrid() last because the 1-arg overload projects onto the 2m LIF grid.
Test.add('SpatialAnalysis.saveCrownCoverGrid() exports RU and custom grids', function(assert) {
  const sa = new SpatialAnalysis();
  const ccRuFile = tempFile('js_crowncover_ru.asc');
  sa.saveCrownCoverGrid(ccRuFile);
  assert.eq(Globals.fileExists(ccRuFile), true);

  const gRu = new Grid(ccRuFile);
  assert.eq(gRu.isValid, true);
  assert.eq(gRu.width, 4);
  assert.eq(gRu.height, 2);
  assert.gt(gRu.value(0, 0), 0, 'stocked RU 0 has positive crown cover');
  assert.lte(gRu.value(0, 0), 1.0, 'crown cover <= 1.0');
  assert.lt(gRu.value(2, 0), 0.05, 'sapling-only RU 2 has near-zero adult crown cover');
  assert.gt(gRu.value(0, 0), gRu.value(2, 0) * 5, 'stocked RU 0 crown cover >> RU 2 border overhang');

  const target = new Grid();
  target.create(20, 10, 20);
  target.setOrigin(0, 0);
  const ccCustomFile = tempFile('js_crowncover_20m.asc');
  sa.saveCrownCoverGrid(ccCustomFile, target);
  const g20 = new Grid(ccCustomFile);
  assert.eq(g20.isValid, true);
  assert.eq(g20.width, 20);
  assert.eq(g20.height, 10);
  assert.eq(g20.cellsize, 20);
  assert.gt(g20.sum(g20.name), 0);
});

function onAfterCreate() {
  Test.run();
}
