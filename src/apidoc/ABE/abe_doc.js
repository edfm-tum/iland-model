// Javascript API documentation framework http://yui.github.io/yuidoc/
// create with "yuidoc ." in /apidoc directory (/build/apidocs must be available!) or with "yuidoc --server ." for interactive mode.
// see also the yuidoc.json for more options.
// http://127.0.0.1:3000/

// Installation of YUIDOC: http://yui.github.io/yuidoc/
// you need node.js -> download and install

/**
 * The agent based forest management engine.
#Overview
One example, the cross is cross



this is the ....
 *
 * @module ABE
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



