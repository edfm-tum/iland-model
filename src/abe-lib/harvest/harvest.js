/**
 * The top-level library module.
 * @module abe-lib
 */

/**
 * The harvest library includes management operations related to harvesting.
 *
 * @class harvest
 * @memberof abe-lib
 */
lib.harvest = {};


Globals.include(lib.path.dir + '/harvest/femel.js');
Globals.include(lib.path.dir + '/harvest/salvage.js');


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
 *    @param {string} options.preferenceFunction ranking string for filtering trees, e.g. 'height > 10' (default: 'height > 10').
 *    @param {string|undefined} options.sendSignal signal send out after activity (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.HarvestAllBigTrees({
 *         schedule: {absolute: true, opt: 5},
 *         preferenceFunction: 'height > 15'
 *     });
 */
lib.harvest.HarvestAllBigTrees = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		id: 'HarvestAllBigTrees',
		schedule: {absolute: true, opt: 3},
		preferenceFunction: 'height > 10',
		sendSignal: undefined,
		constraint: undefined,
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	var act = {
		id: opts.id,
		type: 'general', 
		schedule: opts.schedule,
		action: function() {
			stand.trees.load(opts.preferenceFunction);
			stand.trees.harvest();
			lib.activityLog('HarvestAllBigTrees'); 
		},
		onExit: function() {
            if (opts.sendSignal !== undefined) {
                lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
            }
        },
	};

	act.description = `Harvest all trees with ${opts.preferenceFunction} after ${opts.schedule.opt} years of simulation.`;
	return act;
};

/**
 * Clearcut operation, that removes all trees above a minimum diameter
 * @method clearcut
 * @param {object} options
 *    @param {string} options.id A unique identifier for the harvest activity (default: 'Clearcut').
 *    @param {object} options.schedule schedule of the harvest (default: { minRel: 0.8, optRel: 1, maxRel: 1.2, force: true }).
 *    @param {string} options.preferenceFunction ranking string for filtering trees, e.g. 'dbh > 10' (default: 'dbh > 0').
 *    @param {string|undefined} options.sendSignal signal send out after activity (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.clearcut({
 *         schedule: { minRel: 0.7, optRel: 0.9, maxRel: 1.1, force: true },
 *         preferenceFunction: `dbh > 5`
 *     });
 */
lib.harvest.clearcut = function(options) {
    // 1. Default Options
    const defaultOptions = { 
        id: 'Clearcut',
		schedule: { minRel: 0.8, optRel: 1, maxRel: 1.2, force: true }, 
        preferenceFunction: `dbh > 0`,
		sendSignal: undefined,
		constraint: undefined
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});

	const act = { 
		id: opts.id,
		type: "scheduled", 
		schedule: opts.schedule, 
		onCreate: function() { 
			this.finalHarvest = true; 
		}, 
		onSetup: function() { 
			lib.initStandObj(); 
		},
		onEvaluate: function() {				
			return true; 
		},
		onExecute: function() {
			stand.trace = true;
			let was_simulate = stand.trees.simulate;
			stand.trees.simulate = false;
			stand.trees.load(opts.preferenceFunction);
			stand.trees.harvest();
			stand.trees.simulate = was_simulate;

			lib.activityLog(`Clearcut executed`);
		},
		onExit: function() {
			if (opts.sendSignal !== undefined) {
				lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
				stand.stp.signal(opts.sendSignal);
			}
		}
	};

	if (opts.constraint !== undefined) act.constraint = opts.constraint;
    act.description = `A clearcut operation, that removes all trees with ${opts.preferenceFunction}.`;

	return act;  
};

