/**
 * The ABE-library

Overview
========
The ABE-library:
+ is a pure JavaScript on top of the iLand ABE system
+ simplifies the use of ABE by providing a set of pre-defined building blocks to construct forest management
+ each building block can be further customized, and "standard" ABE / JS activities can be used as well

High-level description.....

Loading and using the library
-----------------------------

Extending the libray
--------------------



 *
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

Globals.include(lib.path.dir + '/thinning/thinning.js');
Globals.include(lib.path.dir + '/planting/planting.js');
Globals.include(lib.path.dir + '/harvest/harvest.js');

Globals.include(lib.path.dir + '/lib_helper.js');

console.log('Forest management library loaded!');

