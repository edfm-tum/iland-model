

/** Helper functions
* ABE Library

The library provides a number of helper functions.

#### Building STPs

The library provides functions to simplify the construction of Stand treatment programs.

+ `lib.createSTP`: takes one or several activites and creates a STP with a given name

#### Introspection

+ use `formattedLog()` and `formattedSTP()` for a detailed look into past and plant activitites

#### Miscallaneous

+ Logging: use `log()` and `dbg()` functions and `lib.logevel` to control the amount of log information
+ Activity log: use `activityLog()` (internally) to add to the stand-level log data


#### Internals

+ `lib.mergeOptions`: help with global / local settings
+ `lib.selectOptimalPatches`: compare patches and select the best based on a criterion

#### Useful activites

+ `changeSTP`: set follow-up STP when the current STP ends
+ `repeater`: simple activity to repeatedly run a single JS function / activity


@class helper
*/


/**
*  Initializes the `stand.obj` Javascript object for the current stand (`stand.id`).
*
*  @method initStandObj
*/
lib.initStandObj = function() {
    if (typeof stand.obj === 'object' && typeof stand.obj.lib === 'object') return;

    if (stand.obj === undefined)
        stand.obj = {}; // set an empty object
    if (stand.obj.lib === undefined)
        stand.obj.lib = {}; // object for library-internal communication

}

/**
*  Initializes all stands of the current simulation (`initStandObj()`).
*
*  @method initAllStands
*/
lib.initAllStands = function() {
    for (const id of fmengine.standIds) {

      // set the focus of ABE to this stand:
      fmengine.standId = id;
      // access data for the stand
      lib.initStandObj();
    }
}



/**
*  Helper function to get a list of all unique stand ids from the standGrid.
*
*  @method getAllStandGridIds
*/
function getAllStandGridIds() {
    // Get the stand grid ScriptGrid object
    // Globals.grid("standgrid") provides access to the spatial stand grid definition [4].
    let standGrid = Globals.grid("standgrid");

    if (!standGrid) {
        console.log("Error: Stand grid not available.");
        return [];
    }

    // Get all unique stand ids from standGrid
    const uniqueStandIds = Array.from(
        new Set(
            standGrid.values().filter(
                v => Number.isInteger(v) && v > -1
            )
        )
    );

    return uniqueStandIds;
}



/**
*  Sanity check for all stands, if they have a STP.
*
*  @method CheckManagementOfStands
*/
lib.CheckManagementOfStands = function() {
    var allGood = true;
    var standGridIDs = getAllStandGridIds();
    
    let nonExistingStands = [];

    for (const id of standGridIDs) {
        // check if stand id is a valid id within the FM engine
        if (fmengine.isValidStand(id) === false) {
            nonExistingStands.push(id);
            allGood = false;
            console.log(`CheckManagementOfStands: stand ${id} has no stp set.`)
        }
    };
    console.log(`CheckManagementOfStands: All stands have stp set: ${allGood}`);

    if (allGood === false) {
        throw new Error(`CheckManagementOfStands: stands ${nonExistingStands} have no stp set.`);
    }    
}

lib.loglevel = 0; // 0: none, 1: normal, 2: debug
/**
*  Internal function to log a string `str` to the iLand logfile.
*  You can control the amount of log messages by setting `lib.loglevel` to the following values:
*  * `0`: None - no messages from abe-library
*  * `1`: Normal - limited amount of messages from the library, usually only high level
*  * `2`: Debug - high number of log messages. Use for debugging and not in productive model applications.
*
*  See also: lib.dbg()
*
*  @example
*      // set log level (e.g. in iLand Javascript console or in your code) after loading the library
*      lib.loglevel = 2; //debug!
*      ...
*      // somewhere in library code: running the function now produces log messages
*      lib.dbg(`selectiveThinning: repeat ${stand.obj.lib.selective_thinning_counter}, removed ${harvested} trees.`);
*
*
*  @param str {String} string to log
*  @method log
*/
lib.log = function(str) {
    if (lib.loglevel > 0)
        fmengine.log(str);
}
/**
*  Internal debug function to log a string `str` to the iLand logfile.
*  You can control the amount of log messages by setting `lib.loglevel` to the following values:
*  * `0`: None - no messages from abe-library
*  * `1`: Normal - limited amount of messages from the library, usually only high level
*  * `2`: Debug - high number of log messages. Use for debugging and not in productive model applications.
*
*  See also: lib.dbg()
*
*  @param str {String} string to log
*  @method dbg
*/
lib.dbg = function(str) {
    if (lib.loglevel > 1)
        fmengine.log(str);
}