/**
 * Shelterwood harvest system
 * @method shelterwood
 * @param {object} options
 *    @param {string} options.id A unique identifier for the harvest activity (default: 'Shelterwood').
 *    @param {object} options.schedule schedule of the harvest (default: { minRel: 0.7, optRel: 0.8, maxRel: 0.9, force: true }).
 *    @param {number} options.nTrees Number of seed trees to select (default: 40).
 *    @param {number} options.nCompetitors Number of competitor trees to select (default: 1000).
 *    @param {object} options.speciesSelectivity species selectivity object (default: {}).
 *    @param {string} options.preferenceFunction ranking string for selecting seed trees, e.g. 'height' (default: 'height').
 *    @param {number} options.interval interval between repeated harvests (default: 5).
 *    @param {number} options.times number of repeated harvests (default: 3).
 *    @param {string|undefined} options.internalSignal internal signal to start each each shelterwood activity (default: 'Shelterwood_remove').
 *    @param {string|undefined} options.sendSignal signal send out after last shelterwood activity (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} program - An object describing the harvest program
 * @example
 *     lib.harvest.shelterwood({
 *         schedule: { minRel: 0.6, optRel: 0.7, maxRel: 0.8, force: true },
 *         nTrees: 50,
 *         speciesSelectivity: { 'pisy': 1, 'abal': 0.5 }
 *     });
 */
lib.harvest.shelterwood = function(options) {
    // 1. Default Options
    const defaultOptions = { 
        id: 'Shelterwood',
		schedule: { minRel: 0.7, optRel: 0.8, maxRel: 0.9, force: true }, // activity should start before the optimal rotation length is reached.
        nTrees: 40,
		nCompetitors: 1000,
		speciesSelectivity: {},
		preferenceFunction: 'height',
		interval: 5,
		times: 2, // number of harvests before the final harvest, so with e.g. times 2 it would be 2 pre-Harvests and lastly one final
		internalSignal: 'Shelterwood_remove',
		sendSignal: undefined,
		constraint: undefined
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});

	// program consists of first a selection treatment, where the seed trees are set to crop trees and all other trees to competitors
	// secondly it consists of a harvest treatment, within every tree gets harvest in multiple harvest activities
	const program = {};

	// select the remaining trees, that should be harvested last
	// those should be the trees, that are i) dominant and ii) the target species of the regeneration
    program["Shelterwood_selector"] = {
        id: opts.id +  "_select_trees",
        type: 'thinning',
        schedule: opts.schedule,
        thinning: 'selection',
        N: opts.nTrees,
        NCompetitors: opts.nCompetitors,
        speciesSelectivity: opts.speciesSelectivity,
        ranking: opts.preferenceFunction,

		onSetup: function() { 
			lib.initStandObj();
		},
        onEnter: function() {
            stand.obj.lib.shelterwoodHarvestCounter = 0;
        },
        onExit: function() {
            stand.stp.signal('Shelterwood_start_repeat');
        },
        description: `Select ${opts.nTrees} seed trees of the stand.`
    };

	program['Shelterwood_repeater'] = lib.repeater({ schedule: { signal: 'Shelterwood_start_repeat'},
											signal: opts.internalSignal,
											interval: opts.interval,
											count: opts.times // in final year all crop trees get removed
										});

	// harvest competitor trees in the first year and seed trees in the last harvest activity
	program["Shelterwood_remover"] = {
        id: opts.id + "_remove_trees",
        type: 'general',
        schedule: { signal: opts.internalSignal},
        action: function() {

            // first year. Save # of marked competitors
			if (stand.obj.lib.shelterwoodHarvestCounter == 0) {
				var marked = stand.trees.load('markcompetitor=true');
                stand.setFlag('compN', marked);
                lib.dbg(`selectiveThinning: start removal phase. ${marked} trees marked for removal.`);
            };

            // remove equal amount of non seed trees in each harvest
			var n = stand.flag('compN') / (opts.times - 1);
            var N_Competitors = stand.trees.load('markcompetitor=true');
			
			if ((N_Competitors - n) > 0) {
				stand.trees.filterRandomExclude(N_Competitors - n);
			}
			
            var harvested = stand.trees.harvest();
			stand.obj.lib.shelterwoodHarvestCounter = stand.obj.lib.shelterwoodHarvestCounter + 1;

            lib.dbg("Year: " + Globals.year + ", shelterwood harvest");
			lib.dbg(`shelterwood harvest: No. ${stand.obj.lib.shelterwoodHarvestCounter}, removed ${harvested} trees.`);

			lib.activityLog(`shelterwood harvest No. ${stand.obj.lib.shelterwoodHarvestCounter}`);
        },

        description: `Shelterwood harvest (during ${opts.times * opts.interval} years), that removes all trees in ${opts.times} harvests.`
    }

	program['Shelterwood_final'] = { 
		type: "scheduled", 
		id: opts.id + "_final_harvest",
		schedule: { signal: 'Shelterwood_start_repeat', wait: (opts.interval * opts.times) }, 

		onCreate: function() { 
			this.finalHarvest = true; 
		},
		onEvaluate: function() { 
			return true
		},
		onExecute: function() {
			stand.trace = true;
			let was_simulate = stand.trees.simulate;
			stand.trees.simulate = false;
			stand.trees.load('markcompetitor=true or markcrop=true');
			var harvested = stand.trees.harvest();
			stand.trees.simulate = was_simulate;
			
			lib.dbg(`shelterwood final harvest: ${harvested} trees removed.`);
    		lib.activityLog('Shelter wood final harvest'); 
		},
		onExit: function() {
            if (opts.sendSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
			};
        },
	};


	if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	program.description = `A shelterwood operation, that removes all trees in ${opts.times} harvests.`;
	
	return program;  
};


