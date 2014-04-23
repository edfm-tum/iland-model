// Javascript API documentation framework http://yui.github.io/yuidoc/
// create with "yuidoc ." in /apidoc directory (/build/apidocs must be available!) or with "yuidoc --server ." for interactive mode.
// see also the yuidoc.json for more options.
// http://127.0.0.1:3000/

/**
 * The agent based forest management engine.
#Overview
One example, the cross is cross



this is the ....
 *
 * @module ABE
*/

/**
  The `fmengine` variable lets you access the ABE core engine.

  Use `fmengine` to register agents and stand treatment programmes. `fmengine` also provides some additional functionalities, such as logging (`log`)
  or executing functions/activities in the ABE context.

  @class fmengine
  */

/**
  `log` writes a log message. Each message is prefixed with a code for identifying the current stand and the current year of the simulation.
  The format of the prefix is: 'S_standid_Y_year_:'.

           fmengine.log('log message for stand ' + stand.id);
           // produces (in year 0 and for stand 7)
           abe: "S7Y0:" log message for stand 7

  @method log
  @param {string} message The message to be printed.
  */

/**
    `runActivity` executes an {{#crossLink "Activity"}}{{/crossLink}} for stand given by `standId`. This bypasses the normal scheduling (useful for debugging/testing).

  @method runActivity
  @param {int} standId the (integer) id of the stand in which context the activity should be executed.
  @param {string} activity the name of the activity that should be executed.
  @return {boolean} returns false if the stand or activity were not found.

  */

/**
  adds a management program (STP) that is provided as the Javascript object 'program'. 'name' is used internally.

  @method addManagement
  @param {object} program The javascript object that defines the @STP.
  @param {string} name The name that ABE should be use for this STP.
  @return {boolean} true on success.
  */

/**
  add an agent definition (Javascript) and gives the agent the name `name`.

  @method addAgent
  @param {object} program The javascript object that defines the {{#crossLink "Agent"}}{{/crossLink}}.
  @param {string} name The name that ABE should be use for this {{#crossLink "Agent"}}{{/crossLink}}.
  @return {boolean} true on success.
  */

/**
  ABE generates much more detailed log messages, if `verbose` is true. This is generally useful for debugging and
  should be turned off for productive use. Note, that `verbose` mode can switched on for specfic code sections:

        ....
        fmengine.verbose = true;
        ... // do some complicated stuff
        fmengine.verbose = false; // switch off verbose mode

  See also: {{#crossLink "stand:trace"}}{{/crossLink}}

  @property verbose
  @type boolean
  @default false
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

