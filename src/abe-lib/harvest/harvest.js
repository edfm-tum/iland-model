
/**
 * The harvest library includes management operations related to harvesting.
 *
 * @class harvest
 * @memberof abe-lib
 */
lib.harvest = {};


Globals.include(lib.path.dir + '/harvest/femel.js');
Globals.include(lib.path.dir + '/harvest/salvage.js');
/*
lib.harvest = function(options) {
	// Default Options
	const defaultOptions = {  
        dbhThreshold: 0,
		absolute: true,
		opt: 10,
		constraint: undefined
    };
	
	// Merge Default Options and input Options
	const opts = mergeOptions(defaultOptions, options || {});
	
	// Define Action
	const act = {
		type: 'general', 
		id: 'clearcut', 
		schedule: {absolute: opts.absolute, opt: opts.opt},
		action: function() {
				stand.trees.load('dbh > ' + opts.dbhThreshold);
				stand.trees.harvest()
		}
	};
	
	act.description = `A simple repeating clearcut operation, that removes all trees above a target diameter ( ${opts.dbhThreshold} cm)).`;
	
	return act;
	
};
*/

/**
 * No harvest management
 * @method noHarvest
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.noHarvest();
 */
lib.harvest.noHarvest = function() {
	var act = {
		type: 'general', 
		schedule: { repeat: true, repeatInterval: 100},
		action: function() {
		}
	};
	
	act.description = `No harvest management`;
						
	return act;
};

/**
 * Harvest all trees above a certain height threshold
 * @method HarvestAllBigTrees
 * @param {object} options
 *    @param {string} options.id A unique identifier for the harvest activity (default: 'HarvestAllBigTrees').
 *    @param {object} options.schedule schedule of the harvest (default: {absolute: true, opt: 3}).
 *    @param {string} options.ranking ranking string for filtering trees, e.g. 'height > 10' (default: 'height > 10').
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.HarvestAllBigTrees({
 *         schedule: {absolute: true, opt: 5},
 *         ranking: 'height > 15'
 *     });
 */
lib.harvest.HarvestAllBigTrees = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		id: 'HarvestAllBigTrees',
		schedule: {absolute: true, opt: 3},
		ranking: 'height > 10',
		constraint: undefined,
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	var act = {
		id: opts.id,
		type: 'general', 
		schedule: opts.schedule,
		action: function() {
			stand.trees.load(opts.ranking);
			lib.activityLog('HarvestAllBigTrees'); 
			stand.trees.harvest();
		},
		onCreate: function() { /*this.finalHarvest = true; */},
	};

	act.description = `Harvest all trees with a ${opts.ranking} after ${opts.schedule.opt} years of simulation.`;
	return act;
};

/**
 * Clearcut operation, that removes all trees above a minimum diameter
 * @method clearcut
 * @param {object} options
 *    @param {object} options.schedule schedule of the harvest (default: { minRel: 0.8, optRel: 1, maxRel: 1.2, force: true }).
 *    @param {string} options.id A unique identifier for the harvest activity (default: 'Clearcut').
 *    @param {number} options.dbhThreshold Minimum DBH threshold for harvesting (default: 0).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.clearcut({
 *         schedule: { minRel: 0.7, optRel: 0.9, maxRel: 1.1, force: true },
 *         dbhThreshold: 5
 *     });
 */
lib.harvest.clearcut = function(options) {
    // 1. Default Options
    const defaultOptions = { 
        schedule: { minRel: 0.8, optRel: 1, maxRel: 1.2, force: true }, 
        id: 'Clearcut',
		dbhThreshold: 0,
		constraint: undefined
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});

	const act = { 
			type: "scheduled", 
            id: opts.id,
            schedule: opts.schedule, 
			onSetup: function() { 
                lib.initStandObj(); // create an empty object as stand property
			},
			onEvaluate: function() {
                stand.trees.loadAll(`dbh>${opts.dbhThreshold}`);
				stand.trees.harvest();
                lib.activityLog(`Clearcut executed`);
				return true; 
			},
			onExecute: function() {
				stand.trees.removeMarkedTrees();
            },
            onCreate: function() { this.finalHarvest = true; }
	};
	if (opts.constraint !== undefined) act.constraint = opts.constraint;
	
    act.description = `A clearcut operation, that removes all trees above a minimum diameter of ( ${opts.dbhThreshold} cm)).`;
  return act;  
};

