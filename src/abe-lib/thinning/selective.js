/**
 * Selective thinning operation
 * @method selectiveThinning
 * @param {object} options
 *    @param {string} options.id A unique identifier for the thinning activity (default: 'selective_select_trees').
 *    @param {object|undefined} options.schedule schedule for the thinning (default: undefined).
 *    @param {string} options.mode mode of thinning, either 'simple' or 'dynamic' (default: 'simple').
 *    @param {string} options.SpeciesMode mode of species selection, either 'simple' or 'dynamic' (default: 'simple').
 *    @param {number|function} options.nTrees number of trees to select, can be a number or a function returning a number (default: 80).
 *    @param {number|function} options.nCompetitors number of competitor trees to select, can be a number or a function returning a number (default: 4).
 *    @param {object|function} options.speciesSelectivity object defining species selectivity, can be an object or a function returning an object (default: {}).
 *    @param {string|function} options.preferenceFunction ranking string for selecting trees, can be a string or a function returning a string (default: 'height').
 *    @param {number} options.interval interval between repeated thinnings (default: 5).
 *    @param {number} options.times number of times to repeat the thinning (default: 5).
 *    @param {string} options.sendSignal signal send out in each thinning activity (default: 'selective_thinning_remove').
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the thinning program
 * @example
 *     lib.thinning.selectiveThinning({
 *         schedule: { start: 30, end: 100 },
 *         mode: 'dynamic',
 *         SpeciesMode: 'dynamic',
 *         nTrees: function() { return stand.flag('nTrees') || 100; },
 *         nCompetitors: function() { return stand.flag('nCompetitors') || 5; }
 *     });
 */
