/* Selective thinning

*/

lib.thinning.selectiveThinning = function(options) {
    // 1. Default Options
    const defaultOptions = {
        schedule: undefined,
        id: 'selective_select_trees',
        mode: 'simple',
        SpeciesMode: 'simple',
        NTrees: 80,
        NCompetitors: 4,
        speciesSelectivity: {},
        ranking: 'height',
        repeatInterval: 5,
        repeatTimes: 5,
        constraint: undefined,

        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});


    // dynamic parameters of selective thinning
    function dynamic_NTrees() {
        // retrieve N from stand flag during runtime
        var value = stand.flag('NTrees');
        if (value === undefined) value = opts.NTrees;
        return value;
    };
    function dynamic_NCompetitors() {
        // retrieve N from stand flag during runtime
        //var value = stand.flag('NCompetitors');
        const Agefactor = Math.max(Math.min(1.0, -0.01*stand.age+1.2), 0.0);
        var value = Math.max(stand.flag('NCompetitors')*Agefactor, 1);
        if (value === undefined) value = opts.NCompetitors;
        return value;
    };
    function dynamic_ranking() {
        // retrieve ranking from stand flag during runtime
        var value = stand.flag('ranking');
        if (value === undefined) value = opts.ranking;
        return value;
    };

    function dynamic_speciesSelectivity() {
        // retrieve species Selectivity from stand flag during runtime
        var value = stand.flag('speciesSelectivity');
        if (value === undefined) value = opts.speciesSelectivity;
        return value;
    };

    // changing parameters if mode is dynamic
    if (opts.mode == 'dynamic') {
        opts.NTrees = dynamic_NTrees;
        opts.NCompetitors = dynamic_NCompetitors;
        opts.ranking = dynamic_ranking;
    };

    // changing parameters if SpeciesMode is dynamic
    if (opts.SpeciesMode == 'dynamic') {
        opts.speciesSelectivity = dynamic_speciesSelectivity;
        //opts.ranking = dynamic_ranking;
    };

    const program = {};

    const select_trees = {
        type: 'thinning',
        id: opts.id,
        schedule: opts.schedule,			//absolute: true, opt: 3},	//repeat: true, repeatInterval: 10
        thinning: 'selection',
        N: opts.NTrees,
        NCompetitors: opts.NCompetitors, // total competitors! not per thinning event
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.ranking,

        onEnter: function() {
            stand.obj.lib.selective_thinning_counter = 0;
        },

        onExit: function() {
            stand.stp.signal('selective_start_repeat');
            lib.activityLog('thinning_selection'); // details? target species?
        },
        description: `Selective thinning. Repeated ${opts.repeatTimes} times every ${opts.repeatInterval} years.`
    };

    program["selector"] = select_trees;
    const remove_trees = {
        type: 'general',
        id: 'selective_remove_trees',
        schedule: { signal: 'selective_thinning_remove'},
        action: function() {
            if (stand.obj.lib.selective_thinning_counter == 0) {
                // first year. Save # of marked competitors
                const marked = stand.trees.load('markcompetitor=true');
                stand.setFlag('compN', marked);
                lib.dbg(`selectiveThinning: start removal phase. ${marked} trees marked for removal.`);
            }
            stand.obj.lib.selective_thinning_counter = stand.obj.lib.selective_thinning_counter + 1;
            var n = stand.flag('compN') / opts.repeatTimes;


            lib.log("Year: " + Globals.year + ", selective thinning harvest");

            stand.trees.load('markcompetitor=true');
            stand.trees.filterRandomExclude(n);
            const harvested = stand.trees.harvest();
            //stand.trees.removeMarkedTrees(); // ? necessary ??
            lib.dbg(`selectiveThinning: repeat ${stand.obj.lib.selective_thinning_counter}, removed ${harvested} trees.`);
        },
        description: `Selective thinning (every ${opts.repeatTimes} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`


    }

    program['repeater'] = lib.repeater({ schedule: { signal: 'selective_start_repeat'},
                                           signal: 'selective_thinning_remove',
                                           interval: opts.repeatInterval,
                                           count: opts.repeatTimes });

    program["remover"] = remove_trees;

/*
    const remove_selection = {
        type: 'general',
        id: 'remove_trees',
        schedule: {repeat: true, repeatInterval: opts.repeatInterval, repeatTimes: opts.repeatTimes},
        action: function() {
            //Globals.alert("Lets go");
            stand.setFlag('count',  stand.flag('count') + 1); // update counter
            var counter = stand.flag('count');
            if (counter == 1) {
                // first year. Save # of marked competitors
                stand.setFlag('compN', stand.trees.load('markcompetitor=true'));
            } /*
            if (counter > 4) {              // 4
                // stop repeating....
                Globals.alert('stop mgmt. ' + stand.activity.name);
                //stand.activityByName('remover').enabled=false;
                // activity: scheint bei repeating activities nicht richtig
                // activity: name
                //stand.activity.enabled = false;
                stand.activity.active = false;
                stand.wakeup(); // stop sleeping
                // force extra
                //stand.runNext(stand.activityByName('extra'));
            } * /
            var n = stand.flag('compN') / opts.repeatTimes;

            console.log("Year: " + Globals.year + ", selective thinning harvest");

            stand.trees.load('markcompetitor=true');
            stand.trees.filterRandomExclude(n);
            stand.trees.harvest();
            stand.trees.removeMarkedTrees();
        },
        onSetup: function() {
            stand.setFlag('count', 0);
            stand.activity.active = false;
            stand.simulate = false;
        },
        onEnter: function() { stand.sleep(opts.repeatTimes*opts.repeatInterval)}
    }

    program["remover"] = remove_selection; */

    if (opts.constraint !== undefined) program.constraint = opts.constraint;


    return program;
}

