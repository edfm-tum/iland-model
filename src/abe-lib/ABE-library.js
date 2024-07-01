/**
  Main include file for the ABE management library.

To use the library: ...

  */
// ABE-library.js

function get_js_file_path() {
    // hacky way to get the file name of the currently running script
    try {
        throw new Error("give me the filename!");

    } catch (error) {
        const filename = error.fileName.replace(/^file:\/\/\//, ''); // remove file:///
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

