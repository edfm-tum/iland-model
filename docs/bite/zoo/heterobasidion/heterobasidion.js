var Heterobasidion = new BiteAgent({
    name: "Heterobasidion root rot", description: "Root disease",
    cellSize: 20,
    climateVariables: ['MAT'], // mean annual temperature as climate variable for the BiteAgent items
    lifecycle: new BiteLifeCycle({
        voltinism: 1, // max 1 generation per year, growth rate adjusts the true population growth
        dieAfterDispersal: false, // cell does not die even with dispersal
        spreadFilter: 'yearsLiving>1', // condition that needs to be met before spread
        spreadDelay: 1, // number of years after colonization
        spreadInterval: 1 // min. frequency of spread (1/years)
    }),

    dispersal: new BiteDispersal({
        kernel: 'min((x)^-1.5,1)', // simple power-law kernel modified from Kallio 1970, Möykkynen et al 1997
        debugKernel: 'temp/kerneltest_heterobasidion.asc', // raster file for debugging the kernel function
        maxDistance: 1000, // maximum spreading distance is 1000m
        onBeforeSpread: function (bit) {
            if (Globals.year < 2) {
                randomSpread(1, bit.grid);
                console.log("added 1 px");
            }
        } // introduce the agent on first simulation year
    }),

    colonization: new BiteColonization({
        dispersalFilter: 'rnd(0,1) < (dispersalGrid)+0.005', // stochastic dispersal + 0.5% long-distance dispersal
        cellFilter: 'stumpBasalArea>0', // spores need freshly cut stumpBasalArea for colonization
        initialAgentBiomass: 0.01 // low initial biomass from tiny spores
    }),

    growth: new BiteBiomass({
        hostTrees: 'species=pisy', // define the host trees to be Scots pine (Pinus sylvestris)
        hostBiomass: function (cell) {
            if (cell.value('yearsLiving') < 10) {
                // first 10 years: only root mass of stumps available for consumption
                var bm_stumps = cell.value('stumpRootMass');
                return bm_stumps;
            } else {
                // after 10 years: living pine trees can also be infected and consumed
                var bm_stumps = cell.value('stumpRootMass');
                var bm_trees = cell.trees.sum('rootmass');
                var rootMass = bm_stumps + bm_trees;
                return rootMass;
            }
        },
        mortality: 0, // no additional mortality
        growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))', // logistic growth function
        growthRateFunction: '((6.52*MAT+(4.29*100-6.52*19))/100)', // temperature-related growth rate (Honkaniemi et al 2017)
        consumption: '(6.52*MAT+(4.29*100-6.52*19))/100',
        growthIterations: 10
    }),

    impact: new BiteImpact({
        impactFilter: 'yearsLiving>10', // impact occurs once colonized > 10 years
        impact: [
            {
                target: 'roots',
                maxBiomass: function (cell) {
                    var agentimp = cell.value('agentImpact') - ((3.141592 * cell.value('stumpBasalArea') * 0.5) / 3);
                    return agentimp;
                },
                fineRootFactor: 1,
                order: 'mod(x+10,20)^2 + mod(y+10,20)^2' // outward from cell center
            }
        ]
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving', 'MAT', 'stumpBasalArea']
    }),

    onYearEnd: function (agent) {
        agent.saveGrid('yearsLiving', 'temp/heterobasidion_yliv.asc');
        agent.saveGrid('cumYearsLiving', 'temp/heterobasidion_cyliv.asc');
        agent.saveGrid('active', 'temp/heterobasidion_active.asc');
        agent.saveGrid('stumpBasalArea', 'temp/heterobasidion_baharvest.asc');
        agent.updateVariable('stumpBasalArea', 0); // reset the grid
    },

    onTreeRemoved: function (cell, tree, reason) {
        if (reason == Tree.RemovedHarvest) {
            var ba = cell.agent.exprBasalArea.value(tree);
            var rm = cell.agent.exprRootMass.value(tree);
            var sd = cell.agent.exprDensity.value(tree);
            cell.setValue('stumpBasalArea', cell.value('stumpBasalArea') + ba);
            cell.setValue('stumpRootMass', cell.value('stumpRootMass') + rm);
            cell.setValue('stumpDensity', cell.value('stumpDensity') + sd);
        }
    },

    onSetup: function (agent) {
        agent.onTreeRemovedFilter = Tree.RemovedHarvest | Tree.RemovedDisturbance;
        agent.addVariable('stumpBasalArea');
        agent.addVariable('stumpRootMass');
        agent.addVariable('stumpDensity');
        agent.exprBasalArea = new TreeExpr("basalarea");
        agent.exprRootMass = new TreeExpr("rootmass");
        agent.exprDensity = new TreeExpr('1');
    }
});

function randomSpread(n, gr) {
    for (var i = 0; i < n; ++i) {
        var x = Math.random() * gr.width;
        var y = Math.random() * gr.height;
        gr.setValue(125, 125, 1);
    }
}
