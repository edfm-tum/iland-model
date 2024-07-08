// load library



/** Set up STPs
Clearcut
Clearcut and planting abal everywhere!
*/


const ClearcutHarvest = lib.harvest.clearcut();
const ClearCutPlanting = lib.planting.general({species='piab'});

//var ClearcutSTP = buildProgram(ClearcutHarvest, ClearCutPlanting);
//fmengine.addManagement(ClearcutSTP, 'Clearcut');
lib.createSTP('Clearcut', ClearcutHarvest, ClearCutPlanting);


/*const Selector = lib.selectPatches({ select: { /*... options to specify what and how to select ...* / maxArea: 0.5, minArea: 0.1 },
                               id: 'c1'
                           })
const Femel = lib.harvest.femel( { size: 20, n: 3, interval: 10, times: 4, width: 10, source: 'c1'}  )
const PlantInPatch = lib.planting.plantPatch( { patches: 'c1', planting: {},  }) // years before / after start of femel
const NegativeSelection = lib.thinning.negativeSelection( { patches: 'c1', targetSpecies: [], intenity:  }) // years after initial femel

lib.createSTP() */

// test for signals
const act_test1 = { type: 'general', schedule: { //absolute: true,
        opt: 40 },
    action: function() {
        Globals.alert('run test1, emitting signal');
        stand.stp.signal('step');
    }
}

const act_test2 = { type: 'general', schedule: { signal: 'step', wait: 1},
    action: function() {
        Globals.alert('signalled activity executed!');
    }
}

lib.createSTP('testsignal', act_test1, act_test2);


/**
Structure
selective thinning, target diameter harvest and natural regeneration
*/

const StructureThinning = lib.thinning.selectiveThinning({mode = 'dynamic'});
const StructureHarvest = lib.harvest.targetDBH({dbhList = {"fasy":65,   //source: 'Waldbau auf ökologischer Grundlage', p.452
"frex":60, "piab":45, "quro":75, "pisy":45, "lade":65,
"qupe":75, "psme":65, "abal":45, "acps":60, "pini":45}});

//var StructureSTP = buildProgram(StructureThinning, StructureHarvest);
//fmengine.addManagement(StructureSTP, 'Structure');
lib.createSTP('Structure', StructureThinning, StructureHarvest);

/**
No Management
*/

const NoHarvest = lib.harvest.noHarvest();

//var NoSTP = buildProgram(NoHarvest);
//fmengine.addManagement(NoSTP, 'NoMgmt');

lib.createSTP('NoMgmt', NoHarvest);

/**
No 3
plant douglas fir everywhere
*/
//const No3Planting = lib.planting.general({species: 'psme', treeHeight: 0.4})
const No3Thinning = lib.thinning.selectiveThinning({ranking='height+10*species=psme'});
const No3Harvest = lib.harvest.targetDBHforNo3({dbhList = {"fasy":65,   //source: 'Waldbau auf ökologischer Grundlage', p.452
"frex":60, "piab":45, "quro":75, "pisy":45, "lade":65,
"qupe":75, "psme":65, "abal":45, "acps":60, "pini":45}});
lib.createSTP('No3', No3Thinning, No3Harvest);
//var No3STP = buildProgram(No3Thinning, No3Harvest);
//fmengine.addManagement(No3STP, 'No3');