/* helper functions within the library */


lib.mergeOptions2 = function(defaults, options) {
    const merged = {};
    for (const key in defaults) {
        merged[key] = options[key] !== undefined ? options[key] : defaults[key];
    }
    return merged;
}

/**
*  Helper function to combine deafult options and user-provided options for activities.
*
*  The function throws an error when `options` include values not defined in `defaults`.
*  You should therefore define all potential keys that can be used by the user with value `undefined`!
*
*    @example
*      // pattern
*      const default_options = { schedule: undefined, intensity: 10 };
*      // create an object with combined options (even if not provided)
*      const opts = lib.mergeOptions(defaultOptions, options || {});
*      // ...
*
*
*    @param defaults object containing default values
*    @param options object user-defined options
*    @return object that contains default values updated with user-provided options
*    @method mergeOptions
*/
lib.mergeOptions = function(defaults, options) {
  const merged = {};
  for (const key in defaults) {
    if (options && options.hasOwnProperty(key)) {
      merged[key] = options[key];
    } else {
      merged[key] = defaults[key];
    }
  }

  // Check for invalid options
  const validOptions = Object.keys(defaults);
  for (const key in options) {
    if (!defaults.hasOwnProperty(key)) {
        throw new Error(`Invalid option: "${key}". \nValid options are: ${validOptions.join(', ')}`);
    }
  }

  return merged;
}
/**
*  Library function to build a full iLand STP from a collection of elements.
*
*  The function takes one or multiple elements that are typically Javascript objects to define iLand activities.
*  These Javascript objects typically are returnd by library functions; for example, the library function `lib.thinning.tending()`
*  returns (one or more) *definitions* of iLand activities. `buildProgram` combines multiple elements to a single Javascript
*  object. Note that this function does not intitalize iLand activities - it merely operates on Javascript objects. Use `createSTP`
*  to actually create a stand treatment program in iLand!
*
*  @see
*
*    @example
*      //
*       const StructureThinning = lib.thinning.selectiveThinning({mode = 'dynamic'});
*       const StructureHarvest = lib.harvest.targetDBH({dbhList = {"fasy":65,   //source: 'Waldbau auf ökologischer Grundlage', p.452
*            "frex":60, "piab":45, "quro":75, "pisy":45, "lade":65,
*            "qupe":75, "psme":65, "abal":45, "acps":60, "pini":45}});
*
*        const stp = lib.buildProgram(StructureThinning, StructureHarvest);
*        // you can still modify the program, e.g, by adding activities!
*        stp['a_new_activity'] = { type: 'general', schedule: .... };
*
*
*
*    @param concepts one or multiple concepts, typically the result of calls to library function
*    @return object a definitions of multiple activities combined in a single object
*    @method buildProgram
*/

lib.buildProgram = function (...concepts) {
  const program = {};
  for (const conceptResult of concepts) {
    if (Array.isArray(conceptResult)) { // Multiple results
      conceptResult.forEach((activity, index) => {
          const key = `${conceptResult[0].type}${index + 1}`; // Or your naming logic
          program[key] = activity;
      });
    } else { // Single result
      program[conceptResult.type] = conceptResult;
    }
  }
  return program;
}


