// JS API Test Suite: ABE (fmengine, stand, unit, stp, patches, scheduler, simulation, abe-lib)
Globals.include('../../js/harness.js');
Test.suite('js_abe_api');

let __setupOk = false;
let __evalCalls = 0;
let __immediateEvalDelta = 0;
let __execCalls = 0;
let __immediateExecDelta = 0;
let __initStandArea = 0;
let __initStandBA = 0;
let __initUnitHarvestMode = '';
let __initUnitU = 0;
let __initUnitMAI = -1;
let __initStpName = '';
let __initActCount = 0;
let __initActNames = [];
let __initTimberIdx = 0;
let __patchRegCount = 0;
let __patchRegLif = -1;
let __patchExtCount = 0;
let __patchStripCount = 0;
let __plantingSapBefore = 0;
let __plantingSapAfter = 0;

function onAfterCreate() {
  fmengine.standId = 1;
  __initStandArea = stand.area;
  __initStandBA = stand.basalArea;
  __initUnitHarvestMode = stand.unit.harvestMode;
  __initUnitU = stand.unit.U;
  __initUnitMAI = stand.unit.landscapeMAI;
  __initStpName = stand.stp.name;
  __initActCount = stand.stp.activityCount;
  __initActNames = stand.stp.activityNames;
  __initTimberIdx = simulation.timberPriceIndex;

  // Persistent stand flags and custom JS object verified again in onFinish()
  stand.setFlag('api_marker', 'persisted-ok');
  stand.obj = { tag: 777, label: 'stand-1' };

  // Patches API on Stand 2
  fmengine.standId = 2;
  stand.patches.clear();
  const reg = stand.patches.createRegular(1, 2);
  stand.patches.list = reg;
  stand.patches.updateGrid();
  __patchRegCount = stand.patches.list.length;
  if (__patchRegCount > 0) {
    const firstPatch = stand.patches.list[0];
    __patchRegLif = stand.patches.lif(firstPatch);
    __patchExtCount = stand.patches.createExtendedPatch(firstPatch.id, 99);
  }
  stand.patches.clear();
  stand.patches.list = stand.patches.createStrips(20, true);
  stand.patches.updateGrid();
  __patchStripCount = stand.patches.list.length;
  stand.patches.clear();

  // Programmatic STP registration + forced activity execution on Stand 5
  const customStp = {
    U: [90, 90, 90],
    probeAct: {
      type: 'scheduled',
      schedule: { min: 1, opt: 1, max: 5, force: true },
      onEvaluate: function() {
        __evalCalls++;
        return 1.0;
      },
      onExecute: function() {
        __execCalls++;
      }
    }
  };
  fmengine.addManagement(customStp, 'js_custom_stp');
  fmengine.addManagementToAgentType('js_custom_stp', 'bgd_owner');
  fmengine.standId = 5;
  stand.setSTP('js_custom_stp');
  const evalBeforeRun = __evalCalls;
  fmengine.runActivityEvaluate(5, 'probeAct');
  __immediateEvalDelta = __evalCalls - evalBeforeRun;
  const execBeforeRun = __execCalls;
  fmengine.runActivity(5, 'probeAct');
  __immediateExecDelta = __execCalls - execBeforeRun;

  // Direct planting injection via fmengine.runPlanting() on Stand 3
  const sapCheck = new SaplingList();
  __plantingSapBefore = sapCheck.loadFromStand(3, 'species = pisy');
  fmengine.runPlanting(3, { species: 'pisy', fraction: 0.25, height: 0.3, age: 2 });
  __plantingSapAfter = sapCheck.loadFromStand(3, 'species = pisy');

  __setupOk = true;
}

