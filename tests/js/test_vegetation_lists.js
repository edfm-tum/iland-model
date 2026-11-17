// JS API Test Suite: TreeList, ScriptTree, TreeExpr, SaplingList, Globals.addSingleTrees/addSaplings
Globals.include('../../js/harness.js');
Test.suite('js_vegetation_lists');

Test.add('TreeList loading, filtering, aggregation, sorting, and tree references', function(assert) {
  const tl = new TreeList();
  const total = tl.loadAll();
  assert.gt(total, 100, 'bgd_mini initialized with >100 adult trees');
  assert.eq(tl.count, total);

  const totalBA = tl.sum('basalarea');
  const meanDbh = tl.mean('dbh');
  assert.gt(totalBA, 0);
  assert.gt(meanDbh, 0);

  // Filter subset
  const spruce = new TreeList();
  const nSpruce = spruce.load('species = piab and dbh > 20');
  assert.gt(nSpruce, 0);
  assert.lt(nSpruce, total);
  const nTallSpruce = spruce.filter('height > 15');
  assert.gt(nTallSpruce, 0);
  assert.lte(nTallSpruce, nSpruce);

  // loadFromList copy filter
  const beech = new TreeList();
  const nBeech = beech.loadFromList(tl, 'species = fasy');
  assert.gt(nBeech, 0);
  assert.eq(beech.count, nBeech);

  // Sort ascending / descending & percentile
  tl.sort('dbh');
  const smallest = tl.treeObject(0);
  const largest = tl.treeObject(tl.count - 1);
  assert.lte(smallest.dbh, largest.dbh, 'sort(dbh) ascending');
  const p10 = tl.percentile(10);
  const p90 = tl.percentile(90);
  assert.lte(p10, p90, 'percentile(10) <= percentile(90)');

  tl.sort('-dbh');
  assert.gte(tl.treeObject(0).dbh, tl.treeObject(tl.count - 1).dbh, 'sort(-dbh) descending');

  // randomize preserves count and sum
  tl.randomize();
  assert.eq(tl.count, total, 'randomize preserves count');
  assert.approx(tl.sum('basalarea'), totalBA, 1e-4, 'randomize preserves total BA');

  // tree() shared reference vs treeObject() independent snapshot + TreeExpr + flags
  tl.sort('dbh');
  const ref = tl.tree(0);
  const snap0 = tl.treeObject(0);
  const snapLast = tl.treeObject(tl.count - 1);
  assert.eq(ref.valid, true);
  assert.approx(ref.dbh, snap0.dbh, 1e-6);
  assert.gt(snapLast.dbh, snap0.dbh, 'snapLast > snap0 DBH');

  // Accessing ref again updates the same ScriptTree wrapper; snap0 remains unchanged
  const ref2 = tl.tree(tl.count - 1);
  assert.approx(ref2.dbh, snapLast.dbh, 1e-6);
  assert.approx(ref.dbh, snapLast.dbh, 1e-6, 'tree() reuses reference object');
  assert.lt(snap0.dbh, snapLast.dbh, 'treeObject() stays independent');

  const expr = new TreeExpr('dbh * height');
  assert.approx(expr.value(snapLast), snapLast.expr('dbh * height'), 1e-6, 'TreeExpr matches ScriptTree.expr');
  assert.approx(expr.value(snapLast), snapLast.dbh * snapLast.height, 1e-4);
  assert.gt(snapLast.info().length, 0);

  // Tree flags (set and restore cleanly)
  ref2.setFlag(Tree.TreeNoHarvest, true);
  assert.eq(ref2.flag(Tree.TreeNoHarvest), true, 'TreeNoHarvest flag set');
  ref2.setFlag(Tree.TreeNoHarvest, false);
  assert.eq(ref2.flag(Tree.TreeNoHarvest), false, 'TreeNoHarvest flag cleared');
});

Test.add('TreeList.spatialFilter() matches coordinate filter', function(assert) {
  const expectedList = new TreeList();
  const nExpected = expectedList.load('x < 200');
  assert.gt(nExpected, 0);

  const mask = new Grid();
  mask.create(4, 2, 100);
  mask.setOrigin(0, 0);
  mask.name = 'm';
  mask.clear();
  for (let iy = 0; iy < 2; ++iy) {
    mask.setValue(0, iy, 1); // x in [0, 100)
    mask.setValue(1, iy, 1); // x in [100, 200)
  }

  const spatialList = new TreeList();
  spatialList.loadAll();
  const nSpatial = spatialList.spatialFilter(mask, 'm = 1');
  assert.eq(nSpatial, nExpected, 'spatialFilter count matches load(x < 200)');
});