/**
*  Main library function to create stand treatment programs in iLand.
*
*  The function takes one or multiple elements that are typically Javascript objects to define iLand activities, combines them,
*  and creates a STP in iLand. If the STP already exists, it updates the existing STP (using `fmengine.updateManagement`),
*  and creates a new stp otherwise (using `fmengine.addManagement`).
*
*
*  See also: {{#crossLink "lib.helper/buildProgram:method"}}{{/crossLink}},{{#crossLink "FMEngine/updateManagement:method"}}{{/crossLink}}
*
*    @example
*      //
*       const StructureThinning = lib.thinning.selectiveThinning({mode = 'dynamic'});
*       const StructureHarvest = lib.harvest.targetDBH({dbhList = {"fasy":65,   //source: 'Waldbau auf ökologischer Grundlage', p.452
*            "frex":60, "piab":45, "quro":75, "pisy":45, "lade":65,
*            "qupe":75, "psme":65, "abal":45, "acps":60, "pini":45}});
*
*        lib.createSTP('my_structure_stp',StructureThinning, StructureHarvest);
*
*    @param stp_name {string} name of the stand treatment program
*    @param concepts one or multiple concepts, typically the result of calls to library function
*    @method createSTP
*/
lib.createSTP = function(stp_name, ...concepts) {
    const program = {};
    let activityCounter = 1;
    for (const conceptResult of concepts) {
        if (Array.isArray(conceptResult)) {
            for (const activity of conceptResult) {
                program[`activity${activityCounter}`] = activity;
                activityCounter++;
            }
        } else {
            program[`activity${activityCounter}`] = conceptResult;
            activityCounter++;
        }
    }

    if (fmengine.isValidStp(stp_name)) {
        // the STP already exists, so update the program
        fmengine.updateManagement(program, stp_name);
    } else {
        // the STP does not yet exist, add to fmengine
        fmengine.addManagement(program, stp_name);
    }
}

/** Management history
    Stands store a activity log, also used for DNN training.
    `activityLog` is the internal function to populate the management history. The `extraValues` is
    any value that can be defined by the activity itself.

See also: TODO: write management history, activityLog()

@param actName {String} the activity name to log
@param extraValues {String}  extra values (activity specific)
@method activityLog
*/
lib.activityLog = function(actName, extraValues) {
    if (typeof stand.obj !== 'object' || typeof stand.obj.lib !== 'object')
        throw new Error(`activityLog() for stand ${stand.id}: stand.obj not available. Call lib.initStandObj()!`);
    if (typeof stand.obj.history !== 'object')
        stand.obj.history = [];
    const log_item = { year: Globals.year,
                       standId: stand.id,
                       stp: stand.stp.name,
                       activity: actName,
                       details: extraValues};
    stand.obj.history.push(log_item);
}

/**
*  returns the activity log for the currently active stand as formatted HTML.
*
*
*  @example
*      // show the log for stand 13 in a popup window
*      fmengine.standId = 13;
*      Globals.alert( lib.formattedLog() );
*
* @method formattedLog
*/
lib.formattedLog = function(StandId) {
    // set the focus of ABE to the input StandID if provided
    if (StandId !== undefined) {
        fmengine.standId = StandId;
      }
    // also add that only existing standIDs can be provided

  if (typeof stand.obj !== 'object' || !Array.isArray(stand.obj.history)) {
    return "No activity log available.";
  }

  let htmlLog = `<h1>Activity Log - ${stand.id}</h1>`;
    stand.obj.history.forEach(item => {
      const year = item.year;
      htmlLog += `<div>
                    <h2>${year} - ${item.activity}</h2>
                    <ul>
                      <li>Stand ID: ${item.standId}</li>
                      <li>STP: ${item.stp}</li>`;
      if (item.details) {
        htmlLog += `<li>Details: <pre>${JSON.stringify(item.details, null, 2)}</pre></li>`;
      }
      htmlLog += `</ul>
                  </div>`;
    });

    return htmlLog;

}

