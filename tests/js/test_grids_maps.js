// JS API Test Suite: Globals.grid(), speciesShareGrid(), resourceUnitGrid(), gridToFile(), Grid, Map
Globals.include('../../js/harness.js');
Test.suite('js_grids_maps');

function tempFile(name) {
  return Globals.defaultDirectory('temp') + name;
}

Test.add('Globals.grid() returns valid 10m and 100m model grids', function(assert) {
  const types10m = [
    'height', 'valid', 'count', 'forestoutside', 'standgrid',
    'sap_hmax10', 'saplingcover10', 'smallsaplingcover10'
  ];
  for (let i = 0; i < types10m.length; ++i) {
    const g = Globals.grid(types10m[i]);
    assert.eq(g.isValid, true, types10m[i] + ' isValid');
    assert.eq(g.cellsize, 10, types10m[i] + ' cellsize');
    assert.eq(g.width, 60, types10m[i] + ' width (400m + 2*100m buffer)');
    assert.eq(g.height, 40, types10m[i] + ' height (200m + 2*100m buffer)');
    assert.eq(g.count, 2400, types10m[i] + ' count');
  }

  const types100m = ['smallsaplingcover', 'saplingcover', 'swc', 'swc_gs', 'swc_pot'];
  for (let i = 0; i < types100m.length; ++i) {
    const g = Globals.grid(types100m[i]);
    assert.eq(g.isValid, true, types100m[i] + ' isValid');
    assert.eq(g.cellsize, 100, types100m[i] + ' cellsize');
    assert.eq(g.width, 4, types100m[i] + ' width');
    assert.eq(g.height, 2, types100m[i] + ' height');
    assert.eq(g.count, 8, types100m[i] + ' count');
  }

  // Stand grid has stands 1..6 across all 800 stockable pixels; valid mask has 800 valid cells
  const validGrid = Globals.grid('valid');
  assert.eq(validGrid.sum('valid'), 800, 'all 800 10m cells valid in bgd_mini');

  assert.throws(function() {
    Globals.grid('no_such_grid_type');
  }, 'invalid grid specified');
});

Test.add('Globals.speciesShareGrid() and resourceUnitGrid() match RU expressions', function(assert) {
  const piabGrid = Globals.speciesShareGrid('piab');
  assert.eq(piabGrid.isValid, true);
  assert.eq(piabGrid.width, 4);
  assert.eq(piabGrid.height, 2);
  assert.eq(piabGrid.cellsize, 100);
  piabGrid.name = 'ba';
  assert.gt(piabGrid.sum('ba'), 0, 'piab basal area > 0');

  assert.throws(function() {
    Globals.speciesShareGrid('nonexistent_species');
  }, 'invalid species');

  const baGrid = Globals.resourceUnitGrid('basalArea');
  assert.eq(baGrid.width, 4);
  assert.eq(baGrid.height, 2);

  let expectedSum = 0;
  for (let i = 0; i < Globals.resourceUnitCount; ++i) {
    expectedSum += Globals.resourceUnit(i).expr('basalArea');
  }
  assert.approx(baGrid.sum('ru'), expectedSum, 1e-4, 'resourceUnitGrid sum matches RU expr sum');
});

Test.add('Globals.gridToFile() exports height (10m) and lif (2m) ASCII grids', function(assert) {
  const hPath = tempFile('export_height.asc');
  const lPath = tempFile('export_lif.asc');
  assert.eq(Globals.gridToFile('height', hPath), true);
  assert.eq(Globals.gridToFile('lif', lPath), true);

  const hg = new Grid(hPath);
  assert.eq(hg.isValid, true);
  assert.eq(hg.width, 60);
  assert.eq(hg.height, 40);
  assert.eq(hg.cellsize, 10);

  const lg = new Grid(lPath);
  assert.eq(lg.isValid, true);
  assert.eq(lg.width, 300);
  assert.eq(lg.height, 200);
  assert.eq(lg.cellsize, 2);
});

