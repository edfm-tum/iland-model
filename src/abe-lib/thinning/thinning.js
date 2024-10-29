lib.thinning = {};

Globals.include(lib.path.dir + '/thinning/selective.js');


lib.thinning.fromBelow = function(options) {
    // 1. Default Options
    const defaultOptions = {
		mode: 'simple',
        thinningShare: 0.2,
		ranking: 'volume', 
		repeatInterval: 5,
		repeatTimes: 5,
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
	
	const program = {
		type: 'thinning',
		id: 'from_below', 
        schedule: { repeat: true, repeatInterval: opts.repeatInterval, repeatTimes: opts.repeatTimes},
		constraint: ["stand.age>30 and stand.age<70"],
        thinning: 'custom',
        targetValue: opts.thinningShare * 100, targetVariable: opts.ranking, targetRelative: true,
        minDbh: 0,
        classes: [70, 30, 0, 0, 0],
		onExecute: function() {
        },
        onExecuted: function() {lib.activityLog('thinning_from_below'); }
	};		
	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	//program.description = `A simple repeating harvest operation (every ${opts.repeatTimes} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return program;	
}



lib.thinning.tending = function(options) {
    // 1. Default Options
    const defaultOptions = {
		id: 'Tending',
		schedule: {repeat: true, repeatInterval: 3},
        speciesSelectivity: { 'fasy': 0.9, 'abal': 0.8 },
        intensity: 10,
		constraint: undefined,
		
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
    //if (typeof opts.speciesSelectivity === 'function')
    //    opts.speciesSelectivity = opts.speciesSelectivity.call(opts);

	const tending = {
		id: opts.id,
		type: 'thinning',
        schedule: opts.schedule,
        thinning: 'tending',
        speciesSelectivity: opts.speciesSelectivity, 
        intensity: opts.intensity,
        onSetup: function() { /* stand.trace = true; */  },
        onExecuted: function() {lib.activityLog('thinning_tending'); }
	};		
	
	if (opts.constraint !== undefined) tending.constraint = opts.constraint;
	
	//program.description = `A simple repeating harvest operation (every ${opts.repeatTimes} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return tending;	
}