/**
*  returns a readable description of the current STP as formatted HTML.
*
*  The description is a list of activites of the STP that is assigned the currently active stand.
*  Items are greyed out if they have already been run in the current rotation, provide the planned year
*  and a detailed descriptions (as provdied by the library functions)
*
*  @example
*      // show the log for stand 13 in a popup window
*      fmengine.standId = 13;
*      Globals.alert( lib.formattedSTP() );
*
* @method formattedSTP
*/
lib.formattedSTP = function(StandId) {
    // set the focus of ABE to the input StandID if provided
    if (StandId !== undefined) {
        fmengine.standId = StandId;
      }
    // also add that only existing standIDs can be provided

  let htmlLog = `<h1>Planned Activities</h1><h2>Stand: ${stand.id} STP: ${stand.stp.name}</h2>`;
  for (name of stand.stp.activityNames) {
      let act = stand.activityByName(name);
      let col = act.active ? 'black' : 'gray';
      let year = act.optimalTime > 10000 ? '(signal)' : act.optimalTime;
      htmlLog += `<div>
                    <h3 style="color: ${col};">${year} - ${act.name}</h3>
                    <ul><li>Active (in this rotation): <b>${act.active}</b></li>
                      <li>Enabled (at all): <b>${act.enabled}</b></li>
                      <li>Description: ${act.description}</li>
                    </ul>
                   </div> `;
  }
  return htmlLog;


}

/**
 * Selects optimal patches based on a given criterion.
 *
 * This function helps with selecting the best patches in a stand based on a specified criterion
 * (e.g., light availability, basal area) or a custom function. It's useful for setting up
 * spatially explicit management activities like creating gaps or targeting specific areas
 * within a stand for treatments.
 *
 * @method selectOptimalPatches
 * @param {object} options Options for configuring the patch selection.
 *   @param {string} options.id A unique identifier for the activity (default: 'selectOptimalPatches').
 *   @param {number} options.N Number of patches to select per hectare (default: 4).
 *   @param {number} options.patchsize Size of the patches (in cells, assuming a square shape, e.g., 2 for 2x2 cells, which is 20x20m or 400m2) (default: 2).
 *   @param {number} options.spacing Space (in 10m cells) between candidate patches (default: 0).
 *   @param {string} options.criterium Criterion for selecting patches ('max_light', 'min_light', or 'min_basalarea') (default: 'max_light').
 *   @param {function|undefined} options.customFun Custom function for evaluating patches. This function should take a patch object as input and return a score (default: undefined).
 *   @param {number} options.patchId ID to assign to the selected patches (default: 1).
 *   @param {string} options.sendSignal Optional: Signal that should be send after selecting patches (default: undefined).
 *   @param {object} options.schedule Schedule object for triggering the patch selection (default: { signal: 'start' }).
 * @return {object} act - An object describing the patch selection activity.
 * @example
 *     // Select 5 patches per hectare based on maximum light availability, using 3x3 patches.
 *     lib.selectOptimalPatches({
 *         N: 5,
 *         patchsize: 3,
 *         criterium: 'max_light'
 *     });
 *
 *     // Select 2 patches per hectare based on a custom evaluation function.
 *     lib.selectOptimalPatches({
 *         N: 2,
 *         customFun: function(patch) {
 *             // Example: Score based on proximity to a specific location
 *             const targetX = 50;
 *             const targetY = 50;
 *             const distance = Math.sqrt(Math.pow(patch.x - targetX, 2) + Math.pow(patch.y - targetY, 2));
 *             return 1 / distance; // Higher score for closer patches
 *         }
 *     });
 */
