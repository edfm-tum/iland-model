// ABE activity invoked by ABELink() via fmengine.runActivity(standId, 'disturbance_response')
var a_disturbance_response = {
    type: 'general',
    schedule: 1000, // not reached; triggered dynamically
    action: function() {
        trees.loadAll();
        trees.harvest();
        trees.removeMarkedTrees();
        // do whatever needs to be done here
        console.log('ABE: disturbance response');
    }
};
