/**
  The planting module include activities thinning operations.



  @module abe-lib
  @submodule thinning
  */

lib.thinning = {};

Globals.include(lib.path.dir + '/thinning/selective.js');


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



lib.thinning.tending = function(options) {
    // 1. Default Options
    const defaultOptions = {
		id: 'Tending',
		signal: 'tending_execute',
		//startAge: 40,
		schedule: undefined,
		nTendingActions: 3,
		tendingInterval: 2,
        speciesSelectivity: { 'fasy': 0.9, 'abal': 0.8 },
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

	// var stp = {
	// 	id: opts.id,
	// 	type: 'thinning',
	// 	thinning: 'tending',
	// 	tending_repeat: lib.repeater({ schedule: opts.startAge, signal: opts.signal, count: opts.nTendingActions, interval: opts.tendingInterval}),
	// 	//test: { type: 'general', schedule: { min: 45, max: 90  }, action: function() { fmengine.log('nextNEXT tending'); }},
	// 	tending: {
	// 		type: 'thinning',
	// 		schedule: { signal: opts.signal},
	// 		thinning: 'tending',
	// 		speciesSelectivity: opts.speciesSelectivity,
	// 		intensity: opts.intensity,
	// 		onCreate: function(act) { 
	// 							  act.scheduled=false; /* this makes sure that evaluate is also called when invoked by a signal */ 
	// 							  console.log('onCreate: this-object: ');
	// 							  printObj(this);
	// 							  console.log('---end---');
								  
	// 							  },
	// 		onExecuted: function() {lib.activityLog('thinning_tending'); },
	// 	}
		
	// }

	 
	// const tending = {
	// 	id: opts.id,
	// 	type: 'thinning',
    //     schedule: opts.schedule,
    //     thinning: 'tending',
    //     speciesSelectivity: opts.speciesSelectivity, 
    //     intensity: opts.intensity,
    //     onSetup: function() { /*stand.trace = true; */ },
    //     onExecuted: function() {lib.activityLog('thinning_tending'); }
	// };	 
		
	
	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	program.description = `A tending activity, that starts at age ${opts.schedule.optRel} and does ${opts.nTendingActions} tending operations every ${opts.tendingInterval} years.`;
						
	return program;	
}