/**
 * Shelterwood harvest system
 * @method shelterwood
 * @param {object} options
 *    @param {string} options.id A unique identifier for the harvest activity (default: 'Shelterwood').
 *    @param {object} options.schedule schedule of the harvest (default: { minRel: 0.7, optRel: 0.8, maxRel: 0.9, force: true }).
 *    @param {number} options.NTrees Number of seed trees to select (default: 40).
 *    @param {number} options.NCompetitors Number of competitor trees to select (default: 1000).
 *    @param {object} options.speciesSelectivity species selectivity object (default: {}).
 *    @param {string} options.ranking ranking string for selecting seed trees, e.g. 'height' (default: 'height').
 *    @param {number} options.repeatInterval interval between repeated harvests (default: 5).
 *    @param {number} options.repeatTimes number of repeated harvests (default: 3).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the harvest program
 * @example
 *     lib.harvest.shelterwood({
 *         schedule: { minRel: 0.6, optRel: 0.7, maxRel: 0.8, force: true },
 *         NTrees: 50,
 *         speciesSelectivity: { 'pisy': 1, 'abal': 0.5 }
 *     });
 */
lib.harvest.shelterwood = function(options) {
    // 1. Default Options
    const defaultOptions = { 
        id: 'Shelterwood',
		schedule: { minRel: 0.7, optRel: 0.8, maxRel: 0.9, force: true }, // activity should start before the optimal rotation length is reached.
        NTrees: 40,
		NCompetitors: 1000,
		speciesSelectivity: {},
		ranking: 'height',
		repeatInterval: 5,
		repeatTimes: 2, // number of harvests before the final harvest, so with e.g. repeatTimes 2 it would be 2 pre-Harvests and lastly one final
		OptionalSignal: undefined,
		constraint: undefined
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});

	// program consists of first a selection treatment, where the seed trees are set to crop trees and all other trees to competitors
	// secondly it consists of a harvest treatment, within every tree gets harvest in multiple harvest activities
	const program = {};

	// select the remaining trees, that should be harvested last
	// those should be the trees, that are i) dominant and ii) the target species of the regeneration
    const select_trees = {
        type: 'thinning',
        id: opts.id +  "_select_trees",
        schedule: opts.schedule,
        thinning: 'selection',
        N: opts.NTrees,
        NCompetitors: opts.NCompetitors,
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.ranking,

		onSetup: function() { 
			lib.initStandObj();
		},

        onEnter: function() {
            stand.obj.lib.Shelterwood_harvest_counter = 0;
        },

        onExit: function() {
            stand.stp.signal('Shelterwood_start_repeat');
  
			if (opts.OptionalSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.OptionalSignal} emitted.`);
			  	stand.stp.signal(opts.OptionalSignal);
			};
        },
        description: `Select ${opts.NTrees} seed trees of the stand.`
    };
	
	program["selector"] = select_trees;

	// harvest competitor trees in the first year and seed trees in the last harvest activity
	const remove_trees = {
        type: 'general',
        id: opts.id + "_remove_trees",
        schedule: { signal: 'Shelterwood_remove'},
        action: function() {

            // first year. Save # of marked competitors
			if (stand.obj.lib.Shelterwood_harvest_counter == 0) {
				var marked = stand.trees.load('markcompetitor=true');
                stand.setFlag('compN', marked);
                lib.dbg(`selectiveThinning: start removal phase. ${marked} trees marked for removal.`);
            };

            // remove equal amount of non seed trees in each harvest
			var n = stand.flag('compN') / (opts.repeatTimes - 1);
            var N_Competitors = stand.trees.load('markcompetitor=true');
			
			if ((N_Competitors - n) > 0) {
				stand.trees.filterRandomExclude(N_Competitors - n);
			}
			
            var harvested = stand.trees.harvest();

            lib.log("Year: " + Globals.year + ", shelterwood harvest");
			lib.activityLog(`shelterwood harvest No. ${stand.obj.lib.Shelterwood_harvest_counter+1}`);
			lib.dbg(`shelterwood harvest: No. ${stand.obj.lib.Shelterwood_harvest_counter+1}, removed ${harvested} trees.`);

			stand.obj.lib.Shelterwood_harvest_counter = stand.obj.lib.Shelterwood_harvest_counter + 1;
        },

        description: `Shelterwood harvest (during ${opts.repeatTimes * opts.repeatInterval} years), that removes all trees in ${opts.repeatTimes} harvests.`
    }

    program["remover"] = remove_trees;

	program['repeater'] = lib.repeater({ schedule: { signal: 'Shelterwood_start_repeat'},
											signal: 'Shelterwood_remove',
											interval: opts.repeatInterval,
											count: (opts.repeatTimes)}); // in final year all crop trees get removed
	
	program['finalHarvest'] = { 
		type: "scheduled", 
		id: opts.id + "_final_harvest",
		schedule: { signal: 'Shelterwood_start_repeat', wait: (opts.repeatInterval * (opts.repeatTimes+1)) }, 

		onEvaluate: function() { 
			return true
		},

		onExecute: function() {
			stand.trees.load('markcompetitor=true or markcrop=true');
			var harvested = stand.trees.harvest();
			stand.trees.removeMarkedTrees();
			
			lib.dbg(`shelterwood final harvest: ${harvested} trees removed.`);
    		lib.activityLog('Shelter wood final harvest'); 
		},
		onCreate: function() { this.finalHarvest = true; }
	};


	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	program.description = `A shelterwood operation, that removes all trees in ${opts.repeatTimes} harvests.`;
	
	return program;  
};

/**
 * Strip cut system
 * @method stripCut
 * @param {object} options
 *    @param {number} options.harvestDirection direction of the strips in degrees (default: 120).
 *    @param {number} options.stripWidth width of each strip in meters (default: 30).
 *    @param {number} options.stripRepetitions number of strips before the next strip is a "first" strip again (default: 5).
 *    @param {number[]} options.harvestIntensities array of harvest intensities for each harvest on the strip (default: [0.7, 0.5, 1]).
 *    @param {number} options.harvestHeightThreshold height threshold for harvesting (default: 30).
 *    @param {number} options.harvestInterval number of years between each harvest activity (default: 5).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.stripCut({
 *         harvestDirection: 90,
 *         stripWidth: 40,
 *         harvestIntensities: [0.6, 0.8, 1]
 *     });
 */
lib.harvest.stripCut = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		harvestDirection : 120, // harvest direction of the strips in degrees; strip orientation is +90°
		stripWidth: 30, // width of each strip
		stripRepetitions: 5, // number of strips before the next strip is a "first" strip again
		
		harvestIntensities: [0.7, 0.5, 1], // harvest intensities of the harvests on the strip (e.g. first removing 70% of all trees in the strip, after 5 years remaining 50% of the rest, after 5 years remaining the rest)
		harvestHeightThreshold: 30, // or another threshold as a gate keeper which strips should be harvested
		harvestInterval: 5, // number of years between each harvest activity
		//heightTreshold: do we need a kind of a top height threshold?
		constraint: undefined,
		
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	const act = {
		// MAYBE ITS BETTER TO HAVE ON EVALUATION WHERE I CHECK IF THE WHOLE STAND IS READY FOR HARVEST INSTEAD OF EVERY PATCH!
		type: "general",
		schedule: {repeat: true, repeatInterval: opts.harvestInterval},
		//schedule: { repeat: true, repeatInterval: opts.RepeatTime}, // maybe not the right thing here...
		onSetup: function() {
			// initialise the patches
			// opts.harvestDirection, opts.stripWidth and opts.stripRepetitions needed.
			stand.patches.list = stand.patches.createStrips(opts.stripWidth,/*horiziontal=*/true);    //needs to be cooler
			
			initStandObj();
			stand.obj.act["NumberOfHarvests"] = 0;
		},
		action: function() {
			var harvestTookPlace = 0;
			
			for (p of stand.patches.list) {
				
				// Step 1: check if harvest should take place... maybe with sth. like the following
				let stripType = ((p.id - 1) % opts.stripRepetitions) + 1 // check the type of harvest (first, second, ..., last or no)
				
				let harvestType = Math.max(stand.obj.act["NumberOfHarvests"] + 2 - stripType, 0) * (stripType - 1 <= stand.obj.act["NumberOfHarvests"]) * ((stand.obj.act["NumberOfHarvests"] + 2 - stripType) <= opts.harvestIntensities.length);
				//Globals.alert("harvestType " + harvestType);
				
				if (harvestType !== 0) {
					// still to do: only harvest if mean height of trees in stand are above harvestHeightThreshold
					
					stand.trees.load('patch=' + p.id);
					stand.trees.sort('height');
					var heightPercentile = stand.trees.percentile(90);
					
					if (heightPercentile >= opts.harvestHeightThreshold) {
						// Step 2: calculate number of trees to be remain on strip maybe: 
						//var treesToRemain = Math.round(stand.trees.count*(1-opts.harvestIntensities[harvestType - 1]));
						//Globals.alert("Harvest! Patch " + p.id + "; Number of Remainers: " + treesToRemain);
						var treesToHarvest = Math.round(stand.trees.count*(opts.harvestIntensities[harvestType - 1]));
					
						stand.trees.filterRandomExclude(treesToHarvest);
						//stand.trees.filter('incsum(1) <= ' + treesToHarvest);
						stand.trees.harvest();	
						lib.activityLog(`Stripcut executed`);
						harvestTookPlace = 1;
					};
								
				};
			} 
		stand.obj.act["NumberOfHarvests"] = stand.obj.act["NumberOfHarvests"] + harvestTookPlace;
		}
	};
	if (opts.constraint !== undefined) act.constraint = opts.constraint;// JM: What does this do?
	
	act.description = `Stripcut system which first devides the stand into ${opts.stripWidth} meter wide stripes and harvests on these stripes every ${opts.harvestInterval} years.`;
  return act;  
};

/**
 * Another strip cut system
 * @method stripCut2
 * @param {object} options
 *    @param {number} options.harvestDirection direction of the strips in degrees (default: 120).
 *    @param {number} options.stripWidth width of each strip in meters (default: 30).
 *    @param {number} options.stripRepetitions number of strips before the next strip is a "first" strip again (default: 5).
 *    @param {number[]} options.harvestIntensities array of harvest intensities for each harvest on the strip (default: [0.7, 0.5, 1]).
 *    @param {number} options.harvestThreshold threshold as a gatekeeper for which strips should be harvested (default: 30).
 *    @param {number} options.harvestInterval number of years between each harvest activity (default: 5).
 *    @param {string} options.constraint constraint, e.g. "topHeight" (default: "topHeight").
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.stripCut2({
 *         harvestDirection: 180,
 *         stripWidth: 25,
 *         harvestIntensities: [0.5, 0.7, 1],
 *         constraint: "basalArea"
 *     });
 */
lib.harvest.stripCut2 = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		harvestDirection : 120, // harvest direction of the strips in degrees; strip orientation is +90°
		stripWidth: 30, // width of each strip
		stripRepetitions: 5, // number of strips before the next strip is a "first" strip again
		
		harvestIntensities: [0.7, 0.5, 1], // harvest intensities of the harvests on the strip (e.g. first removing 70% of all trees in the strip, after 5 years remaining 50% of the rest, after 5 years remaining the rest)
		harvestThreshold: 30, // or another threshold as a gate keeper which strips should be harvested
		harvestInterval: 5, // number of years between each harvest activity
		//heightTreshold: do we need a kind of a top height threshold?
		constraint: "topHeight",
		
        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	opts.constraint = "stand." + opts.constraint + ">" + opts.harvestThreshold;
	
	const act = {
		// MAYBE ITS BETTER TO HAVE ON EVALUATION WHERE I CHECK IF THE WHOLE STAND IS READY FOR HARVEST INSTEAD OF EVERY PATCH!
		type: "general",
		schedule: {repeat: true, repeatInterval: opts.harvestInterval},
		//schedule: { repeat: true, repeatInterval: opts.RepeatTime}, // maybe not the right thing here...
		onSetup: function() {
			// initialise the patches
			// opts.harvestDirection, opts.stripWidth and opts.stripRepetitions needed.
			stand.patches.list = stand.patches.createStrips(opts.stripWidth,/*horiziontal=*/true);    //needs to be cooler
			
			initStandObj();
			stand.obj.act["NumberOfHarvests"] = 0;
			stand.obj.act["NumberOfStrips"] = Math.min(stand.patches.list.length, opts.stripRepetitions);
			
		},
		action: function() {
			var harvestTookPlace = 0;
			
			for (p of stand.patches.list) {
				//Globals.alert("Check patch " + p.id);
				// Step 1: check if harvest should take place... maybe with sth. like the following
				let stripType = ((p.id - 1) % opts.stripRepetitions) + 1 // check the type of harvest (first, second, ..., last or no)
				
				// old let harvestType = Math.max(stand.obj.act["NumberOfHarvests"] + 2 - stripType, 0) * (stripType - 1 <= stand.obj.act["NumberOfHarvests"]) * ((stand.obj.act["NumberOfHarvests"] + 2 - stripType) <= opts.harvestIntensities.length);
				let harvestType = Math.max(stand.obj.act["NumberOfHarvests"] - stripType + 2, 0) * (stand.obj.act["NumberOfHarvests"] - stripType + 1<  opts.harvestIntensities.length);
				//Globals.alert("harvestType " + harvestType);
				
				if (harvestType !== 0) {
					//Globals.alert("Harvest!" + " harvestIntensitie: " + opts.harvestIntensities[harvestType - 1])
					// still to do: only harvest if mean height of trees in stand are above harvestHeightThreshold
					
					stand.trees.load('patch=' + p.id);
					//var treesToRemain = Math.round(stand.trees.count*(1-opts.harvestIntensities[harvestType - 1]));
					var treesToHarvest = Math.round(stand.trees.count*(opts.harvestIntensities[harvestType - 1]));
					
					stand.trees.filterRandomExclude(treesToHarvest);
					//stand.trees.filter('incsum(1) <= ' + treesToHarvest);
					stand.trees.harvest();	
					lib.activityLog(`Stripcut executed`);
					harvestTookPlace = 1;
								
				} else {
					//Globals.alert("No Harvest")
				};
			} 
		stand.obj.act["NumberOfHarvests"] = stand.obj.act["NumberOfHarvests"] + harvestTookPlace;
		//stand.obj.act["NumberOfHarvests"] = stand.obj.act["NumberOfHarvests"] % stand.obj.act["NumberOfStrips"];
		}
	};
	if (opts.constraint !== undefined) act.constraint = opts.constraint;
	
	act.description = `Stripcut system which first devides the stand into ${opts.stripWidth} meter wide stripes and harvests on these stripes every ${opts.harvestInterval} years.`;
  return act;  
};

/**
 * Another strip cut system
 * @method stripCut2
 * @memberof abe-lib.harvest
 * @param {object} options
 *    @param {number} options.harvestDirection direction of the strips in degrees (default: 120).
 *    @param {number} options.stripWidth width of each strip in meters (default: 30).
 *    @param {number} options.stripRepetitions number of strips before the next strip is a "first" strip again (default: 5).
 *    @param {number[]} options.harvestIntensities array of harvest intensities for each harvest on the strip (default: [0.7, 0.5, 1]).
 *    @param {number} options.harvestThreshold threshold as a gatekeeper for which strips should be harvested (default: 30).
 *    @param {number} options.harvestInterval number of years between each harvest activity (default: 5).
 *    @param {string} options.constraint constraint, e.g. "topHeight" (default: "topHeight").
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.stripCut2({
 *         harvestDirection: 180,
 *         stripWidth: 25,
 *         harvestIntensities: [0.5, 0.7, 1],
 *         constraint: "basalArea"
 *     });
 */
lib.harvest.stripCut2 = function(options) {
    // 1. Default Options
    const defaultOptions = {
        harvestDirection: 120, // harvest direction of the strips in degrees; strip orientation is +90°
        stripWidth: 30, // width of each strip
        stripRepetitions: 5, // number of strips before the next strip is a "first" strip again

        harvestIntensities: [0.7, 0.5, 1], // harvest intensities of the harvests on the strip (e.g. first removing 70% of all trees in the strip, after 5 years remaining 50% of the rest, after 5 years remaining the rest)
        harvestThreshold: 30, // or another threshold as a gate keeper which strips should be harvested
        harvestInterval: 5, // number of years between each harvest activity
        constraint: "topHeight",

        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});

    opts.constraint = "stand." + opts.constraint + ">" + opts.harvestThreshold;

    const act = {
        // MAYBE ITS BETTER TO HAVE ON EVALUATION WHERE I CHECK IF THE WHOLE STAND IS READY FOR HARVEST INSTEAD OF EVERY PATCH!
        type: "general",
        schedule: { repeat: true, repeatInterval: opts.harvestInterval },
        //schedule: { repeat: true, repeatInterval: opts.RepeatTime}, // maybe not the right thing here...
        onSetup: function() {
            // initialise the patches
            // opts.harvestDirection, opts.stripWidth and opts.stripRepetitions needed.
            stand.patches.list = stand.patches.createStrips(opts.stripWidth, /*horiziontal=*/ true);   //needs to be cooler

            initStandObj();
            stand.obj.act["NumberOfHarvests"] = 0;
            stand.obj.act["NumberOfStrips"] = Math.min(stand.patches.list.length, opts.stripRepetitions);

        },
        action: function() {
            var harvestTookPlace = 0;

            for (p of stand.patches.list) {
                //Globals.alert("Check patch " + p.id);
                // Step 1: check if harvest should take place... maybe with sth. like the following
                let stripType = ((p.id - 1) % opts.stripRepetitions) + 1 // check the type of harvest (first, second, ..., last or no)

                // old let harvestType = Math.max(stand.obj.act["NumberOfHarvests"] + 2 - stripType, 0) * (stripType - 1 <= stand.obj.act["NumberOfHarvests"]) * ((stand.obj.act["NumberOfHarvests"] + 2 - stripType) <= opts.harvestIntensities.length);
                let harvestType = Math.max(stand.obj.act["NumberOfHarvests"] - stripType + 2, 0) * (stand.obj.act["NumberOfHarvests"] - stripType + 1 < opts.harvestIntensities.length);
                //Globals.alert("harvestType " + harvestType);

                if (harvestType !== 0) {
                    //Globals.alert("Harvest!" + " harvestIntensitie: " + opts.harvestIntensities[harvestType - 1])
                    // still to do: only harvest if mean height of trees in stand are above harvestHeightThreshold

                    stand.trees.load('patch=' + p.id);
                    //var treesToRemain = Math.round(stand.trees.count*(1-opts.harvestIntensities[harvestType - 1]));
                    var treesToHarvest = Math.round(stand.trees.count * (opts.harvestIntensities[harvestType - 1]));

                    stand.trees.filterRandomExclude(treesToHarvest);
                    //stand.trees.filter('incsum(1) <= ' + treesToHarvest);
                    stand.trees.harvest();
                    lib.activityLog(`Stripcut executed`);
                    harvestTookPlace = 1;

                } else {
                    //Globals.alert("No Harvest")
                };
            }
            stand.obj.act["NumberOfHarvests"] = stand.obj.act["NumberOfHarvests"] + harvestTookPlace;
            //stand.obj.act["NumberOfHarvests"] = stand.obj.act["NumberOfHarvests"] % stand.obj.act["NumberOfStrips"];
        }
    };
    if (opts.constraint !== undefined) act.constraint = opts.constraint;

    act.description = `Stripcut system which first devides the stand into ${opts.stripWidth} meter wide stripes and harvests on these stripes every ${opts.harvestInterval} years.`;
    return act;
};

/**
 * Coppice with standards management system
 * @method CoppiceWithStandard
 * @param {object} options
 *    @param {number} options.TargetDBH target DBH for harvesting (default: 80).
 *    @param {number} options.NStandards number of remaining trees per hectare (default: 30).
 *    @param {number} options.RepeatTime time interval between harvests (default: 20).
 *    @param {string|undefined} options.species species to consider (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.CoppiceWithStandard({
 *         TargetDBH: 70,
 *         NStandards: 40,
 *         species: 'fasy'
 *     });
 */
lib.harvest.CoppiceWithStandard = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		TargetDBH: 80, 
		NStandards: 30, // number of remaining trees per ha
		RepeatTime: 20,
		species: undefined,
		constraint: undefined
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	// adjust expression for sort
	if (opts.species === undefined) {
		var RankingExpression = "dbh";
	} else {
		var RankingExpression = "dbh+100*(species=" + opts.species + ")";
	};
	//Globals.alert("RankingExpression " + RankingExpression);
	
	var act = {
		type: 'general', 
		schedule: { repeat: true, repeatInterval: opts.RepeatTime},
		action: function() {
			// harvest all trees above the TargetDBH threshold
			stand.trees.load('dbh>='+opts.TargetDBH);
			stand.trees.harvest();
			
			// harvest all other trees but NStandards
			var NTreesToHarvest = stand.trees.load('dbh<'+opts.TargetDBH) - opts.NStandards;
			stand.trees.sort(RankingExpression);
			stand.trees.filter('incsum(1) <= ' + NTreesToHarvest);
			stand.trees.harvest();
			
			stand.trees.removeMarkedTrees();
			lib.activityLog(`CoppiceWithStandard executed`);
			//stand.activity.finalHarvest=true;
		}
	};
	if (opts.constraint !== undefined) act.constraint = opts.constraint;// JM: What does this do?
	
	act.description = `Coppice with standard strategy: removing every ${opts.RepeatTime} years every but ${opts.NStandards} trees.`;
  return act;  
};


/**
 * Target diameter harvesting system
 * @method targetDBH
 * @param {object} options
 *    @param {number} options.TargetDBH target DBH for harvesting (default: 50).
 *    @param {number} options.RepeatTime time interval between harvests (default: 5).
 *    @param {object} options.dbhList object with DBH thresholds per species (default: {}).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.targetDBH({
 *         TargetDBH: 60,
 *         RepeatTime: 10,
 *         dbhList: { 'fasy': 50, 'abal': 55 }
 *     });
 */
lib.harvest.targetDBH = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		TargetDBH: 50, // JM: here it would be nice to define target dbhs per species!
		RepeatTime: 5, 
		dbhList: {"fasy":65, "frex":60, "piab":45, "quro":75, "pisy":45, "lade":65, "qupe":75, "psme":65, "abal":45, "acps":60, "pini":45}, //source: 'Waldbau auf Ã¶kologischer Grundlage', p.452
		constraint: undefined,
    };
    let opts = lib.mergeOptions(defaultOptions, options || {});
	
	opts.dbhList['rest'] = opts.TargetDBH;  // set the rest to the overall target DBH	
	
	let excludeString = "";
	let isFirst = true;
	// in(species, piab, fasy, ...)=false 
	
	for (var species in opts.dbhList) {
		if (isFirst) {
			excludeString += "species <> ";
			excludeString += species;
			isFirst = false;
		} else {
			excludeString += " and species <> ";
			excludeString += species;
		}
	};
		
	var act = {
		type: 'general', 
		schedule: { repeat: true, repeatInterval: opts.RepeatTime},
		action: function() {
			console.log(`Stand: ${stand.id}, Year: ${Globals.year}, TargetDBH harvest`);
			for (var species in opts.dbhList) {
				var dbh = opts.dbhList[species]
				if (species === 'rest') {
					var N_Trees = stand.trees.load(excludeString + ' and dbh > '+ dbh);
				} else {
					var N_Trees = stand.trees.load('species = ' + species + ' and dbh > ' + dbh);
				};
				
				if (N_Trees > 0) {
					console.log("Species: " + species + ", target DBH: " + dbh + ", Trees: " + N_Trees + " harvested.");
					stand.trees.harvest();
				};
			};
			lib.activityLog(`Harvest targetDBH executed`);
		}
	};
	
	if (opts.constraint !== undefined) act.constraint = opts.constraint;
	
	act.description = `A simple repeating harvest operation (every ${opts.RepeatTime} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return act;
};

/**
 * Target diameter harvesting system for Norway Spruce (No)
 * @method targetDBHforNo3
 * @param {object} options
 *    @param {number} options.TargetDBH target DBH for harvesting (default: 50).
 *    @param {number} options.RepeatTime time interval between harvests (default: 5).
 *    @param {object} options.dbhList object with DBH thresholds per species (default: {}).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.targetDBHforNo3({
 *         TargetDBH: 55,
 *         RepeatTime: 8,
 *         dbhList: { 'fasy': 45, 'abal': 60 }
 *     });
 */
lib.harvest.targetDBHforNo3 = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		TargetDBH: 50, // JM: here it would be nice to define target dbhs per species!
		RepeatTime: 5, 
		dbhList: {},
		constraint: undefined,
    };
    let opts = lib.mergeOptions(defaultOptions, options || {});
	
	opts.dbhList['rest'] = opts.TargetDBH;  // set the rest to the overall target DBH	
	
	let excludeString = "";
	let isFirst = true;
	// in(species, piab, fasy, ...)=false 
	
	for (var species in opts.dbhList) {
		if (isFirst) {
			excludeString += "species <> ";
			excludeString += species;
			isFirst = false;
		} else {
			excludeString += " and species <> ";
			excludeString += species;
		}
	};
		
	var act = {
		type: 'general', 
		schedule: { repeat: true, repeatInterval: opts.RepeatTime},
		action: function() {
			console.log("Year: " + Globals.year + ", TargetDBH harvest");
			for (var species in opts.dbhList) {
				var dbh = opts.dbhList[species]
				if (species === 'rest') {
					stand.trees.load(excludeString + ' and dbh > '+ dbh);
				} else {
					stand.trees.load('species = ' + species + ' and dbh > ' + dbh);
				};
				console.log("Species: " + species + ", target DBH: " + dbh + ", Trees: " + stand.trees.count)
				stand.obj.act["NHarvests"] = stand.obj.act["NHarvests"] + stand.trees.count;
				stand.trees.harvest();
				lib.activityLog(`Harvest targetDBHforNo3 executed`);
			};
			if (stand.obj.act["NHarvests"] > 20) {
				fmengine.runPlanting(stand.id, {species: 'psme', height: 0.4, fraction:1, pattern:'rect2', spacing:10});
				Globals.alert("Planting!");	
				lib.activityLog(`Planting of targetDBHforNo3 executed`);			
			};
			stand.obj.act["NHarvests"] = 0;
		},
		onSetup: function() { 
            lib.initStandObj();
			stand.obj.act["NHarvests"] = 0;
		},
	};
	
	if (opts.constraint !== undefined) act.constraint = opts.constraint;
	
	act.description = `A simple repeating harvest operation (every ${opts.RepeatTime} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return act;
};