lib.selectOptimalPatches = function(options) {
    // 1. Default Options
    const defaultOptions = {
        id: 'selectOptimalPatches',
        N: 4, // select N patches per ha
        patchsize: 2, // 2x2 = 20x20 = 400m2
        spacing: 0, // space (in 10m cells) between candidate patches
        criterium: 'max_light', // fixed options
        customPrefFunc: undefined, // custom preference function
        patchId: 1, // id of selected patches
        sendSignal: undefined, // optional: emit signal after patches have been selected
        schedule: { signal: 'start' }, // default behavior: trigger on 'start'

        // ... add other default  parameters
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});

    // code for patches
    function createPatches(opts) {
        // overwrite patches with a regular pattern (with some at least patchsize spacing)
        stand.patches.list =  stand.patches.createRegular(opts.patchsize,opts.spacing);
    }
    // select the N patches with top scores
    function topN(n) {
        // sorting *directly* within stand.patches.list does not work.
        // instead:
        var slist = stand.patches.list;
        const pcount = stand.patches.list.length;

        // sort score (descending)
        slist.sort( (a,b) => b.score - a.score);

        // reduce to the N patches with top score
        slist = slist.slice(0, n);
        lib.dbg(`topN: before: ${pcount} patches, after: ${slist.length} patches.`);
        return slist;
    }
    function patchEvaluation(patch, opts) {
        var score = 0;

        // pre-defined variables
        switch (opts.criterium) {
            case 'max_light':
                score = stand.patches.lif(patch); 
                break; // get LIF on the cells
            case 'min_light':
                score = - stand.patches.lif(patch); 
                break;
            case 'min_basalarea':
                // evaluate the basal area
                stand.trees.load('patch = ' + patch.id);
                var basal_area = stand.trees.sum('basalarea') / patch.area; // basal area / ha
                score = -basal_area; // top down
                break;
            case 'max_basalarea':
                // evaluate the basal area
                stand.trees.load('patch = ' + patch.id);
                var basal_area = stand.trees.sum('basalarea') / patch.area; // basal area / ha
                score = basal_area; 
                break;
            case 'custom':
                if (opts.customPrefFunc === undefined) {
                    throw new Error(`selectOptimalPatches: the custom preference function 'customPrefFunc' is not defined!`);
                }
                // evaluate the custom function
                stand.trees.load('patch = ' + patch.id);
                var score = stand.trees.sum(opts.customPrefFunc) / patch.area; // calculate score based on custom function / ha
                break;
            default:
                throw new Error(`selectOptimalPatches: invalid criterion "${opts.criterium}"!`);
        }
        patch.score = score;
    }



    return {
        id: opts.id,
        type: 'general', 
        schedule: opts.schedule,
        action: function() {
            // (1) init
            stand.patches.clear();
            const n_ha = opts.N * stand.area;
            lib.log(`selectOptimalPatches: ${n_ha} / ha, based on ${opts.criterium}.`);

            // (2) create candidate patches
            createPatches(opts);

            // (3) Evaluate patches
            stand.patches.list.forEach((p) => patchEvaluation(p, opts));

            // (4) select patches based on the score provided in the evaluation function
            stand.patches.list = topN(n_ha);

            // (5) set all patches to a single ID
            stand.patches.list.forEach((p) => p.id = opts.patchId);
            stand.patches.updateGrid(); // to make changes visible

        },
        onExit: function() {
            if (opts.sendSignal !== undefined) {
                lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
            }
        },
    }
}

/**
 * Switches the active Stand Treatment Program (STP) for a stand.
 *
 * This function allows you to dynamically change the STP that is being applied to a stand during a simulation.
 * It's particularly useful for implementing adaptive management strategies or scenarios where different
 * management regimes should be applied based on specific conditions or triggers.
 *
 * @method changeSTP
 * @param {object} options Options for configuring the STP change.
 *   @param {string} options.STP The name of the STP to switch to. This STP must already be defined in the iLand project.
 *   @param {object} options.schedule Schedule object for triggering the STP change (default: { signal: 'end' }).
 *   @param {string} options.id A unique identifier for the activity (default: 'change_stp').
 * @return {object} act - An object describing the STP change activity.
 * @example
 *   // Switch to the 'harvest_STP' when the 'start' signal is received.
 *   lib.changeSTP({
 *       STP: 'harvest_STP',
 *       schedule: { signal: 'start' }
 *   });
 *
 *   // Switch to 'passive_STP' after 100 years.
 *    lib.changeSTP({
 *       STP: 'passive_STP',
 *       schedule: { absolute: true, opt: 100}
 *   });
 */
lib.changeSTP = function(options) {
    // 1. Default Options
    const defaultOptions = {
        STP: undefined, // the STP that should follow after the end of the currently running STP
        schedule: { signal: 'end' }, // default behavior: trigger on end signal
        id: 'change_stp'
        // ... add other default  parameters
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});
    if (!fmengine.isValidStp(opts.STP)) {
        throw new Error(`lib.changeSTP: the target STP "${opts.STP}" is not available!`);
    }

    return {
      type: 'general', schedule: opts.schedule,
        id: opts.id,
        action: function() {
            fmengine.log("The next STP will be: " + opts.STP);
            // TODO: find a way to actually do that :=)
            stand.setSTP(opts.STP);
        }
    };

}

