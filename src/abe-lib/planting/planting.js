/**
 * The top-level library module.
 * @module abe-lib
 */

/**
 * The planting library includes management operations related to artificial and natural regeneration.
 *
 * Commonalities....
 * @class planting
 * @memberof abe-lib
 */

lib.planting = {};



/**
 * Schedules a general planting activity.
 *
 * @method general
 * @param {Object} options Options for configuring the general planting.
 *     @param {String} options.id A unique identifier for the activity (default: 'planting').
 *     @param {Number} options.schedule The planting schedule (default: 1).
 *     @param {String} options.species The species to plant. **Mandatory**.
 *     @param {Number} options.spacing The spacing between plants (default: 10).
 *     @param {Number} options.fraction The fraction of the area to be planted (default: 1).
 *     @param {Number} options.treeHeight Initial tree height (default: 0.3).
 *     @param {String|undefined} options.pattern Planting pattern (e.g., "rect2"). (default: undefined).
 *     @param {Boolean} options.clear Whether to clear existing vegetation before planting (default: false).
 *     @param {string|undefined} options.sendSignal signal send after planting activity (default: undefined).
 *     @param {string|undefined} options.constraint constraint (default: undefined).
 * @example
 *     // NOTE: lib.planting is an object, not a class. No need to use 'new'
 *     lib.planting.general({
 *         schedule: 2,
 *         species: 'Pine',
 *         spacing: 12,
 *         pattern: 'rect'
 *     });
 */
 lib.planting.general = function(options) {
	const defaultOptions = {
		id: 'planting',
		schedule: 1, 
        species: undefined,
		spacing: 10,
		fraction: 1,
        treeHeight: 0.3,
		pattern: undefined, //"rect2"
		clear: false,
        sendSignal: undefined,
        constraint: undefined
	};
    const opts = lib.mergeOptions(defaultOptions, options || {});
    if (opts.species === undefined)
        throw Error("Planting: species is mandatory");
	
	const plant = {
		type: 'planting', 
        id: opts.id,
		schedule: opts.schedule,
		random: false,
		items: [ { 
			species: opts.species, 
			fraction: opts.fraction, 
			height: opts.treeHeight,
			pattern: opts.pattern,
			spacing: opts.spacing,
			clear: opts.clear
        }],
		onEvaluate: function() { // Space for quitting planting if condition is met
			Globals.alert("Planting whoooohoooo");
            return true;
		},
        onExit: function() {
            if (opts.sendSignal !== undefined) {
                lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
            }
        }
	}	

    if (opts.constraint !== undefined) plant.constraint = opts.constraint;
	
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





/**
 * Dynamically schedules planting activities based on provided options.
 *
 * @method dynamic
 * @param {Object} options Options for configuring the dynamic planting.
 *     @param {String} options.id A unique identifier for the planting activity (default: 'planting').
 *     @param {Schedule} options.schedule The planting schedule.
 *     @param {Object|Function} options.speciesSelectivity Defines the target species and their relative weights.
 *         This can be an object with species as keys and weights as values, or a function that returns such an object.
 *     @param {Object} options.speciesDefaults Default settings for species (default: lib.planting.speciesDefaults).
 *     @param {string|undefined} options.sendSignal signal send out after final femel harvest activity (default: undefined).
 *     @param {string|undefined} options.constraint constraint (default: undefined).
 */
lib.planting.dynamic = function(options) {

    const defaultOptions = {
        id: 'planting',
        schedule: undefined,
        speciesSelectivity: undefined, ///< species to plant        SHOULD WE NAME IT SPECIES LIKE ABOVE?
        speciesDefaults: lib.planting.speciesDefaults,
        sendSignal: undefined,
        constraint: undefined
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});

    // dynamic generation of planting items
    function buildDynamicItems() {
        // read species selectivty from stand meta data....
        let species_list = {};
        if (opts.speciesSelectivity !== undefined) {
            if (typeof opts.speciesSelectivity === 'function')
                species_list = opts.speciesSelectivity.call(opts);
            else
                species_list = opts.speciesSelectivity;
        } else if (typeof stand.obj !== undefined && stand.obj.hasOwnProperty('speciesSelectivity')) {
            species_list = stand.obj.speciesSelectivity;
        } else {
            throw new Error('dynamic planting: no speciesSelectivty provided. Either set as option or in stand.obj!')
        }

        let items=[];

        // loop over each species and fetch the default planting item per species
        // update the dynamic element (e.g. the number of planting patches)
        for (species in species_list) {
            let species_default = opts.speciesDefaults[species];
            if (species_default === undefined)
                throw new Error(`Planting-defaults: no data for species ${species}.`);

            const evaluatedItem = {};
            for (let key in species_default) {
                if (typeof species_default[key] === 'function') {
                    // Call the method defined in the default object
                    evaluatedItem[key] = species_default[key].call(species_default, species_list[species]);
                    // console.log(`function called with ${species_list[species]}: result: ${evaluatedItem[key]} `);
                } else {
                    evaluatedItem[key] = species_default[key];
                }
            }

            items.push( evaluatedItem  );
        }
        return items;
    }

    // build the ABE activity
    const plant = {
        type: 'planting',
        id: opts.id,
        schedule: opts.schedule,
        random: false,
        items: [],
        onExecute: function() {
            // build planting items dynamically and run them
            let items = buildDynamicItems();
            items.forEach(item => {
                fmengine.runPlanting(stand.id, item);
            });
            lib.activityLog('planting');
        },
        buildItems: function() {
            // this is a test function to view resulting planting items
            let items = buildDynamicItems();
            return items;
        },
        onExit: function() {
            if (opts.sendSignal !== undefined) {
                lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
            }
        },
        description : `Planting; species and properties are determined dynamically`

    }    	

    if (opts.constraint !== undefined) plant.constraint = opts.constraint;
	
	return plant;
}




// defaults per species
/**
 * Default planting settings for various species.
 *
 * @property speciesDefaults
 *
 * @type {Object}
 * @example
 *     // Accessing default settings for 'piab' (spruce)
 *     const spruceDefaults = lib.planting.speciesDefaults.piab;
 *     console.log(spruceDefaults);
 *     // Output: { species: 'piab', h: 0.3, age: 1, fraction: [Function] }
 */
lib.planting.speciesDefaults = {
    // spruce, .... use wall-to-wall planting
    'piab': { species: 'piab', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 1 }},
    'psme': { species: 'psme', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 1 }},
    'abal': { species: 'abal', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 0.8 }},
    'fasy': { species: 'fasy', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 0.8 }},
    // for larix, ... use groups
    'lade': { species: 'lade', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'quro': { species: 'quro', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'qupe': { species: 'qupe', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'cabe': { species: 'cabe', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 0.8 }},
    'acps': { species: 'acps', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'pisy': { species: 'pisy', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },

};



