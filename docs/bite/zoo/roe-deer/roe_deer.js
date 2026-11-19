var roe_deer = new BiteAgent({
    name: "Roe deer", description: "Browser",
    cellSize: 1000,
    lifecycle: new BiteLifeCycle({
        voltinism: 1, // max 1 generation per year, growth rate adjusts the true population growth
        dieAfterDispersal: false, // cell does not die even with dispersal
        spreadFilter: 'agentBiomass>0', // condition that needs to be met before spread
        spreadDelay: 1, // number of years after colonization
        spreadInterval: 1 // min. frequency of spread (1/years)
    }),

    dispersal: new BiteDistribution({}), // agent is distributed evenly across the landscape

    colonization: new BiteColonization({
        dispersalFilter: 'dispersalGrid', // 100% chance of new colonization
        saplingFilter: 'species=abal and height<1.3', // Silver fir saplings < 1.3 m
        initialAgentBiomass: 14 * 24 // 14 animals per 100 ha * 24 kg average body mass
    }),

    growth: new BiteBiomass({
        hostTrees: '(species=abal) and height<1.3', // these trees are the hosts
        hostBiomass: function(cell) {
            cell.reloadSaplings(); // reload the sapling list
            cell.saplings.filter('species=abal and height<1.3');
            var bm_saplings = cell.saplings.sum('nrep*foliagemass');
            return bm_saplings;
        },
        mortality: 0, // no additional mortality
        growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))',
        growthRateFunction: '0', // equilibrium population
        consumption: 3.66, // kg agent / kg host (Drozdz & Osiecki 1973, Tixier & Duncan 1996)
        growthIterations: 10
    }),

    impact: new BiteImpact({
        impactFilter: 'agentImpact>0',
        impact: [
            { target: 'browsing', fractionOfTrees: 'agentImpact/hostBiomass' }
        ]
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving', 'hostBiomass', 'agentImpact', 'agentBiomass']
    }),

    onYearEnd: function(agent) {
        agent.saveGrid('yearsLiving', 'temp/roedeer_base.asc');
    }
});
