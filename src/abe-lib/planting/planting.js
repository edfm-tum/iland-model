/**
  The planting module include activities for artificial regeneration.



  @module abe-lib
  @submodule lib.planting
  */

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
            return true;
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

/**
Description
-----------

bla bla bla

asdfadsf




@class lib.planting.dynamic
*/


/**
@type {Options}
@param schedule {Schedule} object
@param id {string} (unique) name of the activity
@param speciesSelectivity {object} define the target species (and their relative weights). This
information is used to build the actual planting items. Can be an object with key-value pairs,
or a JS function that returns such an object
@param speciesDefaults {object}
@property defaultOptions
  */

lib.planting.dynamic = function(options) {

    const defaultOptions = {
        schedule: undefined,
        id: 'planting',
        speciesSelectivity: undefined, ///< species to plant
        speciesDefaults: lib.planting.speciesDefaults
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
    return {
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
        description : `Planting; species and properties are determined dynamically`

    }
}

// defaults per species

lib.planting.speciesDefaults = {
    // spruce, .... use wall-to-wall planting
    'piab': { species: 'piab', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 1 }},
    'abal': { species: 'abal', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 0.8 }},
    'fasy': { species: 'fasy', h: 0.3, age: 1, fraction: function(proportion) {return proportion * 0.8 }},
    // for larix, ... use groups
    'lade': { species: 'lade', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'quro': { species: 'quro', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'qupe': { species: 'qupe', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'acps': { species: 'acps', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },
    'pisy': { species: 'pisy', h: 0.3, age: 1, pattern: 'circle10', random: true,
        n: function(proportion) {return proportion*10000/272; /* 272: area circle10 */} },

};
