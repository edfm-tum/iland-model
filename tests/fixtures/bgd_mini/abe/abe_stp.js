// Self-contained ABE silvicultural programs for bgd_mini irregular-stand tests.
// (Note: :/abe-lib/ is also embedded in both GUI iland and headless ilandc via iland_core.pri.)

var stpThin = {
  U: [100, 100, 100],

  thinning1: {
    type: 'thinning',
    thinning: 'custom',
    schedule: { min: 55, opt: 62, max: 70, force: true },
    targetVariable: 'basalarea',
    targetRelative: true,
    targetValue: 25,
    minDbh: 10,
    classes: [35, 30, 20, 10, 5]
  },

  clearcut1: {
    type: 'scheduled',
    schedule: { minRel: 0.95, optRel: 1.0, maxRel: 1.05, force: true },
    onEvaluate: function() {
      stand.trees.loadAll();
      return stand.trees.harvest('dbh > 15') > 0;
    },
    onCreate: function(act) {
      act.finalHarvest = true;
    }
  }
};

var stpRegen = {
  U: [100, 120, 120],

  planting1: {
    type: 'planting',
    schedule: { min: 8, opt: 10, max: 14, force: true },
    items: [
      { species: 'abal', fraction: 0.30, height: 0.35, age: 3, clear: false },
      { species: 'fasy', fraction: 0.30, height: 0.40, age: 3, clear: false }
    ]
  }
};

fmengine.addManagement(stpThin, 'stp_thin');
fmengine.addManagement(stpRegen, 'stp_regen');

fmengine.addAgent({
  scheduler: { enabled: false },
  stp: {
    'default': 'stp_thin',
    'stp_thin': 'stp_thin',
    'stp_regen': 'stp_regen'
  }
}, 'bgd_owner');
