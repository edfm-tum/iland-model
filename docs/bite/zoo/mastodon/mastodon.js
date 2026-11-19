var mastodon = new BiteAgent({
    name: "Mastodon", description: "Extinct browser",
    cellSize: 1000, // maximum reasonable cellsize regarding vegetation aggregation
    lifecycle: new BiteLifeCycle({
        voltinism: 1,
        dieAfterDispersal: false,
        spreadFilter: 'agentBiomass>0',
        spreadDelay: 1,
        spreadInterval: 1
    }),

    dispersal: new BiteDistribution({}), // distributed evenly across the landscape

    colonization: new BiteColonization({
        dispersalFilter: 'dispersalGrid',
        treeFilter: 'species=piab and dbh<=15', // Norway spruce <= 15 cm DBH
        saplingFilter: 'species=piab',
        initialAgentBiomass: 12000 // 1.5 mastodons/100 ha * 8 t per individual
    }),

    growth: new BiteBiomass({
        hostTrees: '(species=piab and dbh<=15)',
        hostBiomass: function(cell) {
            cell.reloadSaplings();
            cell.saplings.filter('species=piab');
            var bm_saplings = cell.saplings.sum('nrep*foliagemass');
            var bm_trees = cell.trees.sum('foliagemass');
            return bm_saplings + bm_trees;
        },
        mortality: 0,
        growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))',
        growthRateFunction: '0.01', // 1% annual growth rate
        consumption: '(0.05*8000*365*0.2)/8000', // 5% body mass/day, 20% spruce diet
        growthIterations: 10
    }),

    impact: new BiteImpact({
        impactFilter: 'agentImpact>0',
        impact: [
            { target: 'browsing', fractionOfTrees: 'agentImpact/hostBiomass', order: 'height' },
            { treeFilter: 'dbh<=15', target: 'tree', fractionOfTrees: '0.05' }
        ]
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving', 'hostBiomass', 'agentImpact', 'agentBiomass']
    }),

    onYearEnd: function(agent) {
        agent.saveGrid('yearsLiving', 'temp/mastodon_base.asc');
    }
});
