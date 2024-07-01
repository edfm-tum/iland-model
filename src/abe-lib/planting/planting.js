lib.planting = {};


lib.planting.general = function(options) {
	const defaultOptions = {
		schedule: 1, 
		species: 'abal',
		spacing: 10,
		fraction: 1,
		treeHeight: 2,
		pattern: undefined, //"rect2"
		clear: false
	};
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	const plant = {
		type: 'planting', 
		schedule: opts.schedule,
		random: false,
		items: [ { 
			species: opts.species, 
			fraction: opts.fraction, 
			height: opts.treeHeight,
			pattern: opts.pattern,
			spacing: opts.spacing,
			clear: opts.clear}],
		onEvaluate: function() { // Space for quitting planting if condition is met
			Globals.alert("Planting whoooohoooo");
			return true
		}
	}	
	return plant;
}


/*lib.planting.stripwise = function(options) {
	const defaultOptions = {
		schedule: 1, 
		species: 'abal',
		spacing: 10,
		fraction: 1,
		treeHeight: 2,
		pattern: undefined, //"rect2"
		clear: false
	};
	const opts = mergeOptions(defaultOptions, options || {});
	
	const plant = {
		type: 'planting', 
		schedule: opts.schedule,
		random: false,
		items: [ { 
			species: opts.species, 
			fraction: opts.fraction, 
			height: opts.treeHeight,
			pattern: opts.pattern,
			spacing: opts.spacing,
			clear: opts.clear}],
		onEvaluate: function() { // Space for quitting planting if condition is met
			return true
		}
	}	
	return plant;
	
}; */

/*
lib.planting = function(options) {
    // 1. Input Validation 
    //    - Validate options.species 
    //    - Validate options.spacing 
	const defaultOptions = {  
        spacing: 10,
		species: 'pine'
        
        // ... add other default thinning parameters
    };
	const opts = mergeOptions(defaultOptions, options || {});
    
	// 2. Planting Area Calculation (Hypothetical)
    const totalArea = 100; // calculateAvailableArea(options); // Assume you have such a function

    // 3. Individual Activity Generation
    const plantingActivities = [];
    const seedlingsPerActivity = opts.spacing * 2;

    for (let areaOffset = 0; areaOffset < totalArea; areaOffset += seedlingsPerActivity) {
        plantingActivities.push({
            type: 'planting', // Assuming all activities are of type 'planting'
            species: opts.species,
			spacing: opts.spacing
        });
    }

    return plantingActivities;
}*/
