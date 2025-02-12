/**
 * Disturbance response and salvaging
 * @method salvage
 * @param {object} options
 *    @param {expression} options.salvageCondition expression that evaluates for each disturbed tree whether to salvage or not. 
 *                                             If empty, no salvaging of trees takes place. Default: undefined.
 *    @param {number} options.thresholdReset Value between 0 and 1 indicating a disturbance severity threshold. If disturbance is above
 *                                            the threhold, the stand is reset (see other options). What happens depends
 *											  on the options `clearAll`, `onClear` and `signalClear`. Default: 0.5
 *    @param {boolean} options.clearAll  If a stand is reset, a simple clearcut is executed when `clearAll` is `true`. Default: false
 *    @param {string} options.signalClear  If a stand is reset, then this signal is sent if a value is provided. Default: undefined
 *    @param {function} options.onClear  If a stand is reset, this custom function is executed (if provided). Default: undefined
 *    @param {object|undefined} options.schedule default is simply repeating operation, but can be overwritten.
 * @return {object} - An object describing the salvage activity
 * @example
 *     lib.harvest.salvage({ salvageCondition: 'dbh>10', 
 *							 thresholdReset: 0.5, 
 * 							 clearAll: true });
 *         
 */
 
 lib.harvest.salvage = function(options) {
 
     // 1. Default Options
    const defaultOptions = {
		id: 'Salvage',
		schedule: { repeat: true },
		salvageCondition: undefined,
		thresholdReset: 0.5,
		clearAll: undefined, 
		onClear: undefined,
		signalClear: undefined,
		thresholdIgnoreDamage: 50, // threshold m3 volume/ha. if disturbance below, no further tests are executed
        

        // ... add other default parameters
    };

    const opts = lib.mergeOptions(defaultOptions, options || {});

	// use the onExecuteHandler if provided:
	var onExecuteHandler = opts.onClear;
	var description = `Salvage operation.`;
	if (opts.salvageCondition !== undefined)
		description += `Salvage when: '${opts.salvageCondition}'. `;
	if (onExecuteHandler === undefined) {
		if (opts.clearAll === true) {
			description += "When cleared: simple clearcut";
			onExecuteHandler = function() {
				// simple clear-cut: remove all trees > 4m
				const n_trees = stand.trees.loadAll();
				stand.trees.harvest();
				lib.log(`clearcut after disturbance. Harvested ${n_trees} trees.`);
				lib.activityLog('clearcut after disturbance'); 
			}
		} else if (opts.signalClear !== undefined) {
			description += `When cleared: signal '${opts.signalClear}'`;
			onExecuteHandler = function() {
				// send a specific signal instead of specifying code to reset the stand
				lib.log(`clear stand after disturbance. Sent signal ${opts.signalClear}`);
				stand.stp.signal(opts.signalClear);
				lib.activityLog('signal after disturbance'); 
				
			}
		}
	} else {
		description += "When cleared: custom function";
	}
	if (onExecuteHandler === undefined && opts.thresholdReset < 1) {
		throw new Error('harvest.salvage: You need to specify what to do in case the stand should be cleared!');
	}
    return { type: 'salvage', schedule: opts.schedule,
		onExecute: onExecuteHandler,
		thresholdSplitStand: 10, // never split stands
		thresholdIgnoreDamage: opts.thresholdIgnoreDamage,
		thresholdClearStand: opts.thresholdReset,
		debugSplit: false,
		maxPrepone: 0,
		description: description
    };

 
 
 } // end lib.harvest.salvage
 
 