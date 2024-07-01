lib.thinning = {};

lib.thinning.selectiveThinning = function(options) {
    // 1. Default Options
    const defaultOptions = {
		mode: 'simple',
        NTrees: 80, 
		NCompetitors: 4,
        speciesSelectivity: {},
		ranking: 'height', 
		repeatInterval: 5,
		repeatTimes: 5,
		constraint: undefined,
		
        // ... add other default thinning parameters
    };
	const opts = mergeOptions(defaultOptions, options || {});
	
	
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
	
	if (opts.mode == 'dynamic') {
		opts.NTrees = dynamic_NTrees;
		opts.NCompetitors = dynamic_NCompetitors;
		opts.ranking = dynamic_ranking;
	};
	
	const program = {};

	const select_trees = {
		type: 'thinning', 
		id: 'select_trees', 
		schedule: {absolute: true, opt: 3},			//absolute: true, opt: 3},	//repeat: true, repeatInterval: 10	
		thinning: 'selection', 
		N: opts.NTrees, 
		NCompetitors: opts.NCompetitors, // total competitors! not per thinning event
		speciesSelectivity: opts.speciesSelectivity,
		ranking: opts.ranking, 		
		
		onExit: function() {		// JM: what does this do?
			//Globals.alert(stand.flag('NTrees') + ', ' + stand.flag('NCompetitors') + ', ' + stand.flag('ranking'));
			let a = stand.activityByName('remove_trees');
			if (a !== undefined) a.active = true;	
		},
	};		
	
	program["selector"] = select_trees;
	
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
			} */
			var n = stand.flag('compN') / opts.repeatTimes; 
			
			console.log("Year: " + Globals.year + ", selective thinning harvest");
			
			stand.trees.load('markcompetitor=true');
			stand.trees.filterRandomExclude(n);
			//stand.trees.randomize();
			//stand.trees.filter('incsum(1) <= ' + n);
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
	
	program["remover"] = remove_selection;
	
	if (opts.constraint !== undefined) program.constraint = opts.constraint;// JM: What does this do?
	
	//program.description = `A simple repeating harvest operation (every ${opts.repeatTimes} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return program;	
}
