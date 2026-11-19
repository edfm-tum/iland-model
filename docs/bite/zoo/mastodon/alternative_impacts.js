// Alternative Impact 1: Varying uprooting mortality by DBH class (2.5% -> 7.5%)
var mastodon_impact_dbh = new BiteImpact({
    impactFilter: 'agentImpact>0',
    impact: [
        { treeFilter: 'dbh<=5', target: 'tree', fractionOfTrees: '0.025' },
        { treeFilter: 'dbh>5 and dbh<=10', target: 'tree', fractionOfTrees: '0.05' },
        { treeFilter: 'dbh>10 and dbh<=15', target: 'tree', fractionOfTrees: '0.075' }
    ]
});

// Alternative Impact 2: Classifying by tree height instead of diameter
var mastodon_impact_height = new BiteImpact({
    impactFilter: 'agentImpact>0',
    impact: [
        { treeFilter: 'height<=8', target: 'tree', fractionOfTrees: '0.025' },
        { treeFilter: 'height>8 and height<=12', target: 'tree', fractionOfTrees: '0.05' },
        { treeFilter: 'height>12 and height<=15', target: 'tree', fractionOfTrees: '0.075' }
    ]
});

// Alternative Impact 3: Selective size window (8-12 cm DBH trees spared)
var mastodon_impact_selective = new BiteImpact({
    impactFilter: 'agentImpact>0',
    impact: [
        { treeFilter: 'dbh>5 AND dbh<=8', target: 'tree', fractionOfTrees: '0.05' },
        { treeFilter: 'dbh>12 AND dbh<=15', target: 'tree', fractionOfTrees: '0.05' }
    ]
});
