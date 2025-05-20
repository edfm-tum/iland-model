
/**
 * The thinning module include activities thinning operations.
 *
 * @class thinning
 * @memberof abe-lib
 */


lib.thinning = {};

Globals.include(lib.path.dir + '/thinning/selective.js');


/**
 * Thinning from below
 * @method fromBelow
 * @param {object} options
 *    @param {string} options.id A unique identifier for the thinning activity (default: 'ThinningFromBelow').
 *    @param {object} options.schedule schedule for the repeater (default: {min: 30, opt: 40, max: 50}).
 *    @param {number} options.interval interval between repeated thinnings (default: 5).
 *    @param {number} options.times number of times to repeat the thinning (default: 5).
 *    @param {boolean} options.block block other activities in repeater (default: true).
 *    @param {string} options.mode mode of thinning, either 'simple' or 'dynamic' (default: 'simple').
 *    @param {number|function} options.thinningShare share of trees to be thinned, can be a number or a function returning a number (default: 0.1).
 *    @param {number[]} options.thinningClasses array defining how much of each thinning class should be removed (default: [60, 25, 15, 0, 0]).
 *    @param {number|undefined} options.speciesSelectivity species list, which species should have a lower probability to get harvested (default: undefined).
 *    @param {string} options.ranking ranking string for filtering trees, e.g. 'volume' (default: 'volume').
 *    @param {string} options.sendSignal signal for triggering the thinning (default: 'thinning_execute').
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the thinning program
 * @example
 *     lib.thinning.fromBelow({
 *         schedule: { min: 25, opt: 35, max: 45 },
 *         interval: 10,
 *         thinningShare: 0.3
 *     });
 */
