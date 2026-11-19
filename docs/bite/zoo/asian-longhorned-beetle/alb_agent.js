var BarkBeetle = new BiteAgent({
    name: 'Asian longhorned beetle - Anolophora glabripennis', description: "Stem boring insect",
    cellSize: 10,
    lifecycle: new BiteLifeCycle({
        voltinism: '1', // generations per year
        dieAfterDispersal: false,
        spreadFilter: 'yearsLiving>0', // only symptomatic cells spread
        spreadDelay: 1,
        spreadInterval: 1,
        mortality: function(cell) { // mortality depends on host tree presence
            var Ntrees = cell.trees.sum('1');
            var yrl = cell.value('yearsLiving');
            if (Ntrees < 1 & yrl > 2) {
                return true;
            } else {
                return false;
            }
        }
    }),

    dispersal: new BiteDispersal({
        kernel: 'exp(-0.1478*sqrt(x))', // dispersal kernel from Smith et al 2001
        debugKernel: 'temp/kerneltest_alb.asc',
        maxDistance: 1000,
        onBeforeSpread: function(bit) {
            if (Globals.year < 2) {
                randomSpread(1, bit.grid);
                console.log("added 1 px");
            }
        }
    }),

    colonization: new BiteColonization({
        dispersalFilter: 'rnd(0,1) < dispersalGrid',
        treeFilter: 'species=acps AND dbh>=7.5' // Sycamore maple >= 7.5 cm DBH (Dodds & Orwig 2011)
    }),

    impact: new BiteImpact({
        impact: [
            { treeFilter: 'dbh>=7.5', target: 'tree', fractionOfTrees: 'min(yearsLiving*0.002,0.02)', order: '-dbh' }
        ],
        onImpact: function(cell) {
            console.log('Impact on stand: ' + cell.value('standId'));
            ABELink(cell.value('standId'), cell.value('yearsLiving'), 3); // accumulate impact
        }
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving']
    }),

    onSetup: function(agent) { // load standIds to track which stand each BITE cell falls into
        agent.addVariable('tfirst');
        var grid = Factory.newGrid();
        grid.load('gis/bite.stands.asc');
        agent.addVariable(grid, 'standId');
    },

    onYearEnd: function(agent) {
        agent.updateVariable('tfirst', function(cell) {
            if (cell.value('tfirst') > 0) return cell.value('tfirst') + 1;
            if (cell.cumYearsLiving == 1) return 1;
            return 0;
        });
        agent.saveGrid('yearsLiving', 'temp/alb_yliv.asc');
        agent.saveGrid('cumYearsLiving', 'temp/alb_cyliv.asc');
        agent.saveGrid('tfirst', 'temp/alb_tfirst.asc');
        agent.saveGrid('active', 'temp/alb_active.asc');
    }
});

function randomSpread(n, gr) {
    for (var i = 0; i < n; ++i) {
        var x = Math.random() * gr.width;
        var y = Math.random() * gr.height;
        gr.setValue(250, 250, 1);
    }
}

// Management Link (to ABE)
var stand_damage = {};

// Accumulate damage for stand 'standId'
function ABELink(standId, damage, threshold) {
    if (!(standId in stand_damage))
        stand_damage[standId] = 0;
    stand_damage[standId] += damage;

    if (stand_damage[standId] > threshold) {
        console.log("accumulated damage for stand " + standId + " >1: " + stand_damage[standId] + "; starting management");
        fmengine.runActivity(standId, 'disturbance_response');
        stand_damage[standId] = 0; // reset damage counter
    }
}