/**
 * Strip cut system
 * @method stripCut
 * @memberof abe-lib.harvest
 * @param {object} options
 *    @param {string} options.id A unique identifier for the harvest activity (default: 'StripCut').
 *    @param {number} options.harvestDirection direction of the strips in degrees (not working yet) (default: 120).
 *    @param {number} options.stripWidth width of each strip in meters (default: 30).
 *    @param {number} options.nStrips number of strips before the next strip is a "first" strip again (default: 5).
 *    @param {number[]} options.harvestIntensities array of harvest intensities for each harvest on the strip (default: [0.7, 0.5, 1]).
 *    @param {number} options.interval number of years between each harvest activity (default: 5).
 *    @param {string|undefined} options.sendSignal signal send out after each activity (default: undefined).
 *    @param {string} options.constraint constraint, which strips should be harvested e.g. "topHeight>30" (default: "stand.topHeight>30").
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.stripCut2({
 *         harvestDirection: 180,
 *         stripWidth: 25,
 *         harvestIntensities: [0.5, 0.7, 1],
 *         constraint: "stand.basalArea>30"
 *     });
 */
lib.harvest.stripCut = function(options) {
    // 1. Default Options
    const defaultOptions = {
        id: 'StripCut',
		harvestDirection: 120, // harvest direction of the strips in degrees; strip orientation is +90°
        stripWidth: 30, // width of each strip
        nStrips: 5, // number of strips before the next strip is a "first" strip again

        harvestIntensities: [0.7, 0.5, 1], // harvest intensities of the harvests on the strip (e.g. first removing 70% of all trees in the strip, after 5 years remaining 50% of the rest, after 5 years remaining the rest)
        interval: 5, // number of years between each harvest activity
		sendSignal: undefined,
        constraint: "stand.topHeight>30", // gate keeper which strips should be harvested

        // ... add other default thinning parameters
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});

    const act = {
		id: opts.id,
        type: "general",
        schedule: { repeat: true, repeatInterval: opts.interval },
        onSetup: function() {
            // initialise the patches
            // opts.harvestDirection and opts.stripWidth needed.
            stand.patches.list = stand.patches.createStrips(opts.stripWidth, /*horiziontal=*/ true);   //needs to be cooler

            initStandObj();
            stand.obj.act["stripcutHarvestCounter"] = 0;
            opts.nStrips = Math.min(stand.patches.list.length, opts.nStrips);

        },
        action: function() {
            var harvestTookPlace = 0;

            for (p of stand.patches.list) {
                //Globals.alert("Check patch " + p.id);
                // Step 1: check if harvest should take place... maybe with sth. like the following
                let stripType = ((p.id - 1) % opts.nStrips) + 1 // check the type of harvest (first, second, ..., last or no)

                // old let harvestType = Math.max(stand.obj.act["stripcutHarvestCounter"] + 2 - stripType, 0) * (stripType - 1 <= stand.obj.act["stripcutHarvestCounter"]) * ((stand.obj.act["stripcutHarvestCounter"] + 2 - stripType) <= opts.harvestIntensities.length);
                let harvestType = Math.max(stand.obj.act["stripcutHarvestCounter"] - stripType + 2, 0) * (stand.obj.act["stripcutHarvestCounter"] - stripType + 1 < opts.harvestIntensities.length);
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
            stand.obj.act["stripcutHarvestCounter"] = stand.obj.act["stripcutHarvestCounter"] + harvestTookPlace;
        },
		onExit: function() {
            if (opts.sendSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
			};
        },
    };
    if (opts.constraint !== undefined) act.constraint = opts.constraint;

    act.description = `Stripcut system which first devides the stand into ${opts.stripWidth} meter wide stripes and harvests on these stripes every ${opts.interval} years.`;
    return act;
};