Test.add('TreeList simulate mode marks vs actual kill/harvest', function(assert) {
  const baseline = new TreeList();
  const initialTotal = baseline.loadAll();

  const sim = new TreeList();
  sim.loadAll();
  sim.simulate = true;
  assert.eq(sim.simulate, true);

  const marked = sim.harvest('dbh > 55');
  assert.gt(marked, 0, 'large trees marked in simulate mode');

  const checkLive = new TreeList();
  assert.eq(checkLive.loadAll(), initialTotal, 'simulate=true must not remove live trees');
  assert.approx(checkLive.sum('markharvest'), marked, 1e-6, 'markharvest flag set on marked trees');

  checkLive.resetMarks();
  assert.approx(checkLive.sum('markharvest'), 0, 1e-6, 'resetMarks() clears markharvest');

  // Actual removal with simulate=false on a small well-defined subset
  const act = new TreeList();
  const toKill = act.load('x < 100 and dbh > 60');
  assert.gt(toKill, 0, 'trees present in target kill subset');
  const killed = act.kill();
  assert.eq(killed, toKill, 'kill() removed exact filtered count');
  assert.eq(baseline.loadAll(), initialTotal - killed, 'live count reduced by exact killed count');
});

Test.add('SaplingList loading, filtering, browsing, and killing', function(assert) {
  const s3 = new SaplingList();
  const n3 = s3.loadFromStand(3);
  assert.gt(n3, 0, 'Stand 3 has initialized saplings');

  const s6 = new SaplingList();
  const n6 = s6.loadFromStand(6);
  assert.gt(n6, 0, 'Stand 6 has initialized saplings');

  // append = true
  const sBoth = new SaplingList();
  sBoth.loadFromStand(3, '', false);
  const totalBoth = sBoth.loadFromStand(6, '', true);
  assert.eq(totalBoth, n3 + n6, 'append=true combines both stands');
  assert.gt(sBoth.sum('height'), 0);

  const nSpruce = sBoth.filter('species = piab and height > 1.0');
  assert.gt(nSpruce, 0);
  assert.lt(nSpruce, totalBoth);

  // Browsing mark and reset
  const sb = new SaplingList();
  const nb = sb.loadFromStand(6);
  assert.eq(sb.browse(true), nb);
  assert.eq(sb.browse(false), nb);

  // Kill one sapling species (soau) on Stand 6
  const sk = new SaplingList();
  const nSoau = sk.loadFromStand(6, 'species = soau');
  assert.gt(nSoau, 0, 'Stand 6 has soau saplings');
  assert.eq(sk.kill(), nSoau);

  const verify = new SaplingList();
  assert.eq(verify.loadFromStand(6, 'species = soau'), 0, 'soau cohorts removed');
});

Test.add('Globals.addSingleTrees(), addSaplings(), and removeSaplings()', function(assert) {
  // RU 2 (rid = 3, Stand 3) initially has 0 adult trees
  const checkRu2 = new TreeList();
  assert.eq(checkRu2.load('ruindex = 2'), 0, 'RU 2 initially free of adult trees');

  const added = Globals.addSingleTrees(
    2,
    'x;y;species;dbh;height;age\n10.0;10.0;piab;25.0;20.0;60\n20.0;15.0;fasy;30.0;22.0;70\n'
  );
  assert.eq(added, 2, 'addSingleTrees added 2 trees to RU 2');
  assert.eq(checkRu2.load('ruindex = 2'), 2, 'TreeList sees the 2 added trees');
  assert.approx(checkRu2.mean('dbh'), 27.5, 1e-4, 'added trees mean DBH');

  // Inject and remove saplings inside a 10x10m box on Stand 3
  const addedSapCells = Globals.addSaplings(-1, 200, 10, 10, 10, 'pisy', 0.5, 3);
  assert.gt(addedSapCells, 0, 'addSaplings populated cells');

  const sp = new SaplingList();
  assert.gt(sp.loadFromStand(3, 'species = pisy'), 0, 'pisy saplings present in Stand 3');
  Globals.removeSaplings(-1, 200, 10, 10, 10);
  assert.eq(sp.loadFromStand(3, 'species = pisy'), 0, 'removeSaplings cleared injected cells');
});

function onAfterCreate() {
  Test.run();
}