Test.add('Grid coordinate mapping, algebra, resample, aggregate, and save/load', function(assert) {
  const g = new Grid();
  assert.eq(g.create(6, 4, 10), true, 'create 6x4 @ 10m');
  g.setOrigin(20, 30);
  assert.eq(g.width, 6);
  assert.eq(g.height, 4);
  assert.eq(g.count, 24);
  assert.eq(g.cellsize, 10);

  assert.eq(g.isIndexValid(0, 0), true);
  assert.eq(g.isIndexValid(6, 0), false);
  assert.eq(g.isCoordValid(25, 35), true);
  assert.eq(g.isCoordValid(15, 35), false);
  assert.approx(g.metricX(0), 25, 1e-9);
  assert.approx(g.metricY(0), 35, 1e-9);
  assert.eq(g.indexX(25), 0);
  assert.eq(g.indexY(35), 0);

  g.name = 'v';
  g.clear();
  g.setValue(0, 0, 5);
  g.setValueAt(35, 45, 10); // cell (1, 1)
  assert.eq(g.value(1, 1), 10);
  assert.eq(g.values().length, 24);

  g.apply('if(v > 0, v * 3 + 1, 2)'); // (0,0)=16, (1,1)=31, others(22 cells)=2
  assert.eq(g.value(0, 0), 16);
  assert.eq(g.value(1, 1), 31);
  assert.approx(g.sum('v'), 16 + 31 + 22 * 2, 1e-9);

  const c = g.copy();
  c.combine('a + b', { a: g, b: c });
  assert.eq(c.value(0, 0), 32);
  assert.eq(c.value(1, 1), 62);
  assert.eq(g.value(0, 0), 16, 'copy/combine left source grid unchanged');

  // Aggregate 6x4 (10m) by factor 2 -> 3x2 (20m)
  c.aggregate(2);
  assert.eq(c.width, 3);
  assert.eq(c.height, 2);
  assert.eq(c.cellsize, 20);
  // First 2x2 block = {32, 4, 4, 62} -> mean = 25.5
  assert.approx(c.value(0, 0), 25.5, 1e-9);

  // Resample back onto original 6x4 grid geometry
  const resampled = c.copy();
  resampled.resample(g);
  assert.eq(resampled.width, 6);
  assert.eq(resampled.height, 4);
  assert.eq(resampled.cellsize, 10);
  assert.approx(resampled.value(0, 0), 25.5, 1e-9);

  // Save and reload via File
  const gridFile = tempFile('js_grid_roundtrip.asc');
  c.save(gridFile);
  const loaded = new Grid(gridFile);
  assert.eq(loaded.isValid, true);
  assert.eq(loaded.width, 3);
  assert.eq(loaded.height, 2);
  assert.eq(loaded.cellsize, 20);
  assert.approx(loaded.value(0, 0), 25.5, 1e-6);
});

Test.add('Grid.sumTrees() aggregates individual trees accurately', function(assert) {
  const tg = new Grid();
  tg.create(4, 2, 100);
  tg.setOrigin(0, 0);
  tg.sumTrees('basalarea', 'species = piab');

  const tl = new TreeList();
  tl.load('species = piab');
  const expectedBA = tl.sum('basalarea');
  assert.gt(expectedBA, 0);
  assert.approx(tg.sum('x'), expectedBA, 1e-3, 'sumTrees matches TreeList.sum');
});

Test.add('Map wraps standGrid, creates polygons, and copies from Grid', function(assert) {
  const m = new Map();
  assert.ok(m.valid, 'default Map wrapping standGrid is valid');

  let totalArea = 0;
  for (let sid = 1; sid <= 6; ++sid) {
    const a = m.area(sid);
    assert.gt(a, 0, 'stand ' + sid + ' has positive area');
    totalArea += a;
  }
  assert.approx(totalArea, 80000, 1e-6, 'total stand area covers 8 ha');

  const box1 = m.boundingBox(1);
  assert.ok(box1 !== undefined && box1 !== null, 'boundingBox(1) returned');

  // Standalone map loaded from file
  const mFile = new Map(Globals.path('gis/stand_grid.asc'));
  assert.ok(mFile.valid);
  assert.approx(mFile.area(1), m.area(1), 1e-6, 'loaded Map area matches model standGrid');

  // Create stand without mutating the model's global standGrid
  const mCustom = new Map();
  mCustom.clearProjectArea();
  mCustom.createStand(10, 'x >= 0 and x < 100 and y >= 0 and y < 100', false);
  assert.approx(mCustom.area(10), 10000, 1e-6, 'createStand painted 1 ha square');

  // Copy from a 60x40 10m ScriptGrid (matching buffered HeightGrid extent) into an independent Map + rebuild index
  const mGridCopy = new Map();
  mGridCopy.clearProjectArea();
  const src = new Grid();
  src.create(60, 40, 10);
  src.setOrigin(-100, -100);
  src.clear();
  for (let ix = 0; ix < 5; ++ix) {
    for (let iy = 0; iy < 4; ++iy) {
      src.setValue(ix, iy, 7); // 20 cells * 100 m2 = 2000 m2
    }
  }
  mGridCopy.copyFromGrid(src);
  mGridCopy.createMapIndex();
  assert.approx(mGridCopy.area(7), 2000, 1e-6, 'copyFromGrid + createMapIndex sets exact area');
});

function onAfterCreate() {
  Test.run();
}