/**
 * Coppice with standards management system
 * @method CoppiceWithStandard
 * @param {object} options
 *    @param {number} options.targetDBH target DBH for harvesting (default: 80).
 *    @param {number} options.nStandards number of remaining trees per hectare (default: 30).
 *    @param {number} options.interval time interval between harvests (default: 20).
 *    @param {string} options.preferenceFunction ranking string for selecting standards, e.g. 'dbh+100*species=quro' (default: 'dbh').
 *    @param {string|undefined} options.sendSignal signal send out after each activity (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.CoppiceWithStandard({
 *         targetDBH: 70,
 *         nStandards: 40,
 *         species: 'fasy'
 *     });
 */
lib.harvest.CoppiceWithStandard = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		id: "CoppiceWithStandard",
		targetDBH: 80, 
		nStandards: 30, // number of remaining trees per ha
		interval: 20,
		preferenceFunction: "dbh",
		sendSignal: undefined,
		constraint: undefined
    };
    const opts = lib.mergeOptions(defaultOptions, options || {});
	
	var act = {
		id: opts.id,
		type: 'general', 
		schedule: { repeat: true, repeatInterval: opts.interval},
		action: function() {
			// harvest all trees above the targetDBH threshold
			stand.trace = true;
			let was_simulate = stand.trees.simulate;
			stand.trees.simulate = false;
			stand.trees.load('dbh>='+opts.targetDBH);
			var harvested = stand.trees.harvest();
			
			// harvest all other trees but nStandards
			var nTreesToHarvest = stand.trees.load('dbh<'+opts.targetDBH) - opts.nStandards;
			stand.trees.sort(preferenceFunction);
			stand.trees.filter('incsum(1) <= ' + nTreesToHarvest);
			stand.trees.harvest();
			stand.trees.simulate = was_simulate;
			
			lib.activityLog(`CoppiceWithStandard executed`);
		},
		onExit: function() {
            if (opts.sendSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
			};
        }
	};
	if (opts.constraint !== undefined) act.constraint = opts.constraint;
	
	act.description = `Coppice with standard strategy: removing every ${opts.interval} years every but ${opts.nStandards} trees.`;
  return act;  
};


/**
 * Target diameter harvesting system
 * @method targetDBH
 * @param {object} options
 *    @param {number} options.targetDBH target DBH for harvesting (default: 50).
 *    @param {number} options.times time interval between harvests (default: 5).
 *    @param {object} options.dbhList object with DBH thresholds per species (default: {}).
 *    @param {string|undefined} options.sendSignal signal send out after each activity (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.targetDBH({
 *         targetDBH: 60,
 *         times: 10,
 *         dbhList: { 'fasy': 50, 'abal': 55 }
 *     });
 */
