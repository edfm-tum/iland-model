// JS API Test Suite: BITE engine, BiteAgent, BiteCellScript, climate time series, and agent variables
Globals.include('../../js/harness.js');
Test.suite('js_bite_api');

let __setupCellInfo = null;

function onAfterCreate() {
  const ag = Bite.agent('test_bug');
  if (ag) {
    ag.addVariable('customVar');
    ag.updateVariable('customVar', 7.5);
    const c = ag.cell(2, 2);
    if (c) {
      const clim = c.dailyClimateTimeseries('tmean');
      const hasVar = c.hasValue('customVar');
      const v0 = c.value('customVar');
      c.setValue('customVar', 12.25);
      const v1 = c.value('customVar');
      c.active = true;
      c.spreading = true;
      __setupCellInfo = {
        x: c.x,
        y: c.y,
        hasVar: hasVar,
        v0: v0,
        v1: v1,
        climLen: clim.length,
        hasTrees: c.trees !== undefined && c.trees !== null,
        hasSaplings: c.saplings !== undefined && c.saplings !== null,
        hasDeadTrees: c.deadTrees !== undefined && c.deadTrees !== null,
        active: c.active,
        spreading: c.spreading
      };
    }
  }
}

Test.add('Bite singleton registers test_bug agent and geometry', function(assert) {
  assert.contains(Bite.agents, 'test_bug');
  const ag = Bite.agent('test_bug');
  assert.ok(ag !== null && ag !== undefined, 'Bite.agent returns agent');
  assert.eq(ag.name, 'test_bug');
  assert.eq(ag.cellSize, 20);
  assert.eq(ag.width, 20, '400m / 20m = 20 cells');
  assert.eq(ag.height, 10, '200m / 20m = 10 cells');
  assert.eq(ag.isCellValid(2, 2), true);
  assert.eq(ag.isCellValid(99, 99), false);
  assert.gt(ag.info().length, 0);
  assert.contains(ag.variables, 'customVar');
});

Test.add('BiteAgent variables and BiteCellScript accessors work', function(assert) {
  assert.ok(__setupCellInfo !== null, 'cell(2, 2) captured in onAfterCreate');
  assert.eq(__setupCellInfo.x, 2);
  assert.eq(__setupCellInfo.y, 2);
  assert.eq(__setupCellInfo.hasVar, true);
  assert.approx(__setupCellInfo.v0, 7.5, 1e-6);
  assert.approx(__setupCellInfo.v1, 12.25, 1e-6);
  assert.gte(__setupCellInfo.climLen, 365, 'daily tmean >= 365 days');
  assert.eq(__setupCellInfo.hasTrees, true);
  assert.eq(__setupCellInfo.hasSaplings, true);
  assert.eq(__setupCellInfo.hasDeadTrees, true);
  assert.eq(__setupCellInfo.active, true);
  assert.eq(__setupCellInfo.spreading, true);
});

Test.add('BITE onSetup/onCalculate callbacks fire and cells remain active across years', function(assert) {
  assert.eq(__biteStats.setupCalled, true, 'onSetup callback invoked');
  assert.gt(__biteStats.calcCalls, 0, 'onCalculate callback invoked during simulation');

  const ag = Bite.agent('test_bug');
  let activeCells = 0;
  let maxYearsLiving = 0;
  for (let ix = 0; ix < ag.width; ++ix) {
    for (let iy = 0; iy < ag.height; ++iy) {
      const c = ag.cell(ix, iy);
      if (c && c.active) {
        activeCells++;
        if (c.yearsLiving > maxYearsLiving) {
          maxYearsLiving = c.yearsLiving;
        }
      }
    }
  }
  assert.gt(activeCells, 0, 'BITE cells colonized/active');
  assert.gte(maxYearsLiving, 1, 'yearsLiving increments');
});

function onFinish() {
  Test.run();
}