/**
 * Creates a repeater activity that repeatedly triggers a specified signal.
 *
 * This function is useful for creating activities that need to be executed multiple times
 * at regular intervals, such as repeated thinnings or harvests. The repeater can be
 * configured to trigger based on a schedule or a signal, and it can optionally block
 * other activities until it has finished.
 *
 * @method repeater
 * @param {object} options Options for configuring the repeater.
 *   @param {object} options.schedule Schedule object for starting the repeater.
 *   @param {string} options.id A unique identifier for the repeater activity (default: 'repeater').
 *   @param {number} options.count Number of times to repeat the signal (default: undefined).
 *   @param {number} options.interval Interval (in years) between repetitions (default: 1).
 *   @param {string} options.signal Name of the signal to emit at each repetition.
 *   @param {boolean} options.block Whether the repeater should block other activities until it finishes (default: true).
 *   @param {function|undefined} options.parameter Function to provide the signal parameter when the signal is emitted (default: undefined).
 * @return {object} act - An object describing the repeater activity.
 * @example
 *   // Repeat the 'thinning' signal 5 times every 10 years, starting in year 50.
 *   lib.repeater({
 *       schedule: { start: 50 },
 *       count: 5,
 *       interval: 10,
 *       signal: 'thinning'
 *   });
 *
 *   // Repeat the 'harvest' signal 3 times every 2 years, triggered by the 'ready_for_harvest' signal,
 *   // and provide a custom parameter to the signal.
 *   lib.repeater({
 *       schedule: { signal: 'ready_for_harvest' },
 *       count: 3,
 *       interval: 2,
 *       signal: 'harvest',
 *       parameter: function() {
 *           // Example: Return the current stand's basal area as the parameter.
 *           return stand.basalArea();
 *       }
 *   });
 */
lib.repeater = function(options) {
    // 1. Default Options
    const defaultOptions = {
        schedule: undefined, ///< when to start the repeater
        id: 'repeater',
        count: undefined, ///< number of repetitions
        interval: 1, ///< interval between repetitions
        signal: undefined, ///< signal of the activity to be executed
        block: true, ///< all other activities only resume after the repeater ends
        parameter: undefined ///< function that provides the signal parameter
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});
    if (opts.signal === undefined)
        throw new Error("Repeater: signal is required!");

    return {
            type: 'general', schedule: opts.schedule,
            id: opts.id,
            action: function() {
                const repeat_interval = opts.interval;
                const repeat_count = opts.count;
                const signal = opts.signal;
                stand.repeat(this,
                    function() {
                        console.log(`repeater: emit signal "${opts.signal}"`);
                        let param = opts.parameter;
                        if (typeof opts.parameter === 'function')
                            param = opts.parameter.call(opts);

                        stand.stp.signal(opts.signal, param);
                    },
                    opts.interval,
                    opts.count);

                // make sure that only the repeater runs
                if (opts.block)
                    stand.sleep(opts.interval * opts.count);

            },
        }
}


/**
 * Constructs a iLand expression for filtering trees based on the proportion of target species in the stand.
 * When the relative basal area of all target species combined is below `threshold`, then
 * trees of these species are filtered out. The chance of being filtered out declines linearly up to
 * 2x threshold, above no trees are filtered.
 * The species list can be provided in multiple formats.
 *
 * @param {string | string[] | object} speciesList  The list of species. Can be:
 *                                                  - A single species ID string (e.g., "quro").
 *                                                  - An array of species ID strings (e.g., ["quro", "qupe"]).
 *                                                  - An object where keys are species IDs (e.g., 'quro') and values are ignored.
 *                                                  - A function 
 * @param {number} threshold The relative basal area threshold  (0..1) for filtering.
 * @returns {string} The filter string.
 * @method buildRareSpeciesFilter
 */
