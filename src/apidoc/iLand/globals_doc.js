/**
 * iLand Javascript API
Overview
========
this is the ....
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
* My property description.  Like other pieces of your comment blocks,
* this can span multiple lines.
* @property year
* @default {int} 2000
* @readOnly
*/
year: 2000,
/**
@property resourceUnitCount
@type integer
@default 1
@readOnly */
resourceUnitCount: 1,

/**
Sets the current viewing rectangle of the iLand program programmatically.
After the call, the screen centers on the (world coordinates) x and y. The scale is defined as pixels per meter,
e.g. 1 value of 0.5 would mean that each screen pixel are two meters in the real world.
 @method setViewport
@param {double} x center point of the viewport in x-direction (m)
@param {double} y center point of the viewport in y-direction (m)
@param {double} scale pixel (screen) per metre (simulated landscape)
      */
    function setViewport(x,y,scale) {}

}
