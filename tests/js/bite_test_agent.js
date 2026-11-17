// BITE agent definition loaded via modules.bite.file for test_bite_api.js
var __biteStats = {
  setupCalled: false,
  calcCalls: 0,
  sampleCellValues: []
};

var testBugAgent = new BiteAgent({
  name: 'test_bug',
  description: 'Minimal BITE agent for JS API tests',
  cellSize: 20,
  climateVariables: ['MAT', 'GDD'],

  lifecycle: new BiteLifeCycle({
    voltinism: '1',
    spreadFilter: '1',
    spreadDelay: 0,
    spreadInterval: 1,
    dieAfterDispersal: false,
    outbreakStart: 1,
    outbreakDuration: 5,
    mortality: 0
  }),

  dispersal: new BiteDispersal({
    maxDistance: 60,
    kernel: 'exp(-x/20)',
    backgroundInfestationProbability: 0.5
  }),

  colonization: new BiteColonization({
    dispersalFilter: '1',
    treeFilter: 'dbh > 10',
    onSetup: function(item) {
      __biteStats.setupCalled = (item !== undefined && item !== null);
    },
    onCalculate: function(cell) {
      __biteStats.calcCalls++;
      if (__biteStats.sampleCellValues.length < 5) {
        __biteStats.sampleCellValues.push({
          x: cell.x,
          y: cell.y,
          val: cell.value('customVar')
        });
      }
      return 1;
    }
  }),

  impact: new BiteImpact({
    hostTrees: 'dbh > 10',
    simulate: true,
    impact: [
      { target: 'foliage', fractionPerTree: 0.05, maxTrees: 2, order: 'dbh' }
    ]
  }),

  output: new BiteOutput({
    tableName: 'bite_test_bug',
    columns: ['yearsLiving', 'active'],
    outputFilter: '1'
  })
});
