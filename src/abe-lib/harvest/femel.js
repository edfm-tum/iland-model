

lib.harvest.femel = function(options) {

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

        lib.dbg("femel step...");

        // (1) enlarge the patches
        stand.patches.createExtendedPatch(n, // which patch id
                           n+1, // id for new patch
                           stand.obj.lib.femel.opts.growBy); // number of "rings" (not used)
        stand.obj.lib.lastPatchId = n+1; // the newly created patches have this ID

        // (2) call to action
        stand.stp.signal('step');

        if (n === stand.obj.lib.femel.opts.steps) {
            lib.dbg(`reached end of femel at step ${n}.`);
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
        }
    };
}