lib.thinning.selectiveThinning = function(options) {
    // 1. Default Options
    const defaultOptions = {
        id: 'SelectiveThinning',
        schedule: undefined,
        mode: 'simple',
        nTrees: 80,
        nCompetitors: 4,
        speciesSelectivity: {},
        preferenceFunction: 'height',
        interval: 5,
        times: 5,
        sendSignal: 'selective_thinning_remove',
        constraint: undefined,

        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});


    // dynamic parameters of selective thinning
    function dynamic_nTrees() {
        // retrieve N from stand flag during runtime
        var value = stand.flag('nTrees');
        if (value === undefined) value = opts.nTrees;
        return value;
    };
    function dynamic_nCompetitors() {
        // retrieve N from stand flag during runtime
        //var value = stand.flag('nCompetitors');
        const Agefactor = Math.max(Math.min(1.0, -0.01*stand.age+1.2), 0.0);
        var value = Math.max(stand.flag('nCompetitors')*Agefactor, 1);
        if (value === undefined) value = opts.nCompetitors;
        return value;
    };

    // changing parameters if mode is dynamic
    if (opts.mode == 'dynamic') {
        opts.nTrees = dynamic_nTrees;
        opts.nCompetitors = dynamic_nCompetitors;
        opts.preferenceFunction = dynamic_preferenceFunction;
    };

    const program = {};

    program["Selective_selector"] = {
        id: opts.id + '_selector',
        type: 'thinning',
        schedule: opts.schedule,	
        thinning: 'selection',
        N: opts.nTrees,
        nCompetitors: opts.nCompetitors, // total competitors! not per thinning event
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.preferenceFunction,

        onSetup: function() { 
            lib.initStandObj(); // create an empty object as stand property
        },
    
        onEnter: function() {
            stand.obj.lib.selective_thinning_counter = 0;
        },

        onExit: function() {
            //lib.activityLog(opts.id + ' - thinning_selection done'); 
        },
        description: `Part of selective thinning - mark ${opts.nTrees} crop trees and ${opts.nCompetitors} competitors.`
    };

    program['Selective_repeater'] = lib.repeater({ 
        id: opts.id + '_repeater',
        schedule: opts.schedule,
        signal: opts.sendSignal,
        interval: opts.interval,
        count: opts.times 
    });

    program["Selective_remover"] = {
        id: opts.id + '_remover',
        type: 'general',
        schedule: { signal: opts.sendSignal },
        action: function() {
            if (stand.obj.lib.selective_thinning_counter == 0) {
                // first year. Save # of marked competitors
                const marked = stand.trees.load('markcompetitor=true');
                stand.setFlag('compN', marked);
                lib.dbg(`selectiveThinning: start removal phase. ${marked} trees marked for removal.`);
            }
            stand.obj.lib.selective_thinning_counter = stand.obj.lib.selective_thinning_counter + 1;
            var n = stand.flag('compN') / opts.times;

            stand.trees.load('markcompetitor=true');
            stand.trees.filterRandomExclude(n);
            const harvested = stand.trees.harvest();
            //lib.activityLog('thinning remove competitors'); 
            lib.dbg(`selectiveThinning: repeat ${stand.obj.lib.selective_thinning_counter}, removed ${harvested} trees.`);
        },
        description: `Part of selective thinning - remove selected competitors in ${opts.times} activies every ${opts.interval} years.`
    }

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
        sendSignalPeriode: 'next_selection_periode', // signal, every time a new selection periode starts
        nTrees: 80, // number of crop trees to mark in each selection periode
        nCompetitors: 4, // total number of competitors to mark in each selection periode (not per thinning activity!)
        speciesSelectivity: {},
        preferenceFunction: 'height',
        times: 3, // number of thinning activities for each marked crop tree
        interval: 5, // years between each thinning activity
        sendSignalThinning: 'selective_start_thinning', // signal, at every thinning activity
        constraint: ["stand.age>30"],
        block: true,
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
    
    
    // dynamic parameters of selective thinning
    function dynamic_nTrees() {
      // retrieve N from stand flag during runtime
      var value = stand.flag('nTrees');
      if (value === undefined) value = opts.nTrees;
      return value;
    };
    function dynamic_nCompetitors() {
      // retrieve N from stand flag during runtime
      //var value = stand.flag('nCompetitors');
      const Agefactor = Math.max(Math.min(1.0, -0.01*stand.age+1.2), 0.0);
      var value = Math.max(stand.flag('nCompetitors')*Agefactor, 1);
      if (value === undefined) value = opts.nCompetitors;
      return value;
    };
    function dynamic_preferenceFunction() {
      // retrieve ranking from stand flag during runtime
      var value = stand.flag('preferenceFunction');
      if (value === undefined) value = opts.preferenceFunction;
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
      opts.nTrees = dynamic_nTrees;
      opts.nCompetitors = dynamic_nCompetitors;
      opts.preferenceFunction = dynamic_preferenceFunction;
    };
    // changing parameters if SpeciesMode is dynamic
    if (opts.SpeciesMode == 'dynamic') {
      opts.speciesSelectivity = dynamic_speciesSelectivity;
      //opts.preferenceFunction = dynamic_preferenceFunction;
    };
    
    const program = {};

    const initial_selection = {
        type: 'thinning',
        thinning: 'selection',
        id: opts.id + '_initial',
        schedule: opts.schedule,	
        constraint: opts.constraint,
        N: opts.nTrees,
        nCompetitors: opts.nCompetitors, 
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.preferenceFunction,
    
        onSetup: function() { 
            lib.initStandObj(); // create an empty object as stand property
        },
    
        onEnter: function() {
            stand.obj.lib.selective_thinning_counter = 0;
        },
    
        onExecuted: function() {
            lib.dbg("Initial selection in stand " + stand.id + " executed.");
            stand.stp.signal(opts.sendSignalPeriode);            
            stand.stp.signal(opts.sendSignalThinning);
            //lib.activityLog('Initial crop tree selection'); 
        },
        description: `${opts.id} - initial selection of crop trees and competitors.`
    };
    program["SelectiveZ1Z2_initial_selection"] = initial_selection;
    
    program["SelectiveZ1Z2_repeater"] = lib.repeater({
        id: opts.id + "_selection_repeater",
        schedule: { signal: opts.sendSignalPeriode },
        signal: opts.sendSignalPeriode,
        count: 1000, // high number as it should go "forever" ;-)
        interval: opts.returnPeriode,
        block: opts.block,
    }); 
  
    const select_trees = {
        type: 'thinning',
        thinning: 'selection',
        id: opts.id + '_repeating',
        schedule: { signal: opts.sendSignalPeriode },	
        constraint: opts.constraint,
        N: opts.nTrees,
        nCompetitors: opts.nCompetitors, 
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.preferenceFunction + ' * (height < 25)',
    
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
            //lib.activityLog('thinning_selection'); 
        },
        description: `Selective thinning. Repeated ${opts.times} times every ${opts.interval} years.`
    };

    program["SelectiveZ1Z2_selector"] = select_trees;
  
    program['SelectiveZ1Z2_thinning_repeater'] = lib.repeater({ 
        id: opts.id + '_thinning_repeater',
        schedule: { signal: 'selective_start_thinning'},
        signal: 'selective_thinning_remove',
        interval: opts.interval,
        count: opts.times,
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
            var n = stand.flag('compN') / opts.times;
            var N_Competitors = stand.trees.load('markcompetitor=true');
      
            if ((N_Competitors - n) > 0) {
                stand.trees.filterRandomExclude(N_Competitors - n);
            };
      
            const harvested = stand.trees.harvest();
            //lib.activityLog('thinning remove competitors'); // details? target species?
            // stand.trees.removeMarkedTrees(); // ? necessary ??
            lib.dbg(`selectiveThinning: repeat ${stand.obj.lib.selective_thinning_counter}, removed ${harvested} trees.`);
        },
        description: `Selective thinning (every ${opts.times} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`
    }
    program["SelectiveZ1Z2_remover"] = remove_trees;
  
    if (opts.constraint !== undefined) program.constraint = opts.constraint;
  
    return program;
}
  


