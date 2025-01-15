/**
 * Selective thinning operation
 * @method selectiveThinning
 * @param {object} options
 *    @param {object|undefined} options.schedule schedule for the thinning (default: undefined).
 *    @param {string} options.id A unique identifier for the thinning activity (default: 'selective_select_trees').
 *    @param {string} options.mode mode of thinning, either 'simple' or 'dynamic' (default: 'simple').
 *    @param {string} options.SpeciesMode mode of species selection, either 'simple' or 'dynamic' (default: 'simple').
 *    @param {number|function} options.NTrees number of trees to select, can be a number or a function returning a number (default: 80).
 *    @param {number|function} options.NCompetitors number of competitor trees to select, can be a number or a function returning a number (default: 4).
 *    @param {object|function} options.speciesSelectivity object defining species selectivity, can be an object or a function returning an object (default: {}).
 *    @param {string|function} options.ranking ranking string for selecting trees, can be a string or a function returning a string (default: 'height').
 *    @param {number} options.repeatInterval interval between repeated thinnings (default: 5).
 *    @param {number} options.repeatTimes number of times to repeat the thinning (default: 5).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the thinning program
 * @example
 *     lib.thinning.selectiveThinning({
 *         schedule: { start: 30, end: 100 },
 *         mode: 'dynamic',
 *         SpeciesMode: 'dynamic',
 *         NTrees: function() { return stand.flag('NTrees') || 100; },
 *         NCompetitors: function() { return stand.flag('NCompetitors') || 5; }
 *     });
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
        schedule: opts.schedule,	
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
            lib.activityLog('thinning_selection'); 
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
            lib.activityLog('thinning remove competitors'); // details? target species?
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

lib.thinning.selectiveThinningZ1Z2 = function(options) {
    // 1. Default Options
    const defaultOptions = {
        id: 'selectiveThinningZ1Z2',
        schedule: undefined,
        mode: 'simple',
        SpeciesMode: 'simple',
        returnPeriode: 30, // years until the next generation of crop trees should be selected
        signal: 'next_selection_periode', // signal, that the selection_repeater should emit
        NTrees: 80, // number of crop trees to mark in each selection periode
        NCompetitors: 4, // total number of competitors to mark in each selection periode (not per thinning activity!)
        speciesSelectivity: {},
        ranking: 'height',
        repeatTimes: 3, // number of thinning activities for each marked crop tree
        repeatInterval: 5, // years between each thinning activity
        constraint: undefined,
        block: true,
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

    const initial_selection = {
        type: 'thinning',
        thinning: 'selection',
        id: opts.id + '_initial',
        schedule: opts.schedule,	
        constraint: ["stand.age>30"],
        N: opts.NTrees,
        NCompetitors: opts.NCompetitors, 
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.ranking,
    
        onSetup: function() { 
            lib.initStandObj(); // create an empty object as stand property
        },
    
        onEnter: function() {
            stand.obj.lib.selective_thinning_counter = 0;
        },
    
        onExecuted: function() {
            lib.dbg("Initial selection in stand " + stand.id + " executed.");
            stand.stp.signal('start_selection_repeater');            
            stand.stp.signal('selective_start_thinning');
            lib.activityLog('Initial crop tree selection'); 
        },
        description: `Selective thinning. Repeated ${opts.repeatTimes} times every ${opts.repeatInterval} years.`
    };
    program["initial_selection"] = initial_selection;
    
    program["selection_repeater"] = lib.repeater({
        schedule: { signal: 'start_selection_repeater' },
        id: opts.id + "_repeater",
        signal: opts.signal,
        count: 1000, // high number as it should go "forever" ;-)
        interval: opts.returnPeriode,
        block: opts.block,
    }); 
  
    const select_trees = {
        type: 'thinning',
        thinning: 'selection',
        id: opts.id + '_repeating',
        schedule: { signal: opts.signal },	
        constraint: ["stand.age>30"],
        N: opts.NTrees,
        NCompetitors: opts.NCompetitors, 
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.ranking + ' * (height < 25)',
    
        onCreate: function(act) { 
            act.scheduled=false; /* this makes sure that evaluate is also called when invoked by a signal */ 
            console.log(`onCreate: ${opts.id}: `);
            printObj(this);
            console.log('---end---');							  
        },
    
        onEnter: function() {
            lib.dbg("Hello world");
            stand.obj.lib.selective_thinning_counter = 0;
        },
    
        onExecuted: function() {
            lib.dbg("Select trees in stand " + stand.id + " executed.");
			stand.stp.signal('selective_start_thinning');
            lib.activityLog('thinning_selection'); 
        },
        description: `Selective thinning. Repeated ${opts.repeatTimes} times every ${opts.repeatInterval} years.`
    };

    program["selector"] = select_trees;
  
    program['thinning_repeater'] = lib.repeater({ 
        schedule: { signal: 'selective_start_thinning'},
        signal: 'selective_thinning_remove',
        interval: opts.repeatInterval,
        count: opts.repeatTimes,
        block: opts.block,
    });
  
    const remove_trees = {
        type: 'general',
        id: opts.id + '_remove_trees',
        schedule: { signal: 'selective_thinning_remove'},
        action: function() {
            if (stand.obj.lib.selective_thinning_counter == 0) {
                // first year. Save # of marked competitors
                const marked = stand.trees.load('markcompetitor=true');
                stand.setFlag('compN', marked);
                lib.dbg(`selectiveThinning: start removal phase. ${marked} trees marked for removal.`);
            }
            
            lib.log("Year: " + Globals.year + ", selective thinning harvest");
      
            stand.obj.lib.selective_thinning_counter = stand.obj.lib.selective_thinning_counter + 1;
            var n = stand.flag('compN') / opts.repeatTimes;
            var N_Competitors = stand.trees.load('markcompetitor=true');
      
            if ((N_Competitors - n) > 0) {
                stand.trees.filterRandomExclude(N_Competitors - n);
            };
      
            const harvested = stand.trees.harvest();
            lib.activityLog('thinning remove competitors'); // details? target species?
            // stand.trees.removeMarkedTrees(); // ? necessary ??
            lib.dbg(`selectiveThinning: repeat ${stand.obj.lib.selective_thinning_counter}, removed ${harvested} trees.`);
        },
        description: `Selective thinning (every ${opts.repeatTimes} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`
    }
    program["remover"] = remove_trees;
  
    if (opts.constraint !== undefined) program.constraint = opts.constraint;
  
    return program;
}
  