// Javascript API documentation framework http://yui.github.io/yuidoc/
// create with "yuidoc ." in /apidoc directory (/build/apidocs must be available!) or with "yuidoc --server ." for interactive mode.
// see also the yuidoc.json for more options.
// http://127.0.0.1:3000/

/**
 * The Agent Based forest management Engine API
 Overview
 ========

 *
 * @module ABE
 */

/**
* This is the description for my class.
Overview
========
This is nice *markdown* formatting.
 - point 1
 - point 2
*
* @class Activity
*/
/**
* My property description.  Like other pieces of your comment blocks,
* this can span multiple lines.
* @property schedule
* @type {Schedule}
* @default ""
*/

/**
 * Fired when an error occurs...
 *
 * @event onEnter
 * @param {String} msg A description of...
 */

/**
* My property description.  Like other pieces of your comment blocks,
* this can span multiple lines.
* @property constraint
* @type {Constraint}
* @default ""
*/
/**
* My method description.  Like other pieces of your comment blocks,
* this can span multiple lines.
*
* @method methodName
* @param {String} foo Argument 1
* @param {Schedule} config A schedule object
* @param {String} config.name The name on the config object
* @param {Function} config.callback A callback function on the config object
* @param {Boolean} [extra=false] Do extra, optional work
* @return {Boolean} Returns true on success
*/

/**
* Schedule object.
*
* @class Schedule
*/

/**
* My property description.  Like other pieces of your comment blocks,
* this can span multiple lines.
* @property min
* @type {double}
* @default ""
 * @example
 *	// set 'foo' and 'bar'
 *     model.set('foo', 'bar');
 */

/**
* Constraints object.
*
* @class Constraint
*/

/**
* Definition of an Agent.
* The main part of the agent defintion is a list of assignments of stand treatment programmes (STP) to mixture types.
* In addition, logic can be added for selecting mixture types from forest attributes.
*
*

        var x = { stp: { "fasy": ["beech_default_1", "beech_default_2", "beech_default3"],
                         "piab": "spruce_default" }
                  onSelect: function(){ return "fasy"; }
                 }
*
@class Agent
*/
/** Description of stp
@property stp
@type object
*/
/**
  onSelect allows to provide custom code to select the mixture type for a given stand.
  @example
       onEvent: function() { if (stand.share('piab') > 50)
                                return 'piab';
                            return 'default'; }
  @event onSelect
  @type String
  */

