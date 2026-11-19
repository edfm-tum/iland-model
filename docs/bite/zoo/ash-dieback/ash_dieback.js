var ash_dieback = new BiteAgent({
    name: "Ash dieback - Hymenoscyphus fraxineus", description: "Fungal disease",
    cellSize: 50,
    lifecycle: new BiteLifeCycle({
        voltinism: 1, // max 1 generation per year
        dieAfterDispersal: false, // cell does not die after dispersal
        spreadDelay: 1, // number of years after colonization
        spreadInterval: 1, // min. frequency of spread (1/years)
        spreadFilter: 'yearsLiving>0', // only symptomatic cells spread
        mortality: function(cell) { // agent and cell mortality depends on tree density
            var Ntrees = cell.trees.sum('1');
            var density = (Ntrees * 10000) / 2500; // stems per ha (50 m cell)
            var yrl = cell.value('yearsLiving');
            if (density < 100 & yrl > 3) { // density < 100 trees/ha after year 3
                return true;
            } else {
                return false;
            }
        }
    }),

    dispersal: new BiteDispersal({
        kernel: '(((3.30-1)*(3.30-2))/(2*3.141592*206.26)^2*(1+x/206.26)^(-3.30))*1000', // Inverse power-law kernel (Grosdidier et al 2018)
        debugKernel: 'temp/kerneltest_ash.asc',
        maxDistance: 1500,
        onBeforeSpread: function(bit) {
            if (Globals.year < 2) {
                randomSpread(1, bit.grid);
                console.log("added 1 px");
            }
        }
    }),

    colonization: new BiteColonization({
        dispersalFilter: 'rnd(0,1) < dispersalGrid',
        treeFilter: 'species=frex', // European ash trees
        saplingFilter: 'species=frex' // European ash saplings
    }),

    /* Defoliation impact from Timmermann et al 2017.
       Logistic curves visually fitted to the heaviest defoliation class (50-100%). */
    impact: new BiteImpact({
        impactFilter: 'yearsLiving>1', // impact occurs after first year since colonization
        impact: [
            {
                treeFilter: 'mod(id, 100)<>42', // 1% of population assumed resistant
                target: 'foliage',
                fractionPerTree: 'rnd(0.5,1)',
                fractionOfTrees: '0.3/(1+exp(-(yearsLiving-2.5)))',
                order: 'height'
            },
            {
                target: 'sapling',
                fractionOfTrees: '0.8/(1+exp(-(yearsLiving-6)))',
                order: 'dbh'
            }
        ]
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving']
    }),

    onYearEnd: function(agent) {
        agent.saveGrid('yearsLiving', 'temp/ashdieback_yliv.asc');
        agent.saveGrid('cumYearsLiving', 'temp/ashdieback_cyliv.asc');
        agent.saveGrid('active', 'temp/ashdieback_active.asc');
    }
});

function randomSpread(n, gr) {
    for (var i = 0; i < n; ++i) {
        var x = Math.random() * gr.width;
        var y = Math.random() * gr.height;
        gr.setValue(50, 50, 1);
    }
}
