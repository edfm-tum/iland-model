/** Helper functions
* ABE Library
*/

lib.initStandObj = function() {
    if (typeof stand.obj === 'object' && typeof stand.obj.lib === 'object') return;

    if (stand.obj === undefined)
        stand.obj = {}; // set an empty object
    if (stand.obj.lib === undefined)
        stand.obj.lib = {}; // object for library-internal communication

}

/* helper functions within the library */


lib.mergeOptions = function(defaults, options) {
    const merged = {};
    for (const key in defaults) {
        merged[key] = options[key] !== undefined ? options[key] : defaults[key];
    }
    return merged;
}

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

lib.loglevel = 0; // 0: none, 1: normal, 2: debug
lib.log = function(str) {
    if (lib.loglevel > 0)
        fmengine.log(str);
}
lib.dbg = function(str) {
    if (lib.loglevel > 1)
        fmengine.log(str);
}

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

        // ... add other default  parameters
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});

    return {
      type: 'general', schedule: opts.schedule,
        action: function() {
            fmengine.log("The next STP will be: " + opts.STP);
            // TODO: find a way to actually do that :=)
        }
    };

}

/** repeater activity
*/
lib.repeater = function(options) {
    // 1. Default Options
    const defaultOptions = {
        count: undefined, ///< number of repetitions
        interval: 1, ///< interval between repetitions
        signal: undefined, ///< signal of the activity to be executed
        block: true ///< all other activities only resume after the repeater ends
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});

    return {
            type: 'general', schedule: 40,
            action: function() {
                const repeat_interval = opts.interval;
                const repeat_count = opts.count;
                const signal = opts.signal;
                stand.repeat(this,
                    function(n) { console.log(`repeater: emit signal "${opts.signal}". Execution #${n}`);
                                  stand.stp.signal(opts.signal);
                                  //fmengine.runActivity(stand.id, activity);
                                   },
                    opts.interval,
                    opts.count);
                // make sure that only the repeater runs
                if (opts.block)
                    stand.sleep(opts.interval * opts.count);

            }
        }
}