lib.buildRareSpeciesFilter = function(speciesList, threshold) {
    let speciesIds;

    // 1. Normalize the speciesList input to an array of species IDs.
    if (typeof speciesList === 'string') {
        speciesIds = [speciesList]; // Single species ID
        lib.dbg(`speciesIds: ${speciesIds}`)        
    } else if (Array.isArray(speciesList)) {
        speciesIds = speciesList; // Array of species IDs
        lib.dbg(`speciesIds: ${speciesIds}`)        
    } else if (typeof speciesList === 'object' && speciesList !== null) {
        speciesIds = Object.keys(speciesList); // Object: extract keys
        lib.dbg(`speciesIds: ${speciesIds}`)        
    } else if (typeof speciesList === 'function' && speciesList !== null) {
        speciesIds = Object.keys(speciesList.call()); // Object: extract keys
        lib.dbg(`speciesIds: ${speciesIds}`)        
    } else {
        // Handle invalid input (optional, but good practice)
        throw new Error(`Invalid speciesList format! ${speciesList}, Type: ${typeof speciesList}`);
    }

    // Loop over each species, if there is a list of unique thresholds for each species in speciesList
    if (typeof threshold !== 'number') {
        let thresholds;

        if (typeof threshold === 'object' && threshold !== null) {
            thresholds = Object.values(threshold) // Object: extract values
            lib.dbg(`thresholds: ${thresholds}`)  
        } else if (typeof threshold === 'function' && threshold !== null) {
            thresholds = Object.values(threshold.call()); // Object: extract keys
            lib.dbg(`thresholds: ${thresholds}`)
        } else {
            // Handle invalid input (optional, but good practice)
            throw new Error(`Invalid threshold format! ${threshold}, Type: ${typeof threshold}`);
        }
        
        // check if length of speciesIds and thresholds is equal
        if (speciesIds.length !== thresholds.length) {
            throw new Error(`buildRareSpeciesFilter: Length of speciesIds and thresholds not equal! speciesIds: ${speciesIds}, thresholds: ${thresholds}`);
        }
        
        // save one filter string for each species
        const filterParts = [];

        // loop over speciesIds        
        for (let i = 0; i < speciesIds.length; i++) {
            // set speciesId and species specific threshold
            var speciesId = speciesIds[i];
            var threshold = thresholds[i];

            const relBA = stand.relSpeciesBasalAreaOf(speciesId);
            const threshold2x = 2 * threshold;
        
            let pfilter = 0;
            if (relBA <= threshold) {
                pfilter = 0;
            } else if (relBA >= threshold2x) {
                pfilter = 1;
            } else {
                pfilter = 1 - (threshold2x - relBA) / threshold;
                pfilter = Math.max(0, Math.min(1, pfilter));
            }
        
            lib.dbg(`buildRareSpeciesFilter: Species: ${speciesId}, rel basal area: ${relBA}, threshold: ${threshold}, pFilter: ${pfilter}`);
        
            // Build per-species condition
            const condition = `if(species = ${speciesId}, rnd(0,1) < ${pfilter.toFixed(3)}, true)`;
            filterParts.push(condition);
        }
    
        // Combine all species conditions
        const combinedFilter = filterParts.join(' and ');
        console.log(combinedFilter);
        return combinedFilter || 'true'; // if empty, return always true

    } else {
        // 2. Calculate the total relative basal area.
        let totalBasalArea = 0;
        for (const speciesId of speciesIds) {
            totalBasalArea += stand.relSpeciesBasalAreaOf(speciesId);
        }
    
        // 3. Determine the filtering probability (pfilter).
        const threshold2x = 2 * threshold;
        let pfilter = 0;
    
        if (totalBasalArea <= threshold) {
            pfilter = 0; // filter out all trees
        } else if (totalBasalArea >= threshold2x) {
            pfilter = 1; // keep the trees
        } else {
            // a linear function between threshold and 2xthreshold
            pfilter = 1 - (threshold2x - totalBasalArea) / threshold;
            pfilter = Math.max(0, Math.min(1, pfilter));
        }
        lib.dbg(`buildRareSpeciesFilter: rel basal area of targets: ${totalBasalArea}. pFilter: ${pfilter}`);
    
        // 4. Construct the filter string.
        if (pfilter == 1) {
            return 'true'; // the filter is inactive
        } else {
            const speciesCodes = speciesIds.join(', ');  // Use the normalized speciesIds array
            const filterString = `if( in(species, ${speciesCodes}), rnd(0,1)<${pfilter.toFixed(3)}, true)`;
            return filterString;
        }
    }   

}


