// load library



/** Set up STPs
Clearcut
Clearcut and planting abal everywhere!
*/


const ClearcutHarvest = lib.harvest.clearcut();
const ClearCutPlanting = lib.planting.general({species='piab'});

//var ClearcutSTP = buildProgram(ClearcutHarvest, ClearCutPlanting);
//fmengine.addManagement(ClearcutSTP, 'Clearcut');
lib.createSTP('Clearcut', ClearcutSTP);

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
