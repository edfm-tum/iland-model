lib.harvest = {};

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
	const opts = mergeOptions(defaultOptions, options || {});
	
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
	const opts = mergeOptions(defaultOptions, options || {});
	
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

lib.harvest.clearcut = function(options) {
    // 1. Default Options
    const defaultOptions = { 
		minRel: 0.9,
		optRel: 1,
		maxRel: 1.15,
		dbhThreshold: 0,
		constraint: undefined,
		
        // ... add other default thinning parameters
    };
	const opts = mergeOptions(defaultOptions, options || {});

	const act = { // available function remain(x) kill all but x random trees
			type: "scheduled", 
			schedule: {minRel: opts.minRel, optRel: opts.optRel, maxRel: opts.maxRel, force: true }, //JM: does it work/make a difference to include repeat=T here aswell?
			onSetup: function() { // JM: do we nee this aswell?
				initStandObj(); // create an empty object as stand property
				//Globals.alert('onSetup ...');
				//printObj(stand.activity);
				// stand.obj.act: some information specific for a activity (coded by unique activity index)
				stand.obj.act[ stand.activity.index ] = { "mon": "dieu!"};
			},
			onEvaluate: function(){ //JM: is this logical for clear cut to only do the harvest on selected, if executed 5 years later?
				//Globals.alert('Evaluated');
				stand.trees.loadAll();
				stand.trees.harvest();
				console.log(stand.obj.act[stand.activity.index].mon);
				return true; 
			},
			onExecute: function() {
				//stand.trees.load('dbh > ' + opts.dbhThreshold);
				//stand.trees.harvest();
				stand.trees.removeMarkedTrees();
				stand.activity.finalHarvest=true;
				//stand.obj.act["harvest.year"] = Globals.year;
			}//,// but do the same thing as the default operation 				
			//onCreate: function() {stand.activity.finalHarvest=true; }
	};
	if (opts.constraint !== undefined) act.constraint = opts.constraint;// JM: What does this do?
	
	act.description = `A simple repeating clearcut operation, that removes all trees above a minimum diameter of ( ${opts.dbhThreshold} cm)).`;
  return act;  
};

lib.harvest.CoppiceWithStandard = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		TargetDBH: 80, 
		NStandards: 30, // number of remaining trees per ha
		RepeatTime: 20,
		species: undefined,
		constraint: undefined
    };
	const opts = mergeOptions(defaultOptions, options || {});
	
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
			//stand.activity.finalHarvest=true;
		}
	};
	if (opts.constraint !== undefined) act.constraint = opts.constraint;// JM: What does this do?
	
	act.description = `Coppice with standard strategy: removing every ${opts.RepeatTime} years every but ${opts.NStandards} trees.`;
  return act;  
};


lib.harvest.targetDBH = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		TargetDBH: 50, // JM: here it would be nice to define target dbhs per species!
		RepeatTime: 5, 
		dbhList: {},
		constraint: undefined,
    };
	let opts = mergeOptions(defaultOptions, options || {});
	
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
				stand.trees.harvest();
			};
		}
	};
	
	if (opts.constraint !== undefined) act.constraint = opts.constraint;// JM: What does this do?
	
	act.description = `A simple repeating harvest operation (every ${opts.RepeatTime} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return act;
};


lib.harvest.targetDBHforNo3 = function(options) {
	// 1. Default Options
    const defaultOptions = { 
		TargetDBH: 50, // JM: here it would be nice to define target dbhs per species!
		RepeatTime: 5, 
		dbhList: {},
		constraint: undefined,
    };
	let opts = mergeOptions(defaultOptions, options || {});
	
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
			};
			if (stand.obj.act["NHarvests"] > 20) {
				fmengine.runPlanting(stand.id, {species: 'psme', height: 0.4, fraction:1, pattern:'rect2', spacing:10});
				Globals.alert("Planting!");				
			};
			stand.obj.act["NHarvests"] = 0;
		},
		onSetup: function() { // JM: do we nee this aswell?
			initStandObj();
			stand.obj.act["NHarvests"] = 0;
		},
	};
	
	if (opts.constraint !== undefined) act.constraint = opts.constraint;// JM: What does this do?
	
	act.description = `A simple repeating harvest operation (every ${opts.RepeatTime} years), that removes all trees above a target diameter ( ${opts.TargetDBH} cm)).`;
						
	return act;
};


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

