// JS API Test Suite: Globals, Settings, File I/O, ES Modules, ResourceUnits, CSVFile, DBHDistribution, ClimateConverter
Globals.include('../../js/harness.js');
Test.suite('js_globals_io');

function tempFile(name) {
  return Globals.defaultDirectory('temp') + name;
}

Test.add('Globals core properties and world extent', function(assert) {
  assert.eq(Globals.year, 1, 'initial year after create()');
  assert.eq(Globals.resourceUnitCount, 8, 'bgd_mini has 8 resource units');
  assert.eq(Globals.worldX, 400, 'worldX meters');
  assert.eq(Globals.worldY, 200, 'worldY meters');
  assert.eq(Globals.qt5, true, 'qt5 compat flag');
  assert.eq(typeof Globals.msec, 'number', 'msec type');
  assert.gte(Globals.msec, 0, 'msec >= 0');
  assert.gt(Globals.currentDir.length, 0, 'currentDir non-empty');
  assert.gt(Globals.defaultDirectory('output').length, 0, 'output dir non-empty');
});

Test.add('Globals XML setting read/write and invalid-key error', function(assert) {
  const orig = Globals.setting('model.site.availableNitrogen');
  assert.ok(orig !== undefined && orig !== null, 'availableNitrogen readable');

  Globals.set('model.site.availableNitrogen', '65');
  assert.eq(String(Globals.setting('model.site.availableNitrogen')), '65', 'updated setting');
  Globals.set('model.site.availableNitrogen', String(orig));

  assert.throws(function() {
    Globals.setting('invalid.nonexistent.xml.key');
  }, 'not valid');

  assert.throws(function() {
    Globals.set('invalid.nonexistent.xml.key', '123');
  }, 'not valid');
});

Test.add('Globals path resolution and text file round-trip', function(assert) {
  const envPath = Globals.path('gis/environment.txt');
  assert.contains(envPath, 'environment.txt');
  assert.eq(Globals.fileExists('gis/environment.txt'), true, 'existing fixture file');
  assert.eq(Globals.fileExists('gis/does_not_exist_xyz.txt'), false, 'missing fixture file');

  const f = tempFile('js_text_roundtrip.txt');
  const payload = 'line1\nvalue=42\n';
  Globals.saveTextFile(f, payload);
  assert.eq(Globals.fileExists(f), true, 'saved text file exists');
  assert.eq(Globals.loadTextFile(f), payload, 'loaded text matches saved');

  assert.throws(function() {
    Globals.loadTextFile(tempFile('missing_text_file_xyz.txt'));
  }, 'does not exist');
});

Test.add('Globals.random() stays within bounds and varies', function(assert) {
  const seen = {};
  let distinct = 0;
  for (let i = 0; i < 25; ++i) {
    const r = Globals.random(10, 25);
    assert.gte(r, 10, 'random >= lower bound');
    assert.lte(r, 25, 'random <= upper bound');
    const k = r.toFixed(6);
    if (!seen[k]) {
      seen[k] = true;
      distinct++;
    }
  }
  assert.gt(distinct, 5, 'Globals.random produces varied draws');
});

Test.add('Globals.loadModule() imports ES module', function(assert) {
  const modPath = tempFile('calc_helper_mod.mjs');
  Globals.saveTextFile(modPath, 'export function sumSquares(a, b) { return a * a + b * b; }\nexport const tag = "iland-esm";\n');
  Globals.loadModule('CalcHelperMod', modPath);
  assert.eq(CalcHelperMod.sumSquares(3, 4), 25, 'ESM function call');
  assert.eq(CalcHelperMod.tag, 'iland-esm', 'ESM exported constant');
});

