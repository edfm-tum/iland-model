/**
 *
 * @title Forest Management Library
 * @description abe-lib documentation
 *
 * ### ABE Forest Management Library
 * This library provides a set of pre-defined building blocks to construct forest management on top of the iLand ABE system. Each building block can be further customized, and "standard" ABE / JS activities can be used as well.
 *
 * This library is structured around several key modules:
 *
 * ### Core Modules:
 *  - <a href="../classes/harvest.html">harvest</a> The harvest library includes management operations related to harvesting.
 *  - <a href="../classes/planting.html">planting</a> The planting library includes management operations related to artificial and natural regeneration.
 *  - <a href="../classes/thinning.html">thinning</a> The thinning module includes activities for thinning operations.
 *
 * ### Utility Modules:
 *  - {{#crossLink "abe-lib.helper"}}{{/crossLink}} Provides helper functions to build and inspect stand treatment programs (STPs).
 *
 * ### Main functions per category
 *  - Harvest:
 *      - {{#crossLink "abe-lib.harvest.clearcut"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.harvest.shelterwood"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.harvest.femel"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.harvest.salvage"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.harvest.stripCut"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.harvest.targetDBH"}}{{/crossLink}}
 *
 *  - Planting:
 *      - {{#crossLink "abe-lib.planting.general"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.planting.dynamic"}}{{/crossLink}}
 *
 *  - Thinning:
 *      - {{#crossLink "abe-lib.thinning/fromBelow:method"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.thinning.selectiveThinning"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.thinning.tending"}}{{/crossLink}}
 *
 *  - Helper:
 *      - {{#crossLink "abe-lib.helper.createSTP"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.helper.buildProgram"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.helper.selectOptimalPatches"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.helper.changeSTP"}}{{/crossLink}}
 *      - {{#crossLink "abe-lib.helper.repeater"}}{{/crossLink}}
 *
 * ### Loading and Using the Library
 *  - Include the library in your iLand project using `Globals.include()`.
 *  - Initialize the library using `lib.initStandObj()` or `lib.initAllStands()`.
 *  - Use the provided functions to create and manage STPs.
 *
 * ### Extending the Library
 *  - You can add custom functions and modules to the library.
 *  - Follow the existing coding style and documentation conventions.
 *
 *
 *  For detailed information on specific functions and classes, please refer to the individual module documentation.
 *
 * @author Johannes Mohr, Werner Rammer, EDFM, TU Munich
 * @version 1.0
 * @module abe-lib
 */



function get_js_file_path() {
    // hacky way to get the file name of the currently running script
    try {
        throw new Error("give me the filename!");

    } catch (error) {
        // remove file:/// and remove "qrc" (when loaded from Qt resource files)
        const filename = error.fileName.replace(/^file:\/\/\//, '').replace(/^qrc:\//, ':\/');
        return { fileName: filename,
                 dir: filename.substring(0, filename.lastIndexOf("/")) }
    }
}

var lib = {};
lib.path = get_js_file_path(); //

console.log("Loading forest management library from '" + lib.path.dir + "' ... ");

Globals.include('/' + lib.path.dir + '/thinning/thinning.js'); //'/' + 
Globals.include('/' + lib.path.dir + '/planting/planting.js');
Globals.include('/' + lib.path.dir + '/harvest/harvest.js');

Globals.include('/' + lib.path.dir + '/lib_helper.js');

console.log('Forest management library loaded!');

