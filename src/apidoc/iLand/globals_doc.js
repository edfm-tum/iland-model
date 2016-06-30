/**
 * iLand Javascript API

Overview
========

iLand - the invidiual based landscape and disturbance model - is capable of simulating the development of forests on landscapes
of thousands of hectares. The model is built in C++ (using the [Qt-framework](http://qt.io) and utilizes the the built-in
V8 Javascript engine. The model exposes a number of (C++)-objects to the Javascript context, that allow accessing various iLand functions from Javascript.

See [iLand scripting](http://iland.boku.ac.at/iLand+scripting) for an overview.


 *
 * @module iLand
*/



/**
The Globals object exposes a series of auxiliary functions to the scripting context. An instance of "Globals" is always available in the global context, so no explicit creation is necessary.
The provided function cover inter alia:

- handling of directories and simple text files
- access to XML settings
- access and control over Outputs
- miscellaneous functions (e.g., for making screeshots)

 @class Globals
 */
globals = {
/**
The current year of the simulation.
@property year
@type {integer}
@readOnly
*/

/**
The number of resource units in the current simulation.
@property resourceUnitCount
@type integer
@default 1
@readOnly */


/**
Sets the current viewing rectangle of the iLand program programmatically.
After the call, the screen centers on the (world coordinates) x and y. The scale is defined as pixels per meter,
e.g. 1 value of 0.5 would mean that each screen pixel are two meters in the real world.

@method setViewport
@param {double} x center point of the viewport in x-direction (m)
@param {double} y center point of the viewport in y-direction (m)
@param {double} scale pixel (screen) per metre (simulated landscape)
*/


/**
Completely reloads the ABE (agent based management engine) sub module. This includes loading of the (static) stand description file,
and the javascript source code of ABE.
@method reloadABE
*/

/**
Read a value from the [project file](http://iland.boku.ac.at/project+file). The `key` is the full path to the
requested node using a '.'-notation.

See also: {{#crossLink "Globals/set:method"}}{{/crossLink}}

@Example
    // global 'onInit' function is called during startup
    var width = Globals.setting('model.world.width'); // the horizontal extent of the project area
    var filename = Globals.setting('user.targetdir') + "my_file.txt"; // a user-defined key in the project file

@method setting
@param {string} key fully qualified key within the project file
@return The value of the setting, or _undefined_ if the setting is not present
*/


/**
Set a setting in the [project file](http://iland.boku.ac.at/project+file) with the key `key` to a value of `value`.
The `key` is the full path to the requested node using a '.'-notation.

See also: {{#crossLink "Globals/setting:method"}}{{/crossLink}}

@method set
@param {string} key fully qualified key within the project file
@param  value new value of the setting `key`.
@return {boolean} _true_ on success.
*/


/**
Get directory with of the given category `category`. See [filenames and paths](http://iland.boku.ac.at/filenames+and+paths) for available categories.
Using this defaultDirectory() avoids absolute file paths in scripts.

See also: {{#crossLink "Globals/currentDir:property"}}{{/crossLink}}

@method defaultDirectory
@param category {string} folder category
@return {string} The requested file path (without a trailing slash)
  */


/**
Use `path()` to construct a path relative to the project directory. Useful for avoiding absolute file paths in Javascript.

Example (Project-file: "c:\iland\test\test.xml"): Globals.path('temp/filexy.txt') -> "c:\iland\test\temp.filexy.txt"

See also: `defaultDirectory`, `currentDirectory`

@method path
@param filename {string} filename that should be extended
@return {string} fully qualified file path
*/


/**
gets or sets the current directory of iLand. Relative paths are resolved relative to the `currentDir`.

Example (Project-file: "c:\iland\test\test.xml"): Globals.path('temp/filexy.txt') -> "c:\iland\test\temp.filexy.txt"

See also: {{#crossLink "Globals/path:method"}}{{/crossLink}}
@property currentDir
@type {string}
@Example
    Globals.currentDir="c:/temp";
    Globals.loadTextFile("i_am_in_temp.txt"); // accesses c:/temp
*/

/**
extent of the world (without buffer) in meters (x-direction).

See also: {{#crossLink "Globals/worldY:property"}}{{/crossLink}}
@property worldX
@type {integer}
*/

/**
extent of the world (without buffer) in meters (y-direction).

See also: {{#crossLink "Globals/worldX:property"}}{{/crossLink}}
@property worldY
@type {integer}
*/

/**
starts the output `table_name`. The table name for each output can be found on the [output](http://iland.boku.ac.at/output) wiki page.
Starting [debug outputs](http://iland.boku.ac.at/debug+outputs) is also possible - the `table_name` has to have the format _debug_XXX_, with _XXX_ one of the following:

+ treeNPP
+ treePartition
+ treeGrowth
+ waterCycle
+ dailyResponse
+ establishment
+ carbonCycle
+ performance

See also: {{#crossLink "Globals/stopOutput:method"}}{{/crossLink}}

@method startOutput
@param table_name {string} Output that should be started
@return {boolean} true on succes, an error message is printed in case of failure.
@Example
    Globals.startOutput("debug_performance");
*/
/**
stops the output `table_name`. The table name for each output can be found on the [output](http://iland.boku.ac.at/output) wiki page.
Starting (debug outputs) is also possible - see {{#crossLink "Globals/startOutput:method"}}{{/crossLink}}.

See also: {{#crossLink "Globals/startOutput:method"}}{{/crossLink}}

@method stopOutput
@param table_name {string} Output that should be stopped
@return {boolean} true on succes, an error message is printed in case of failure.
*/


/**
creates a snapshot from the current state of the model. `file_name` is the path to the target database, which is created if the database file does not exist
(paths relative to the _home_ directory). The [wiki](http://iland.boku.ac.at/initialize+trees) provides details about snapshots.

See also: {{#crossLink "Globals/loadModelSnapshot:method"}}{{/crossLink}}

@method saveModelSnapshot
@param file_name {string} filename of the output database.
@return {boolean} true on succes.
*/

    /**
    loads a snapshot database (created with a previous call to {{#crossLink "Globals/saveModelSnapshot:method"}}{{/crossLink}}).
    The model must be already created (i.e. resource units, ...);
    exisiting trees are removed and replaced by the trees from the database.

    See also: {{#crossLink "Globals/saveModelSnapshot:method"}}{{/crossLink}}

    @method loadModelSnapshot
    @param file_name {string} filename of the input database.
    @return {boolean} true on succes.
    */


}
