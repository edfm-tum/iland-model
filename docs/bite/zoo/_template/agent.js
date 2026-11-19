// Starter BITE agent script for the BITE Zoo.
// Replace this template with your verified iLand BITE agent definition.

var example_agent = new BiteAgent({
    name: "Example Agent",
    description: "Short agent description",
    cellSize: 20,

    lifecycle: new BiteLifeCycle({
        voltinism: 1,
        dieAfterDispersal: false,
        spreadFilter: 'yearsLiving>0',
        spreadDelay: 1,
        spreadInterval: 1
    }),

    dispersal: new BiteDispersal({
        kernel: 'exp(-0.05*x)',
        maxDistance: 500
    }),

    colonization: new BiteColonization({
        dispersalFilter: 'rnd(0,1) < dispersalGrid',
        treeFilter: 'species=piab'
    }),

    impact: new BiteImpact({
        impactFilter: 'yearsLiving>0',
        impact: [
            { target: 'foliage', fractionPerTree: '0.25', fractionOfTrees: '0.5' }
        ]
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving']
    })
});