lib.harvest.targetDBH = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		id: 'targetDBH',
		targetDBH: 50, 
		times: 5, 
		dbhList: {"fasy":65, "frex":60, "piab":45, "quro":75, "pisy":45, "lade":65, "qupe":75, "psme":65, "abal":45, "acps":60, "pini":45}, //source: 'Waldbau auf Ã¶kologischer Grundlage', p.452
		sendSignal: undefined,
		constraint: undefined,
    };
    let opts = lib.mergeOptions(defaultOptions, options || {});
	
	opts.dbhList['rest'] = opts.targetDBH;  // set the rest to the overall target DBH	
	
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
		id: opts.id,
		type: 'general', 
		schedule: { repeat: true, repeatInterval: opts.times},
		action: function() {
			lib.dbg(`Stand: ${stand.id}, Year: ${Globals.year}, targetDBH harvest`);
			for (var species in opts.dbhList) {
				var dbh = opts.dbhList[species]
				if (species === 'rest') {
					var N_Trees = stand.trees.load(excludeString + ' and dbh > '+ dbh);
				} else {
					var N_Trees = stand.trees.load('species = ' + species + ' and dbh > ' + dbh);
				};
				
				if (N_Trees > 0) {
					lib.dbg("Species: " + species + ", target DBH: " + dbh + ", Trees: " + N_Trees + " harvested.");
					stand.trees.harvest();
				};
			};
			lib.activityLog(`Harvest targetDBH executed`);
		},
		onExit: function() {
            if (opts.sendSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
			};
        }
	};
	
	if (opts.constraint !== undefined) act.constraint = opts.constraint;
	
	act.description = `A simple repeating harvest operation (every ${opts.times} years), that removes all trees above a target diameter ( ${opts.targetDBH} cm)).`;
						
	return act;
};

/**
 * Target diameter harvesting system for Norway Spruce (No)
 * @method targetDBHforNo3
 * @param {object} options
 *    @param {number} options.targetDBH target DBH for harvesting (default: 50).
 *    @param {number} options.times time interval between harvests (default: 5).
 *    @param {object} options.dbhList object with DBH thresholds per species (default: {}).
 *    @param {string|undefined} options.sendSignal signal send out after each activity (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.targetDBHforNo3({
 *         targetDBH: 55,
 *         times: 8,
 *         dbhList: { 'fasy': 45, 'abal': 60 }
 *     });
 */
lib.harvest.targetDBHforNo3 = function(options) {
	// 1. Default Options
    const defaultOptions = {
		id: 'TargetDBHforNo3', 
		targetDBH: 50, 
		times: 5, 
		dbhList: {},
		sendSignal: undefined,
		constraint: undefined,
    };
    let opts = lib.mergeOptions(defaultOptions, options || {});
	
	opts.dbhList['rest'] = opts.targetDBH;  // set the rest to the overall target DBH	
	
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
		id: opts.id, 
		type: 'general', 
		schedule: { repeat: true, repeatInterval: opts.times},
		onSetup: function() { 
            lib.initStandObj();
			stand.obj.act["NHarvests"] = 0;
		},
		action: function() {
			lib.dbg("Year: " + Globals.year + ", targetDBH harvest");
			for (var species in opts.dbhList) {
				var dbh = opts.dbhList[species]
				if (species === 'rest') {
					stand.trees.load(excludeString + ' and dbh > '+ dbh);
				} else {
					stand.trees.load('species = ' + species + ' and dbh > ' + dbh);
				};
				lib.dbg("Species: " + species + ", target DBH: " + dbh + ", Trees: " + stand.trees.count)
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
		onExit: function() {
            if (opts.sendSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
			};
        }
	};
	
	if (opts.constraint !== undefined) act.constraint = opts.constraint;
	
	act.description = `A simple repeating harvest operation (every ${opts.times} years), that removes all trees above a target diameter ( ${opts.targetDBH} cm)).`;
						
	return act;
};
