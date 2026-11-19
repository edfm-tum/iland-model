var GypsyMoth = new BiteAgent({
    name: "Gypsy Moth", description: "defoliator",
    cellSize: 10,
    climateVariables: ['GDD10'],
    lifecycle: new BiteLifeCycle({
        voltinism: 1, // max 1 generation per year, growth rate adjusts the true population growth
        dieAfterDispersal: false, // cell does not die even with dispersal
        spreadFilter: 'yearsLiving>0', // condition that needs to be met before spread
        spreadDelay: 1, // number of years after colonization
        spreadInterval: 1, // min. frequency of spread (1/years)
        outbreakStart: 'rnd(8,12)', // random number of years for outbreak interval
        outbreakDuration: 'rnd(2,4)' // random number for outbreak duration
    }),

    dispersal: new BiteDispersal({
        kernel: '(1/3.141592*21.32637^2)*exp(-(x^2/21.32637^2))', // Gaussian kernel (Elderd et al 2013)
        debugKernel: 'temp/kerneltest_gypsymoth.asc',
        maxDistance: 50,
        onBeforeSpread: function(bit) {
            if (Globals.year < 2) {
                randomSpread(1, bit.grid);
                console.log("added 1 px");
            }
        }
    }),

    colonization: new BiteColonization({
        dispersalFilter: 'rnd(0,1) < dispersalGrid',
        cellFilter: 'GDD10>500', // > 500 degree days (base 10C)
        treeFilter: 'species=quro', // Common oak (Quercus robur)
        initialAgentBiomass: 250 / 100 * 0.0016 // 250 ind/ha * 1.6 g per moth
    }),

    growth: new BiteBiomass({
        hostTrees: 'species=quro',
        hostBiomass: function(cell) {
            cell.reloadSaplings(); // trees load automatically, saplings require reloadSaplings()
            cell.saplings.filter('species=quro');
            var bm_saplings = cell.saplings.sum('nrep*foliagemass');
            var bm_trees = cell.trees.sum('foliagemass');
            return bm_saplings + bm_trees;
        },
        mortality: function(cell) {
            var oy = cell.value('outbreakYears');
            if (oy == 0) {
                return 0;
            } else {
                // antagonist mortality rises during outbreaks
                var death = (0.95 / (1 + Math.exp(-(oy - 2))));
                return death;
            }
        },
        growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))',
        growthRateFunction: function(cell) {
            var oy = cell.value('outbreakYears');
            if (oy == 0) {
                return 0;
            } else {
                var grate = (1.3 / (1 + Math.exp(-(oy - 1))));
                return grate;
            }
        },
        consumption: (0.004 * 520 * 0.5 * 0.1) / 0.0016, // 4 g/larva, 520 eggs, 5% larval survival, 1.6 g adult
        growthIterations: 1
    }),

    impact: new BiteImpact({
        impactFilter: 'agentImpact>0',
        impact: [
            { target: 'foliage', maxBiomass: 'agentImpact', order: "height" }
        ]
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving', 'hostBiomass', 'agentImpact', 'agentBiomass']
    }),

    onYearEnd: function(agent) {
        agent.saveGrid('yearsLiving', 'temp/gypsymoth_yliv.asc');
        agent.saveGrid('cumYearsLiving', 'temp/gypsymoth_cyliv.asc');
        agent.saveGrid('active', 'temp/gypsymoth_active.asc');
    }
});

function randomSpread(n, gr) {
    for (var i = 0; i < n; ++i) {
        var x = Math.random() * gr.width;
        var y = Math.random() * gr.height;
        gr.setValue(250, 250, 1);
    }
}
