// Force a prescribed fire ignition in simulation year 2 inside stocked RU 1
function onAfterCreate() {
    Fire.onIgnition = function() {
        if (Globals.year === 2) {
            Fire.ignite(50, 50, 25000, 15, 270);
        }
    };
}
