/**
* The `patches` object provides functions to create and manage spatial sub-divisions (patches) within a forest stand.
* Patches are defined on a 10x10m grid. They can be used for spatially explicit forest management.
*
* Use the `patches` property of the (current) `stand` variable to access the functionality.
*
*      // create 2 random patches
*      stand.patches.createRandomPatches(2);
*      // get the list of patches
*      var patch_list = stand.patches.list;
*      // get the light influence field for the first patch
*      log("LIF of first patch: " + stand.patches.lif(patch_list[0]));
*
*
@class Patches
*/
var patches = {

/**
 * A list containing all {{#crossLink "Patch"}}{{/crossLink}} objects of the current stand.
 *
 * @property list
 * @type Array
 */
list: [],

/**
 * The bounding rectangle of the stand in meters. This is a read-only property.
 * The rectangle object has `x`, `y`, `width`, and `height` properties.
 *
 * @property rectangle
 * @type Object
 */
rectangle: {},

/**
 * Creates a new patch by extending an existing patch.
 * The new patch surrounds the existing patch with the given `patchId`.
 *
 * @method createExtendedPatch
 * @param {integer} patchId The ID of the existing patch to extend.
 * @param {integer} newPatchId The ID for the new patch.
 * @param {integer} [grow_by=1] The distance (in 10m cells) to extend the patch by.
 * @return {integer} The number of cells added to the new patch.
 */
createExtendedPatch: function(patchId, newPatchId, grow_by) {},

/**
 * Calculates the average value of the Light Influence Field (LIF) at 4m height for a given patch.
 *
 * @method lif
 * @param {Patch} patch The patch object for which to calculate the average LIF.
 * @return {double} The average LIF value for the patch.
 */
lif: function(patch) {},

/**
 * Creates a given number of random patches within the stand.
 * The patches are non-overlapping and cover the entire stand area.
 *
 * @method createRandomPatches
 * @param {integer} n The number of random patches to create.
 */
createRandomPatches: function(n) {},

/**
 * Removes all existing patches from the stand.
 *
 * @method clear
 */
clear: function() {},

/**
 * Creates a single patch with a specific shape at a given location.
 *
 * @method createPatch
 * @param {double} x The x-coordinate (in meters) of the center of the shape.
 * @param {double} y The y-coordinate (in meters) of the center of the shape.
 * @param {string} shape_string A string defining the shape, e.g., "circle(15)" for a circle with a 15m radius, or "rect(20,30)" for a 20x30m rectangle.
 * @param {integer} [id=-1] The ID for the new patch. If -1, a new unique ID is generated.
 * @return {boolean} `true` if the patch was created successfully, `false` otherwise.
 */
createPatch: function(x, y, shape_string, id) {},

/**
 * Creates patches in the form of horizontal or vertical strips.
 *
 * @method createStrips
 * @param {double} width The width of the strips in meters.
 * @param {boolean} horizontal If `true`, horizontal strips are created, otherwise vertical strips.
 * @return {Array} A list of the created {{#crossLink "Patch"}}{{/crossLink}} objects.
 */
createStrips: function(width, horizontal) {},

/**
 * Creates a regular pattern of patches (e.g., squares).
 *
 * @method createRegular
 * @param {integer} size The size of each patch in meters.
 * @param {integer} spacing The spacing between patches in meters.
 * @return {Array} A list of the created {{#crossLink "Patch"}}{{/crossLink}} objects.
 */
createRegular: function(size, spacing) {},

/**
 * Creates patches from a raster grid. The grid is provided as a {{#crossLink "ScriptGrid"}}{{/crossLink}} object.
 * For each unique value in the grid, a corresponding patch is created.
 *
 * @method createFromGrid
 * @param {ScriptGrid} grid The input grid object.
 * @return {Array} A list of the created {{#crossLink "Patch"}}{{/crossLink}} objects.
 */
createFromGrid: function(grid) {},

/**
 * This method is usually called automatically. It re-creates the internal 10x10m grid representation
 * of the patches from the list of patch objects. You might need to call this if you have manipulated
 * the `list` of patches manually.
 *
 * @method updateGrid
 */
updateGrid: function() {}

};