Test.add('Globals.resourceUnit() exposes valid RUs and expressions', function(assert) {
  for (let i = 0; i < Globals.resourceUnitCount; ++i) {
    const ru = Globals.resourceUnit(i);
    assert.eq(ru.valid, true, 'RU ' + i + ' valid');
    assert.eq(ru.index, i, 'RU index matches loop');
    assert.eq(ru.id, i + 1, 'bgd_mini RU id = index + 1');
    assert.gt(ru.info().length, 0, 'RU info non-empty');
    assert.gt(ru.expr('soilDepth'), 0, 'RU soilDepth > 0');
  }

  // Verify stocked vs sapling-only RUs from bgd_mini fixture (resourceUnit reuses singleton wrapper)
  const ruRef = Globals.resourceUnit(0);
  const ba0 = ruRef.expr('basalArea');
  const ba2 = Globals.resourceUnit(2).expr('basalArea'); // rid=3 (stand 3, saplings only)
  const ba7 = Globals.resourceUnit(7).expr('basalArea'); // rid=8 (stand 6, saplings only)
  assert.eq(ruRef.index, 7, 'Globals.resourceUnit reuses singleton wrapper');
  assert.gt(ba0, 0, 'stocked RU 0 has adult basal area');
  assert.eq(ba2, 0, 'sapling-only RU 2 has 0 adult basal area');
  assert.eq(ba7, 0, 'sapling-only RU 7 has 0 adult basal area');
});

Test.add('CSVFile new/Factory parsing, editing, and saving', function(assert) {
  const csvPath = tempFile('js_sample.csv');
  Globals.saveTextFile(csvPath, '# comment line\nspecies;dbh;note\npiab;24.5;spruce\nfasy;31;beech\n');

  const c1 = new CSVFile();
  assert.eq(c1.loadFile(csvPath), true, 'loadFile returns true');
  assert.eq(c1.rowCount, 2, 'rowCount');
  assert.eq(c1.colCount, 3, 'colCount');
  assert.eq(c1.captions, true, 'captions property default');
  assert.eq(c1.columnName(0), 'species');
  assert.eq(c1.columnIndex('dbh'), 1);
  assert.eq(c1.columnIndex('missing'), -1);
  assert.eq(c1.value(0, 'species'), 'piab');
  assert.eq(typeof c1.jsValue(0, 'dbh'), 'number');
  assert.approx(c1.jsValue(0, 'dbh'), 24.5, 1e-9);
  assert.eq(c1.jsValue(1, 'note'), 'beech');

  const row0 = c1.row(0);
  assert.ok(row0 !== undefined && row0 !== null, 'row(0) returns row data');

  c1.setValue(0, 1, '99.25');
  const outCsv = tempFile('js_sample_out.csv');
  c1.saveFile(outCsv);

  const c2 = Factory.newCSVFile(outCsv);
  assert.eq(c2.rowCount, 2, 'Factory CSV reload rowCount');
  assert.approx(c2.jsValue(0, 'dbh'), 99.25, 1e-9, 'edited value persisted');
});

Test.add('DBHDistribution calculates landscape and stand distributions', function(assert) {
  const d = Factory.newDBHDistribution();
  d.calculateFromLandscape();
  const landscapeFile = tempFile('dbh_landscape.txt');
  d.saveToTextFile(landscapeFile);
  assert.eq(Globals.fileExists(landscapeFile), true, 'landscape DBH file created');
  assert.gt(Globals.loadTextFile(landscapeFile).length, 10, 'landscape DBH file non-empty');

  d.addStand(1);
  d.addStand(2);
  const standInfoFile = tempFile('dbh_stands.txt');
  d.saveStandInfo(standInfoFile);
  assert.eq(Globals.fileExists(standInfoFile), true, 'stand info file created');
  assert.gt(Globals.loadTextFile(standInfoFile).length, 10, 'stand info file non-empty');
});

Test.add('ClimateConverter instantiates and exposes properties', function(assert) {
  const cc = new ClimateConverter();
  cc.tableName = 'climate_test';
  cc.captions = true;
  cc.year = 'yr';
  assert.eq(cc.tableName, 'climate_test');
  assert.eq(cc.captions, true);
  assert.eq(cc.year, 'yr');

  const cc2 = Factory.newClimateConverter();
  cc2.tableName = 'factory_clim';
  assert.eq(cc2.tableName, 'factory_clim');
});

function onAfterCreate() {
  Test.run();
}