lib.thinning.selectiveThinningBackup = function(options) {
    // 1. Default Options
    const defaultOptions = {
        schedule: undefined,
        id: 'SelectiveThinning',
        mode: 'simple',
        SpeciesMode: 'simple',
        nTrees: 80,
        nCompetitors: 4,
        speciesSelectivity: {},
        preferenceFunction: 'height',
        interval: 5,
        times: 5,
        sendSignal: undefined,
        constraint: undefined,

        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});


    // dynamic parameters of selective thinning
    function dynamic_nTrees() {
        // retrieve N from stand flag during runtime
        var value = stand.flag('nTrees');
        if (value === undefined) value = opts.nTrees;
        return value;
    };
    function dynamic_nCompetitors() {
        // retrieve N from stand flag during runtime
        //var value = stand.flag('nCompetitors');
        const Agefactor = Math.max(Math.min(1.0, -0.01*stand.age+1.2), 0.0);
        var value = Math.max(stand.flag('nCompetitors')*Agefactor, 1);
        if (value === undefined) value = opts.nCompetitors;
        return value;
    };
    function dynamic_preferenceFunction() {
        // retrieve ranking from stand flag during runtime
        var value = stand.flag('preferenceFunction');
        if (value === undefined) value = opts.preferenceFunction;
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
        opts.nTrees = dynamic_nTrees;
        opts.nCompetitors = dynamic_nCompetitors;
        opts.preferenceFunction = dynamic_preferenceFunction;
    };

    // changing parameters if SpeciesMode is dynamic
    if (opts.SpeciesMode == 'dynamic') {
        opts.speciesSelectivity = dynamic_speciesSelectivity;
        //opts.preferenceFunction = dynamic_preferenceFunction;
    };

    const program = {};

    program["Selective_selector"] = {
        id: opts.id + '_selector',
        type: 'thinning',
        schedule: opts.schedule,	
        thinning: 'selection',
        N: opts.nTrees,
        nCompetitors: opts.nCompetitors, // total competitors! not per thinning event
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.preferenceFunction,

        onSetup: function() { 
            lib.initStandObj(); // create an empty object as stand property
        },
    
        onEnter: function() {
            stand.obj.lib.selective_thinning_counter = 0;
        },

        onExit: function() {
            stand.stp.signal('selective_start_repeat');
            //lib.activityLog('thinning_selection'); 
        },
        description: `Selective thinning. Repeated ${opts.times} times every ${opts.interval} years.`
    };

    program['Selective_repeater'] = lib.repeater({ 
        id: opts.id + '_repeater',
        schedule: { signal: 'selective_start_repeat'},
        signal: 'selective_thinning_remove',
        interval: opts.interval,
        count: opts.times 
    });

    program["Selective_remover"] = {
        id: opts.id + '_remover',
        type: 'general',
        schedule: { signal: 'selective_thinning_remove'},
        action: function() {
            if (stand.obj.lib.selective_thinning_counter == 0) {
                // first year. Save # of marked competitors
                const marked = stand.trees.load('markcompetitor=true');
                stand.setFlag('compN', marked);
                lib.dbg(`selectiveThinning: start removal phase. ${marked} trees marked for removal.`);
            }
            stand.obj.lib.selective_thinning_counter = stand.obj.lib.selective_thinning_counter + 1;
            var n = stand.flag('compN') / opts.times;


            lib.log("Year: " + Globals.year + ", selective thinning harvest");

            stand.trees.load('markcompetitor=true');
            stand.trees.filterRandomExclude(n);
            const harvested = stand.trees.harvest();
            //lib.activityLog('thinning remove competitors'); // details? target species?
            //stand.trees.removeMarkedTrees(); // ? necessary ??
            lib.dbg(`selectiveThinning: repeat ${stand.obj.lib.selective_thinning_counter}, removed ${harvested} trees.`);
        },
        description: `Selective thinning (every ${opts.times} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`
    }