Test.add('fmengine exposes enabled state, stands, and STPs', function(assert) {
  assert.eq(__setupOk, true, 'onAfterCreate completed');
  assert.eq(fmengine.enabled, true, 'ABE enabled');
  assert.eq(fmengine.isValidStand(1), true);
  assert.eq(fmengine.isValidStand(999), false);
  assert.eq(fmengine.standIds.length, 6, 'bgd_mini has 6 polygon stands');

  assert.contains(fmengine.stpNames, 'stp_thin');
  assert.contains(fmengine.stpNames, 'stp_regen');
  assert.contains(fmengine.stpNames, 'js_custom_stp');
  assert.eq(fmengine.isValidStp('stp_thin'), true);
  assert.eq(fmengine.isValidStp('nonexistent_stp'), false);
});

Test.add('StandObj, UnitObj, STPObj, Scheduler, and SimulationObj properties', function(assert) {
  fmengine.standId = 1;
  assert.eq(stand.id, 1);
  assert.gt(__initStandArea, 0);
  assert.gt(__initStandBA, 0);
  assert.gt(stand.volume, 0);
  assert.gt(stand.dbh, 0);
  assert.gt(stand.height, 0);
  assert.gt(stand.topHeight, 0);
  assert.approx(stand.topHeight, stand.height, 5.0);
  assert.gt(stand.nspecies, 0);
  assert.gt(stand.trees.loadAll(), 0, 'stand.trees loaded');

  let relSum = 0;
  for (let i = 0; i < stand.nspecies; ++i) {
    relSum += stand.relSpeciesBasalArea(i);
  }
  assert.approx(relSum, 1.0, 1e-4, 'relative species basal areas sum to 1');

  assert.neq(__initUnitHarvestMode, 'invalid', 'UnitObj harvestMode valid');
  assert.gt(__initUnitU, 0, 'UnitObj rotation U > 0');
  assert.gte(__initUnitMAI, 0, 'UnitObj landscapeMAI >= 0');

  assert.eq(__initStpName, 'stp_thin');
  assert.eq(__initActCount, 2, 'stp_thin has 2 activities');
  assert.contains(__initActNames, 'thinning1');
  assert.contains(__initActNames, 'clearcut1');

  assert.eq(scheduler.enabled, false, 'bgd_owner scheduler is disabled');
  assert.gt(__initTimberIdx, 0, 'simulation.timberPriceIndex > 0');
});

Test.add('Stand flags and custom JS obj persist across simulated years', function(assert) {
  fmengine.standId = 1;
  assert.eq(stand.flag('api_marker'), 'persisted-ok');
  assert.ok(stand.obj !== undefined && stand.obj !== null);
  assert.eq(stand.obj.tag, 777);
  assert.eq(stand.obj.label, 'stand-1');
});

Test.add('stand.patches creates regular, extended, and strip patches', function(assert) {
  assert.gt(__patchRegCount, 0, 'createRegular produced patches');
  assert.gte(__patchRegLif, 0, 'patch LIF >= 0');
  assert.lte(__patchRegLif, 1.0, 'patch LIF <= 1');
  assert.gt(__patchExtCount, 0, 'createExtendedPatch grew neighboring cells');
  assert.gt(__patchStripCount, 0, 'createStrips produced strip patches');
  fmengine.standId = 2;
  assert.eq(stand.patches.list.length, 0, 'clear() emptied patch list');
});

Test.add('Programmatic STP registration, activity execution, and runPlanting', function(assert) {
  assert.eq(__immediateEvalDelta, 1, 'runActivityEvaluate invoked probeAct onEvaluate once');
  assert.eq(__immediateExecDelta, 1, 'runActivity invoked probeAct onExecute once');
  assert.gte(__evalCalls, 1, 'probeAct onEvaluate total calls >= 1');
  assert.gte(__execCalls, 1, 'probeAct onExecute total calls >= 1');
  assert.eq(__plantingSapBefore, 0, 'Stand 3 initially had no pisy saplings');
  assert.gt(__plantingSapAfter, 0, 'runPlanting established pisy saplings on Stand 3');
});

Test.add('Embedded :/abe-lib/ABE-library.js resource loads in ilandc', function(assert) {
  const libSrc = Globals.loadTextFile(':/abe-lib/ABE-library.js');
  assert.gt(libSrc.length, 100);
  assert.contains(libSrc, 'lib');
});

function onFinish() {
  Test.run();
}
