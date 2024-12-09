/** Helper functions
* ABE Library

Building STPs
-------------

The library provides functions to simplify the construction of Stand treatment programs.

+ `lib.createSTP`: takes one or several activites and creates a STP with a given name

Introspection
-------------

+ use `formattedLog()` and `formattedSTP()` for a detailed look into past and plant activitites

Miscallaneous
-------------

+ Logging: use `log()` and `dbg()` functions and `lib.logevel` to control the amount of log information
+ Activity log: use `activityLog()` (internally) to add to the stand-level log data


Internals
-------------

+ `lib.mergeOptions`: help with global / local settings
+ `lib.selectOptimalPatches`: compare patches and select the best based on a criterion

Useful activites
----------------

+ `changeSTP`: set follow-up STP when the current STP ends
+ `repeater`: simple activity to repeatedly run a single JS function / activity


@class lib.helper
*/


/**
*  Initializes the `stand.obj` Javascript object for the current stand (`stand.id`).
*
*  @method lib.initStandObj
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
*  @method lib.initAllStands
*/
lib.initAllStands = function() {
    for (const id of fmengine.standIds) {
      // set the focus of ABE to this stand:
      fmengine.standId = id;
      // access data for the stand
      initStandObj();
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
*  @method lib.log
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
*  @method lib.dbg
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
*    @method lib.mergeOptions
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
*    @method lib.buildProgram
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
*    @method lib.createSTP
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
* @method lib.formattedLog
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
* @method lib.formattedLog
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

/** Create Patch activity
*/
lib.selectOptimalPatches = function(options) {
    // 1. Default Options
    const defaultOptions = {
        N: 4, // select N patches per ha
        patchsize: 2, // 2x2 = 20x20 = 400m2
        spacing: 0, // space (in 10m cells) between candidate patches
        criterium: 'max_light', // fixed options
        customFun: undefined, // custom function
        patchId: 1, // id of selected patches
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

        if (opts.customFun !== undefined) {
            score = opts.customFun(patch);
        } else {

            // pre-defined variables
            switch (opts.criterium) {
            case 'max_light':
                score = stand.patches.lif(patch); break; // get LIF on the cells
            case 'min_light':
                score = - stand.patches.lif(patch); break;
            case 'min_basalarea':
                // evaluate the basal area
                stand.trees.load('patch = ' + patch.id);
                let basal_area = stand.trees.sum('basalarea') / patch.area; // basal area / ha
                score = -basal_area; // top down
                break;
            default:
                throw new Error(`selectOptimalPatches: invalid criterion "${opts.criterium}"!`);
            }

        }
        patch.score = score;
    }



    return {
        type: 'general', schedule: opts.schedule,
        action: function() {
            // (1) init
            stand.patches.clear();
            const n_ha = opts.N * stand.area;
            lib.dbg(`selectOptimalPatches: ${n_ha} / ha, based on ${opts.criterium}.`);

            // (2) create candidate patches
            createPatches(opts);

            // (3) Evaluate patches
            stand.patches.list.forEach((p) => patchEvaluation(p, opts));

            // (4) select patches based on the score provided in the evaluation function
            stand.patches.list = topN(n_ha);

            // (5) set all patches to a single ID
            stand.patches.list.forEach((p) => p.id = opts.patchId);
            stand.patches.updateGrid(); // to make changes visible

        }
    }
}

/* STP switcher */
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

/** repeater activity
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