/*
    const remove_selection = {
        type: 'general',
        id: 'remove_trees',
        schedule: {repeat: true, repeatInterval: opts.interval, repeatTimes: opts.times},
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
            var n = stand.flag('compN') / opts.times;

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
        onEnter: function() { stand.sleep(opts.times*opts.interval)}
    }

    program["remover"] = remove_selection; */

    if (opts.constraint !== undefined) program.constraint = opts.constraint;


    return program;
}

lib.thinning.selectiveThinningZ1Z2Backup = function(options) {
    // 1. Default Options
    const defaultOptions = {
        id: 'selectiveThinningZ1Z2',
        schedule: undefined,
        mode: 'simple',
        SpeciesMode: 'simple',
        returnPeriode: 30, // years until the next generation of crop trees should be selected
        signal: 'next_selection_periode', // signal, that the selection_repeater should emit
        nTrees: 80, // number of crop trees to mark in each selection periode
        nCompetitors: 4, // total number of competitors to mark in each selection periode (not per thinning activity!)
        speciesSelectivity: {},
        preferenceFunction: 'height',
        times: 3, // number of thinning activities for each marked crop tree
        interval: 5, // years between each thinning activity
        constraint: undefined,
        block: true,
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
    
    
    // dynamic parameters of selective thinning
    function dynamic_nTrees() {
      // retrieve N from stand flag during runtime
      var value = stand.flag('nTrees');
      if (value === undefined) value = opts.nTrees;
      return value;
    };
    function dynamic_nCompetitors() {
      // retrieve N from stand flag during runtime
      //var value = stand.flag('nCompetitors');
      const Agefactor = Math.max(Math.min(1.0, -0.01*stand.age+1.2), 0.0);
      var value = Math.max(stand.flag('nCompetitors')*Agefactor, 1);
      if (value === undefined) value = opts.nCompetitors;
      return value;
    };
    function dynamic_preferenceFunction() {
      // retrieve ranking from stand flag during runtime
      var value = stand.flag('preferenceFunction');
      if (value === undefined) value = opts.preferenceFunction;
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
      opts.nTrees = dynamic_nTrees;
      opts.nCompetitors = dynamic_nCompetitors;
      opts.preferenceFunction = dynamic_preferenceFunction;
    };
    // changing parameters if SpeciesMode is dynamic
    if (opts.SpeciesMode == 'dynamic') {
      opts.speciesSelectivity = dynamic_speciesSelectivity;
      //opts.preferenceFunction = dynamic_preferenceFunction;
    };
    
    const program = {};

    const initial_selection = {
        type: 'thinning',
        thinning: 'selection',
        id: opts.id + '_initial',
        schedule: opts.schedule,	
        constraint: ["stand.age>30"],
        N: opts.nTrees,
        nCompetitors: opts.nCompetitors, 
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.preferenceFunction,
    
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
            //lib.activityLog('Initial crop tree selection'); 
        },
        description: `Selective thinning. Repeated ${opts.times} times every ${opts.interval} years.`
    };
    program["SelectiveZ1Z2_initial_selection"] = initial_selection;
    
    program["SelectiveZ1Z2_repeater"] = lib.repeater({
        id: opts.id + "_selection_repeater",
        schedule: { signal: 'start_selection_repeater' },
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
        N: opts.nTrees,
        nCompetitors: opts.nCompetitors, 
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.preferenceFunction + ' * (height < 25)',
    
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
            //lib.activityLog('thinning_selection'); 
        },
        description: `Selective thinning. Repeated ${opts.times} times every ${opts.interval} years.`
    };

    program["SelectiveZ1Z2_selector"] = select_trees;
  
    program['SelectiveZ1Z2_thinning_repeater'] = lib.repeater({ 
        id: opts.id + '_thinning_repeater',
        schedule: { signal: 'selective_start_thinning'},
        signal: 'selective_thinning_remove',
        interval: opts.interval,
        count: opts.times,
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
            var n = stand.flag('compN') / opts.times;
            var N_Competitors = stand.trees.load('markcompetitor=true');
      
            if ((N_Competitors - n) > 0) {
                stand.trees.filterRandomExclude(N_Competitors - n);
            };
      
            const harvested = stand.trees.harvest();
            //lib.activityLog('thinning remove competitors'); // details? target species?
            // stand.trees.removeMarkedTrees(); // ? necessary ??
            lib.dbg(`selectiveThinning: repeat ${stand.obj.lib.selective_thinning_counter}, removed ${harvested} trees.`);
        },
        description: `Selective thinning (every ${opts.times} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`
    }
    program["SelectiveZ1Z2_remover"] = remove_trees;
  
    if (opts.constraint !== undefined) program.constraint = opts.constraint;
  
    return program;
}
  