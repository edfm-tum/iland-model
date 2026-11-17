// JS API Test Suite: Wind, Fire, and BarkBeetle plugin script bindings & callbacks
Globals.include('../../js/harness.js');
Test.suite('js_disturbance_api');

let __windCallbackCount = 0;
let __bbCallbackCount = 0;
let __simWindUntouched = false;
let __realWindReduced = false;
let __fireHandlerRegistered = false;

function tempFile(name) {
  return Globals.defaultDirectory('temp') + name;
}

function onWindDone() {
  __windCallbackCount++;
}

function onBBDone() {
  __bbCallbackCount++;
}

function onAfterCreate() {
  // Check simulate=true vs simulate=false on Wind.windEvent() right after model creation
  const treesBefore = new TreeList();
  const nBefore = treesBefore.loadAll();

  Wind.initializeEdgeAge(5);
  Wind.windEvent(45, 270, 5, true); // simulate = true
  const nAfterSim = treesBefore.loadAll();
  __simWindUntouched = (nAfterSim === nBefore);

  Wind.windEvent(45, 270, 5, false); // real storm
  const nAfterReal = treesBefore.loadAll();
  __realWindReduced = (nAfterReal < nBefore);

  // Fire JS callbacks for year 2 prescribed ignition
  Fire.onCalculateFireSize = function(ruId, genSize) {
    return genSize;
  };
  __fireHandlerRegistered = (typeof Fire.onCalculateFireSize === 'function');

  Fire.onIgnition = function() {
    if (Globals.year === 2) {
      Fire.ignite(50, 50, 25000, 15, 270);
    }
  };
}

Test.add('Wind script grids, windEvent simulate vs real, damagedArea, and onAfterWind callback', function(assert) {
  assert.eq(__simWindUntouched, true, 'Wind.windEvent(simulate=true) preserves live trees');
  assert.eq(__realWindReduced, true, 'Wind.windEvent(simulate=false) kills trees');
  assert.gt(__windCallbackCount, 0, 'modules.wind.onAfterWind callback invoked');

  const gEdge = Wind.grid('edge');
  const gCws = Wind.grid('cwsUproot');
  assert.eq(gEdge.isValid, true);
  assert.eq(gCws.isValid, true);
  assert.eq(gEdge.width, 60);
  assert.eq(gEdge.height, 40);
  assert.eq(gEdge.cellsize, 10);

  const edgeFile = tempFile('js_wind_edge.asc');
  assert.eq(Wind.gridToFile('edge', edgeFile), true);
  assert.eq(Globals.fileExists(edgeFile), true);

  const dmgArea = Wind.damagedArea(0, tempFile('js_wind_dmg.asc'));
  assert.gte(dmgArea, 0);
});

Test.add('Fire.onIgnition prescribed fire sets Fire properties and exposes grids', function(assert) {
  assert.eq(__fireHandlerRegistered, true, 'Fire.onCalculateFireSize property round-trips');
  assert.gt(Fire.id, 0, 'Fire.ignite() recorded fire id in year 2');
  assert.gte(Fire.x, 0, 'Fire.x >= 0');
  assert.gte(Fire.y, 0, 'Fire.y >= 0');

  const gFireId = Fire.grid('fireID');
  const gKbdi = Fire.grid('KBDI');
  assert.eq(gFireId.isValid, true);
  assert.eq(gKbdi.isValid, true);

  const kbdiFile = tempFile('js_fire_kbdi.asc');
  assert.eq(Fire.gridToFile('kbdi', kbdiFile), true);
  assert.eq(Globals.fileExists(kbdiFile), true);
});

Test.add('BarkBeetle properties, simulate toggle, setInfested, grids, and onAfterBarkbeetle', function(assert) {
  assert.gt(__bbCallbackCount, 0, 'modules.barkbeetle.onAfterBarkbeetle invoked');
  const origEn = BarkBeetle.enabled;
  BarkBeetle.enabled = !origEn;
  assert.eq(BarkBeetle.enabled, !origEn, 'BarkBeetle.enabled toggle');
  BarkBeetle.enabled = origEn;

  const origSim = BarkBeetle.simulate;
  BarkBeetle.simulate = !origSim;
  assert.eq(BarkBeetle.simulate, !origSim, 'BarkBeetle.simulate toggle');
  BarkBeetle.simulate = origSim;

  BarkBeetle.setBackgroundInfestationProbability(0.05);
  assert.gte(BarkBeetle.generations(2, 2), 0, 'generations(2, 2) valid');

  const gInf = BarkBeetle.grid('infested');
  const gDead = BarkBeetle.grid('dead');
  assert.eq(gInf.isValid, true);
  assert.eq(gDead.isValid, true);

  const deadFile = tempFile('js_bb_dead.asc');
  assert.eq(BarkBeetle.gridToFile('dead', deadFile), true);
  assert.eq(Globals.fileExists(deadFile), true);

  assert.gte(BarkBeetle.damagedArea(0, ''), 0);
});

function onFinish() {
  Test.run();
}
