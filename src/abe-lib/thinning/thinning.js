/**
 * The top-level library module.
 * @module abe-lib
 */

/**
 * The thinning module include activities thinning operations.
 *
 * @class thinning
 * @memberof abe-lib
 */


lib.thinning = {};

Globals.include(lib.path.dir + '/thinning/selective.js');


lib.thinning = {};

Globals.include(lib.path.dir + '/thinning/selective.js');

/**
 * Thinning from below
 * @method fromBelow
 * @param {object} options
 *    @param {string} options.id A unique identifier for the thinning activity (default: 'ThinningFromBelow').
 *    @param {object} options.repeaterSchedule schedule for the repeater (default: {min: 30, opt: 40, max: 50}).
 *    @param {string} options.signal signal for triggering the thinning (default: 'thinning_execute').
 *    @param {number} options.repeatInterval interval between repeated thinnings (default: 5).
 *    @param {number} options.repeatTimes number of times to repeat the thinning (default: 5).
 *    @param {boolean} options.block block other activities in repeater (default: true).
 *    @param {string} options.mode mode of thinning, either 'simple' or 'dynamic' (default: 'simple').
 *    @param {number|function} options.thinningShare share of trees to be thinned, can be a number or a function returning a number (default: 0.2).
 *    @param {number[]} options.thinningClasses array defining how much of each thinning class should be removed (default: [60, 25, 15, 0, 0]).
 *    @param {string} options.ranking ranking string for filtering trees, e.g. 'volume' (default: 'volume').
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the thinning program
 * @example
 *     lib.thinning.fromBelow({
 *         repeaterSchedule: { min: 25, opt: 35, max: 45 },
 *         repeatInterval: 10,
 *         thinningShare: 0.3
 *     });
 */
lib.thinning.fromBelow = function(options) {
	// Problem TODO: thinning from below doesn't take as much volume as intended but!
    // 1. Default Options
    const defaultOptions = {
		id: 'ThinningFromBelow',
		repeaterSchedule: {min: 30, opt: 40, max: 50}, // opt for Broadleaves: (35, 55, 85), Conifers: (25, 40, 55), more or less adapted from 'Waldbau auf soziologisch-ökologischer Grundlage', H. Mayer, p. 235
		signal: 'thinning_execute',
		repeatInterval: 5,
		repeatTimes: 5,
		block: true,
		mode: 'simple',
        thinningShare: 0.2, // per thinning action
		thinningClasses: [60, 25, 15, 0, 0], // how much of which class should be removed?
		ranking: 'volume', 
		constraint: undefined,
		
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	
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

	var thinning_repeat = lib.repeater({ schedule: opts.repeaterSchedule, 
		id: opts.id + "_repeater",
		signal: opts.signal, 
		count: opts.repeatTimes, 
		interval: opts.repeatInterval,
		block: opts.block});
	program["thinning_repeat"] = thinning_repeat;
	
	var thinning = {
		//id: opts.id,
		type: 'thinning',
		schedule: { signal: opts.signal},
		constraint: ["stand.age>30 and stand.age<70"],
        thinning: 'custom',
        targetValue: opts.thinningShare * 100, 
		targetVariable: opts.ranking, 
		targetRelative: true,
        minDbh: 0,
        classes: opts.thinningClasses,
		onCreate: function(act) { 
							  act.scheduled=false; /* this makes sure that evaluate is also called when invoked by a signal */ 
							  console.log(`onCreate: ${opts.id}: `);
							  printObj(this);
							  console.log('---end---');							  
							  },
		onExecuted: function() {
			//Globals.alert("Thinning from below in stand " + stand.id + " executed.");
			lib.activityLog('thinning_from_below'); 
		},
	}
	program["thinning"] = thinning;

	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	//program.description = `A simple repeating harvest operation (every ${opts.repeatTimes} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return program;	
}


/**
 * Tending operation
 * @method tending
 * @param {object} options
 *    @param {string} options.id A unique identifier for the tending activity (default: 'Tending').
 *    @param {string} options.signal signal for triggering the tending (default: 'tending_execute').
 *    @param {object|undefined} options.schedule schedule for the tending (default: undefined).
 *    @param {number} options.nTendingActions number of tending actions (default: 3).
 *    @param {number} options.tendingInterval interval between tending actions (default: 2).
 *    @param {object} options.speciesSelectivity object defining species selectivity, e.g. { 'fasy': 0.9, 'abal': 0.8 } (default: { 'fasy': 0.9, 'abal': 0.8 }).
 *    @param {number} options.intensity intensity of tending (default: 10).
 *    @param {boolean} options.block block other activities in repeater (default: true).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the tending program
 * @example
 *     lib.thinning.tending({
 *         nTendingActions: 5,
 *         tendingInterval: 3,
 *         speciesSelectivity: { 'piab': 0.7, 'lade': 0.6 }
 *     });
 */
lib.thinning.tending = function(options) {
    // 1. Default Options
    const defaultOptions = {
		id: 'Tending',
		signal: 'tending_execute',
		schedule: undefined,
		nTendingActions: 3,
		tendingInterval: 2,
        speciesSelectivity: { 'rest': 0.9 },
        intensity: 10,
		block: true,
		constraint: undefined,		
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
    //if (typeof opts.speciesSelectivity === 'function')
    //    opts.speciesSelectivity = opts.speciesSelectivity.call(opts);
	//Globals.alert("Hello World");

	var program = {};

	var tending_repeat = lib.repeater({ schedule: opts.schedule, 
		id: opts.id + "_tending_repeater",
		signal: opts.signal, 
		count: opts.nTendingActions, 
		interval: opts.tendingInterval,
		block: opts.block});
	program["tending_repeat"] = tending_repeat;
	
	var tending = {
		id: opts.id,
		type: 'thinning',
		schedule: { signal: opts.signal},
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
			lib.activityLog('thinning_tending'); 
		},
	}
	program["tending"] = tending;

	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	program.description = `A tending activity, that starts at age ${opts.schedule.optRel} and does ${opts.nTendingActions} tending operations every ${opts.tendingInterval} years.`;
						
	return program;	
}
