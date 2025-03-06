/**
 * Femel management system
 * @method femel
 * @param {object} options
 *    @param {number} options.steps number of consecutive enlargement steps after start (default: 2).
 *    @param {number} options.interval number of years between each step (default: 10).
 *    @param {number} options.growBy number of "rings" of 10m cells to grow each step (default: 1).
 *    @param {string|undefined} options.internalSignal internal signal to start each femel harvest step (default: 'harvest_femel').
 *    @param {string|undefined} options.sendSignal signal send out after final femel harvest activity (default: undefined).
 *    @param {object|undefined} options.schedule schedule object (default: undefined).
 *    @param {string|undefined} options.constraint constraint (default: undefined).
 * @return {object} act - An object describing the harvest activity
 * @example
 *     lib.harvest.femel({
 *         steps: 3,
 *         interval: 5,
 *         growBy: 2,
 *         schedule: { start: 10 }
 *     });
 */


lib.harvest.femel = function(options) {
    
    // 1. Default Options
    const defaultOptions = {
        id: 'femelHarvest',
        steps: 2, // number of consecutive enlargement steps after start
        interval: 10, // years between each step
        growBy: 1, // number of "rings" of 10m cells to grow each step
        internalSignal: 'harvest_femel', 
        sendSignal: undefined,
        schedule: undefined,
        constraint: undefined

        // ... add other default parameters
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});

    // Program consists of a repeater and the harvest activity
    const program = {};

    // Initializing activity
    program['Femel_Init'] = { 
        id: opts.id + '_init',
        type: 'general', 
        schedule: opts.schedule,
        action: function() {
            lib.dbg(`Init femel management`);

            lib.initStandObj();
            stand.obj.lib.femel = { opts: opts }; // store femel options

            stand.obj.lib.FemelID = 1; // set first femel ID to harvest to 1

            // already do the first harvest
            stand.trees.load('patch=' + stand.obj.lib.FemelID); 
			var harvested = stand.trees.harvest();

			lib.dbg(`femel activity - harvest: ${harvested} trees removed.`);
        },
    };

    // repeating activity that sends out a signal every time a harvest should take place
    program['Femel_repeater'] = lib.repeater({ 
        id: opts.id + '_repeater',
        schedule: opts.schedule,
        signal: opts.internalSignal,
        interval: opts.interval,
        count: opts.steps
    });

    // activity that increases the extent of the femel
    program['Femel_enlarger']  = { 
        id: opts.id + '_enlarger',
        type: 'general', 
        schedule: { signal: opts.internalSignal },
        action: function() {
            lib.dbg(`femel activity - enlarge femel`);

            // enlarge the patches
            stand.patches.createExtendedPatch(stand.obj.lib.FemelID, // which patch id
                stand.obj.lib.FemelID+1, // id for new patch
                stand.obj.lib.femel.opts.growBy // number of "rings" (not used)
            );

            // set next femel patch to harvest to new patch ID
            stand.obj.lib.FemelID = stand.obj.lib.FemelID+1; // the newly created patches have this ID
        },
    };
	
	// harvest activity to harvest patch 
    program['Femel_harvester'] = { 
        id: opts.id + '_harvester',
        type: 'general', 
        schedule: { signal: opts.internalSignal },
        action: function() {
            stand.trees.load('patch=' + stand.obj.lib.FemelID); 
			var harvested = stand.trees.harvest();

			lib.dbg(`femel activity - harvest: ${harvested} trees removed.`);
    		lib.activityLog('femel harvest');             
        },
    };

    program['Femel_final'] = { 
        id: opts.id + '_final',
		type: "scheduled", 
		schedule: { signal: opts.internalSignal, wait: (opts.interval * opts.steps) },
		onCreate: function() { 
            this.finalHarvest = true; 
        }, 
		onEvaluate: function() { 
			return true
		},
		onExecute: function() {
			//stand.trees.load('patch=0'); // + (stand.obj.lib.FemelID)); 
			//var harvested = stand.trees.harvest();
			
			//lib.dbg(`femel activity - final harvest: ${harvested} trees removed.`);
    		lib.activityLog('femel activity - femel done'); 

            // remove patches from stand
            stand.patches.clear();
            stand.patches.updateGrid(); // to make changes visible
		},
		onExit: function() {
            if (opts.sendSignal !== undefined) {
			  	lib.dbg(`Signal: ${opts.sendSignal} emitted.`);
			  	stand.stp.signal(opts.sendSignal);
			};
        }
	};

    if (opts.constraint !== undefined) program.constraint = opts.constraint;
	
	program.description = `A femel harvest system.`;
	
	return program;  

}

lib.harvest.femelBackup = function(options) {

    // 1. Default Options
    const defaultOptions = {
        steps: 2, // number of consecutive enlargement steps after start
        interval: 10, // years between each step
        growBy: 1, // number of "rings" of 10m cells to grow each step
        schedule: undefined,

        // ... add other default parameters
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});

    function femelStep(n) {

        lib.dbg(`femel step... test: ${opts.steps}`);

        // (1) enlarge the patches
        stand.patches.createExtendedPatch(n, // which patch id
                           n+1, // id for new patch
                           stand.obj.lib.femel.opts.growBy); // number of "rings" (not used)
        stand.obj.lib.lastPatchId = n+1; // the newly created patches have this ID

        // (2) call to action
        stand.stp.signal('step');

        if (n === stand.obj.lib.femel.opts.steps) {
            lib.dbg(`reached end of femel at step ${n}.`);
            stand.activity.enabled = false;
            stand.stp.signal('end');
        }
    }

    return { type: 'general', schedule: opts.schedule,
        action: function() {
            // start femel activity
            // check STP if all required components are here?
            lib.initStandObj();
            stand.obj.lib.femel = { opts: opts }; // store femel options

            stand.stp.signal('start');

            stand.stp.signal('step');

            // set up repeated steps
            stand.repeat(undefined, femelStep,  opts.interval, opts.steps);
            stand.sleep(50); // hack to prevent multiple executions
        },
        onCreate: function() {
            // activity does not "end" after action is called
            // the end is triggered by code (exit())
            this.manualExit = true;
        }
    };
}

