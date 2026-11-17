// JS API Test Suite: Global lifecycle hooks, startOutput/stopOutput, DeadTreeList, saveModelSnapshot
Globals.include('../../js/harness.js');
Test.suite('js_lifecycle_dead');

const __events = [];
let __stopOk = false;
let __startOk = false;
let __badStartOk = true;
let __badStopOk = true;

function tempFile(name) {
  return Globals.defaultDirectory('temp') + name;
}

function onBeforeCreate() {
  __events.push('beforeCreate:' + Globals.year);
}

function onAfterCreate() {
  __events.push('afterCreate:' + Globals.year);
  __stopOk = Globals.stopOutput('standdead');
  __startOk = Globals.startOutput('standdead');
  __badStartOk = Globals.startOutput('nonexistent_output_table_xyz');
  __badStopOk = Globals.stopOutput('nonexistent_output_table_xyz');
}

function onYearBegin() {
  __events.push('yearBegin:' + Globals.year);
}

function onYearEnd() {
  __events.push('yearEnd:' + Globals.year);
}

Test.add('Global lifecycle hooks fire in exact sequence and year order', function(assert) {
  const expected = ['beforeCreate:0', 'afterCreate:1'];
  for (let y = 1; y <= 6; ++y) {
    expected.push('yearBegin:' + y);
    expected.push('yearEnd:' + y);
  }
  assert.eq(__events.length, expected.length, 'hook event count for 6-year simulation');
  for (let i = 0; i < expected.length; ++i) {
    assert.eq(__events[i], expected[i], 'event[' + i + ']');
  }
});

Test.add('Globals.startOutput() and stopOutput() toggle valid outputs safely', function(assert) {
  assert.eq(__stopOk, true, 'stopOutput(standdead)');
  assert.eq(__startOk, true, 'startOutput(standdead)');
  assert.eq(__badStartOk, false, 'startOutput rejects unknown table');
  assert.eq(__badStopOk, false, 'stopOutput rejects unknown table');
});

Test.add('DeadTreeList loads Snags/DWD/Both, aggregates, filters, and removes', function(assert) {
  assert.neq(DeadTreeList.Snags, DeadTreeList.DWD);
  assert.neq(DeadTreeList.Both, DeadTreeList.Snags);

  let totalSnags = 0;
  let totalDWD = 0;
  for (let ru = 0; ru < Globals.resourceUnitCount; ++ru) {
    const dtSnag = new DeadTreeList();
    const dtDwd = new DeadTreeList();
    const dtBoth = new DeadTreeList();
    const nS = dtSnag.loadFromRU(ru, DeadTreeList.Snags, false);
    const nD = dtDwd.loadFromRU(ru, DeadTreeList.DWD, false);
    const nB = dtBoth.loadFromRU(ru, DeadTreeList.Both, false);
    assert.eq(nS + nD, nB, 'RU ' + ru + ' Snags + DWD == Both');
    totalSnags += nS;
    totalDWD += nD;
  }
  assert.gt(totalSnags, 0, 'landscape has standing snags by year 6');
  assert.gt(totalDWD, 0, 'landscape has downed deadwood by year 6');

  // RU 6 (Stand 5, rid=7) has downed deadwood by year 6 (including large spruce)
  const standDt = new DeadTreeList();
  const nStandBoth = standDt.loadFromStand(5, DeadTreeList.Both);
  assert.gt(nStandBoth, 0, 'Stand 5 has dead trees');
  assert.gte(standDt.mean('dbh'), 5.0, 'swdDBHSingle=5 tracks dead trees >= 5 cm');
  assert.gt(standDt.sum('biomass'), 0, 'deadwood biomass > 0');

  // Filter downed deadwood (snag = 0) and remove from Stand 5
  const nDowned = standDt.filter('snag = 0');
  assert.gt(nDowned, 0, 'Stand 5 has DWD items (snag = 0)');
  const removed = standDt.remove();
  assert.eq(removed, nDowned, 'remove() returns removed DWD count');

  const verifyDwd = new DeadTreeList();
  assert.eq(verifyDwd.loadFromStand(5, DeadTreeList.DWD), 0, 'all Stand 5 DWD removed');
});

Test.add('Globals.saveModelSnapshot() writes snapshot SQLite file', function(assert) {
  const snapPath = tempFile('js_lifecycle_snapshot.sqlite');
  assert.eq(Globals.saveModelSnapshot(snapPath), true, 'saveModelSnapshot succeeded');
  assert.eq(Globals.fileExists(snapPath), true, 'snapshot file exists on disk');
});

function onFinish() {
  Test.run();
}