lib.thinning.fromBelow = function(options) {
	// Problem TODO: thinning from below doesn't take as much volume as intended but!
    // 1. Default Options
    const defaultOptions = {
		id: 'ThinningFromBelow',
		schedule: {min: 30, opt: 40, max: 50}, // opt for Broadleaves: (35, 55, 85), Conifers: (25, 40, 55), more or less adapted from 'Waldbau auf soziologisch-ökologischer Grundlage', H. Mayer, p. 235
		interval: 5,
		times: 5,
		block: true,
		mode: 'simple',
        thinningShare: 0.1, // per thinning action
		thinningClasses: [80, 15, 4, 0.9, 0.1], // how much of which class should be removed?
		speciesSelectivity: undefined,
		ranking: 'basalarea', 
		sendSignal: 'thinning_execute',
		constraint: undefined,
		
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	// tree filter for thinning
	var treeFilter = undefined;/*
	if (opts.speciesSelectivity !== undefined) {
		lib.dbg(`Stand flag: ${stand.flag('targetSpecies')}`);
		//opts.speciesSelectivity = opts.speciesSelectivity.call(opts);
		//treeFilter = lib.buildRareSpeciesFilter(opts.speciesSelectivity, 0.2);
	};*/
	
	// dynamic parameters of selective thinning
	function dynamic_thinningShare() {
		// retrieve thinning share from stand flag during runtime
		var value = stand.flag('thinningShare');
		if (value === undefined) value = opts.thinningShare;
		return value;
	};

	// changing parameters if mode is dynamic
	if (opts.mode == 'dynamic') {
		opts.thinningShare = dynamic_thinningShare;
	};

	var program = {};

	program["init"] = {
		id: opts.id + '_init',
		type: 'general',
		schedule: opts.schedule,
		action: function() {
			var treeFilter = undefined;
			if (opts.speciesSelectivity !== undefined) {
				lib.dbg(`Stand flag: ${stand.flag('targetSpecies')}`);
				var speciesObject = opts.speciesSelectivity.call();
				treeFilter = lib.buildRareSpeciesFilter(speciesObject, speciesObject);
			};
			lib.dbg(`Thinning from below initialized.`);
		},
	};

	program["thinning_repeat"] = lib.repeater({ 
		id: opts.id + "_repeater",
		schedule: opts.schedule, 
		signal: opts.sendSignal, 
		count: opts.times, 
		interval: opts.interval,
		block: opts.block
	});
	
	program["thinning"] = {
		//id: opts.id + '_thinning',
		type: 'thinning',
		schedule: { signal: opts.sendSignal},
		constraint: ["stand.age>30 and stand.age<70"],
        thinning: 'custom',
        targetValue: opts.thinningShare * 100, 
		targetVariable: opts.ranking, 
		targetRelative: true,
		filter: treeFilter,
        minDbh: 0,
        classes: opts.thinningClasses,
		onCreate: function(act) { 
							  act.scheduled=false; /* this makes sure that evaluate is also called when invoked by a signal */ 
							  console.log(`onCreate: ${opts.id}: `);
							  printObj(this);
							  console.log('---end---');							  
							  },
		onExecuted: function() {
			lib.dbg(`Thinning from below executed. filter: `+ this.filter);
			//lib.activityLog('thinning_from_below'); 
		},
	}

	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	//program.description = `A simple repeating harvest operation (every ${opts.times} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return program;	
}


/**
 * Tending operation
 * @method tending
 * @param {object} options
 *    @param {string} options.id A unique identifier for the tending activity (default: 'Tending').
 *    @param {object|undefined} options.schedule schedule for the tending (default: undefined).
 *    @param {number} options.times number of tending actions (default: 3).
 *    @param {number} options.interval interval between tending actions (default: 2).
 *    @param {object} options.speciesSelectivity object defining species selectivity, e.g. { 'fasy': 0.9, 'abal': 0.8, 'rest': 0.2 } (default: { 'rest': 0.9 }).
 *    @param {number} options.intensity intensity of tending (default: 10).
 *    @param {boolean} options.block block other activities in repeater (default: true).
 *    @param {string} options.sendSignal signal for triggering the tending (default: 'tending_execute').
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the tending program
 * @example
 *     lib.thinning.tending({
 *         times: 5,
 *         interval: 3,
 *         speciesSelectivity: { 'piab': 0.7, 'lade': 0.6 }
 *     });
 */
lib.thinning.tending = function(options) {
    // 1. Default Options
    const defaultOptions = {
		id: 'Tending',
		schedule: undefined,
		times: 3,
		interval: 2,
        speciesSelectivity: { 'rest': 0.3 },
        intensity: 10,
		block: true,
		sendSignal: 'tending_execute',
		constraint: undefined,		
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
    //if (typeof opts.speciesSelectivity === 'function')
    //    opts.speciesSelectivity = opts.speciesSelectivity.call(opts);
	//Globals.alert("Hello World");

	var program = {};

	program["tending_repeat"] = lib.repeater({ 
		id: opts.id + "_repeater",
		schedule: opts.schedule, 
		signal: opts.sendSignal, 
		count: opts.times, 
		interval: opts.interval,
		block: opts.block
	});
	
	program["tending"] = {
		id: opts.id + '_tending',
		type: 'thinning',
		schedule: { signal: opts.sendSignal},
		thinning: 'tending',
		speciesSelectivity: opts.speciesSelectivity,
		intensity: opts.intensity,
		onCreate: function(act) { 
							  act.scheduled=false; /* this makes sure that evaluate is also called when invoked by a signal */ 
							  console.log(`onCreate: ${opts.id}: `);
							  printObj(this);
							  console.log('---end---');							  
							  },
		onExecuted: function() {
			//Globals.alert("Tending in stand " + stand.id + " executed.");
			//lib.activityLog('thinning_tending'); 
		},
	}

	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	program.description = `A tending activity, that starts at age ${opts.schedule.optRel} and does ${opts.times} tending operations every ${opts.interval} years.`;
						
	return program;	
}


/**
 * Plenter thinning operation
 * @method plenter
 * @param {object} options
 *    @param {string} options.id A unique identifier for the thinning activity (default: 'Plenter').
 *    @param {object|undefined} options.schedule schedule for the thinning (default: undefined).
 *    @param {number} options.times number of thinning actions (default: 1000).
 *    @param {number} options.interval interval between thinning actions (default: 5).
 *    @param {object} options.plenterCurve object defining target diameter distribution i.e. plenter curve.
 *    @param {number} options.dbhSteps width of dbh classes to compare with plenterCurve (default: 5).
 *    @param {number} options.intensity intensity of thinning activity, specifying which share of trees to remove should be removed (default: 1).
 *    @param {boolean} options.block block other activities in repeater (default: true).
 *    @param {string} options.sendSignal signal for triggering the thinning (default: 'plenter_execute').
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the plenter thinning program
 * @example
 *     lib.thinning.plenter({
 *         interval: 5,
 *         plenterCurve: { 10: 350, 20: 150, 30: 80, 40: 40, 50: 20 },
 * 		   dbhSteps: 10
 *     });
 */
lib.thinning.plenter = function(options) {
	// 1. Default Options
    const defaultOptions = {
		id: 'Plenter',
		schedule: undefined,
		times: 1000, // repeat for a long time!
		interval: 5,
		plenterCurve: { 10: 350, 15: 220, 20: 150, 25: 105, 30: 80, 35: 50, 40: 40, 45: 25, 50: 20 }, // target number of tress per dbh class
		dbhSteps: 5, // dbh class width
		intensity: 1,
		block: true,
		sendSignal: 'plenter_execute',
		constraint: undefined,		
        // ... add other default thinning parameters
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	// check if any two dbh classes in PlenterCurve are closer than dbhSteps
	var dbhClasses = Object.keys(opts.plenterCurve);
	for (var i = 0; i < dbhClasses.length - 1; i++) {
		if (dbhClasses[i + 1] - dbhClasses[i] < opts.dbhSteps) {
			throw new Error(`Plenter thinning: dbh classes in 'plenterCurve' are too close together! Change parameter 'dbhSteps' or 'plenterCurve'.`);
		}
	}

	var program = {};

	program["plenter_repeat"] = lib.repeater({ 
		id: opts.id + "_repeater",
		schedule: opts.schedule, 
		signal: opts.sendSignal, 
		count: opts.times, 
		interval: opts.interval,
		block: opts.block
	});

	program["plenter_thinning"] = {
		id: opts.id + '_thinning',
		type: 'scheduled', 
		schedule: { signal: opts.sendSignal },
		onEvaluate: function() {
			return true; 
		},
		onExecute: function() {
			// Compare current and target distributions and mark trees for removal (example)
			for (var i in dbhClasses) {
				var dbh = dbhClasses[i];
				// load all trees of dbh class [dbh-dbhSteps, dbh]
				var treesInClass = stand.trees.load('dbh>' + (dbh - opts.dbhSteps) + ' and dbh<=' + dbh);
				var targetCount = opts.plenterCurve[dbh] * stand.area;
				lib.dbg(`plenter activity - dbh class: ${dbh}: ${treesInClass} trees in Class, ${targetCount} trees target.`);

				var N = stand.trees.filterRandomExclude((treesInClass - targetCount) * opts.intensity);
				stand.trees.harvest();

				stand.trees.removeMarkedTrees();

				lib.dbg(`plenter activity - dbh class: ${dbh}: ${treesInClass - N} trees removed.`);
			}
	
			//Remove the marked trees
			stand.trees.removeMarkedTrees();
		},
		// onExecuted: function() {
		// 	lib.activityLog('plenter_thinning'); 
		// },
		onExit: function() {
            if (opts.sendSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
			};
        }
	};

	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	program.description = `A plenter thinning activity, that removes every ${opts.interval} years a certain number of trees per dbh class according to a target distribution (plenter curve).`;
						
	return program;	
}