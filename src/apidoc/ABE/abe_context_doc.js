
/**
  The global variable `fmengine` (of type `FMEngine`) variable lets you access the ABE core engine.

  Use `fmengine` to register agents and stand treatment programmes. `fmengine` also provides some additional functionalities, such as logging (`log`)
  or executing functions/activities in the ABE context.

  @class FMEngine
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
  Calling `abort` stops the execution of ABE and prints the error message `message` to the console (and shows the error). Note that `abort` does not
  interrupt execution of JavaScript code immediately. However, the ABE core engine cancels all further activities, when the `abort` method has been called.


            if (some_error_condition) {
                fmengine.abort('This is really not expected!!');
                return; // required; otherwise do_some_stuff() would be executed.
            }
            do_some_stuff();


  @method abort
  @param {string} message The error message.
  */

/**
    `runActivity` executes an {{#crossLink "Activity"}}{{/crossLink}} for stand given by `standId`. This bypasses the normal scheduling (useful for debugging/testing).

  @method runActivity
  @param {int} standId the (integer) id of the stand in which context the activity should be executed.
  @param {string} activity the name of the activity that should be executed.
  @return {boolean} returns false if the stand or activity were not found.

  */

/**
  adds a management program (STP) whose definition is provided by the Javascript object `program`. The `name` of the program is used internally.

  See also: {{#crossLink "Activity"}}{{/crossLink}}

  @method addManagement
  @param {object} program The javascript object that defines the STP.
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

  See also: {{#crossLink "Stand/trace:property"}}{{/crossLink}}

  @property verbose
  @type boolean
  @default false
  */

/**
  `standId` is the numeric Id of the stand that is set as the current execution context, i.e. the `stand`, `site`, etc. variables
  that are available in the current context refer to the stand with this Id. The property is set automatically by ABE during the
  execution of forest management. It can, however, be changed by Javascript for debugging/testing purposes. Note that changing
  the `standId` might lead to unexpected behavior.


  See also: {{#crossLink "Stand/id:property"}}{{/crossLink}}

  @property standId
  @type int
  @default -1
  */



/**
* Access to properties of the current stand.
* The `stand` variable is available in the execution context of forest management and provides access to properties and functions
* of the stand that is currently processed.
*
* Note that the variable `stand` is provided through the C++ framework and does not need to be created separately.

*
@class Stand
*/

/**
  If `trace` is set to true, detailed log information is produced by ABE. This is useful for testing/ debugging.
  The trace-mode can be switched on/ off like this:

        // enable trace for stand 7
        fmengine.standId = 7; // set the current stand to the stand with Id 7
        stand.trace = true; // enable trace


  See also: {{#crossLink "FMEngine/verbose:property"}}{{/crossLink}}

  @property trace
  @type boolean
  @default false
  */

/**
  The id of the stand that is currently processed.

  See also: {{#crossLink "FMEngine/standId:property"}}{{/crossLink}} in `fmengine`.

  @property id
  @type integer
  @default -1
*/

/**
  The basal area / ha stocking on the stand (living trees, >4m).

  @property basalArea
  @type double
*/


/**
  The total standing timber volume / ha of the stand (living trees, >4m).

  @property volume
  @type double
*/

/**
  The mean age of the stand (years). It is calculated as basal area weighted mean age of all trees on the stand (>4m).
  Note the difference to `absoluteAge`, which is the number of years since the rotation started.

  See also {{#crossLink "Stand/absoluteAge:property"}}{{/crossLink}}.

  @property age
  @type double
*/

/**
    The age of the stand given in years since the rotation started. At startup, the `absoluteAge` is estimated from
    the `age` of the stand (i.e. the mean age of the initialized trees). Later, the stand age counter is reset
    by management activities.

    See also {{#crossLink "Stand/age:property"}}{{/crossLink}}.

  @property absoluteAge
  @type double
*/

/**
  The number of different tree species present on the stand (trees >4m). Use to iterate over the available species on the stand:

        // print the species id and the basal area for each available species.
        // note that the species are ordered by the basal area share.
        for (var i=0;i<stand.nspecies;++i)
            log(stand.speciesId(i) + ": " + stand.basalArea(i));

  @property nspecies
  @type int
*/

/**
  Retrieve the species id at position `index`.

  @method speciesId
  @param {integer} index The index of the species (valid between 0 and `nspecies`-1).
  @return {string} The unique id of the species.*/

/**
  Retrieve the basal area of the species at position `index`.

  @method basalArea
  @param {integer} index The index of the species (valid between 0 and `nspecies`-1).
  @return {double} The basal area (m2/ha) of the species.*/

/**
  Retrieve the basal area share (0..1) of the species at position `index`.

        // get the share of the dominant species:
        log( stand.relBasalArea(0) * 100 + "%");

  @method basalAreaRel
  @param {integer} index The index of the species (valid between 0 and `nspecies`-1).
  @return {double} The basal area share (0..1) of the species.*/

/**
  Force a reload of the stand data, i.e. fetch stand statistics (e.g. basal area, age)
  from the trees comprising the stand.

  Usually, this is done automatically by ABE, however, it might be useful in some rare circumstances.

  @method reload

*/

/**
  Use `activity` to retrieve an {{#crossLink "Activity"}}{{/crossLink}} object.

  Note: the global variable `activity` is a "short-cut" to access the currently active activity.

        stand.activity("my_thinning_2").enabled = false; // disable an activity
        var act = stand.activity("my_thinning_1"); // save a reference to the activity for later use

  See also: the global variable `{{#crossLink "Activity"}}activity{{/crossLink}}`

  @method activity
  @param {string} activity_name The name of the activity to be retrieved. Activity names are provided during activity definition (see {{#crossLink "FMEngine/addManagement:method"}}fmengine.addManagement{{/crossLink}})
  @return {Activity} the Activity, or `undefined` if not found.
*/














/**
* Access to properties of the current activity.
* The `activity` variable is available in the execution context of forest management and provides access to properties and functions
* of the current activity (linked to a `stand`) that is currently processed. In addition, other activities of the (currently active) programme
* can be accessed with {{#crossLink "Stand/activity:method"}}{{/crossLink}}.
*
* Note that the variable `activity` is provided through the C++ framework and does not need to be created separately.
*
* See also: Variables `stand` ({{#crossLink "Stand"}}{{/crossLink}})

*
@class Activity
*/


/**
  The `active` property indicates whether an activity __can__ still be executed during this rotation. After an activity is executed,
  the `active` flag is set to false, and the next active (and `enabled`) activity is selected. At the end of a rotation, the `active`
  flag of all activities of the programme are set back to true. By changing the value of `active`, activities can be either removed
  from the current rotation, or re-enabled.

        //re-enable the 'todo' activity.
        stand.activity("todo").active = true;


  See also: {{#crossLink "Activity/enabled:property"}}{{/crossLink}}

  @property active
  @type boolean
  @default false
  */

/**
  An activity can only be executed, if the `enabled` property is true. Activities can be switched on/ off using this flag.

  Note that an activity needs also to be `{{#crossLink "Activity/active:property"}}{{/crossLink}}` in order to get executed.

        // switch off a repeating activity, when a height threshold is surpassed.
        if (stand.hmean>12)
            stand.activity('repeater').enabled = false;


  See also: {{#crossLink "Activity/active:property"}}{{/crossLink}}

  @property enabled
  @type boolean
  @default false
  */

/**
  This flag indicates whether the {{#crossLink "Scheduler"}}{{/crossLink}} is used when this activity is run, i.e. if the
  exact date of execution is selected by the scheduling algorithm of ABE. The default value depends on the type of the activity,
  e.g., default is false for activities of type __general__, but true for __scheduled__ activities.

  @property scheduled
  @type boolean
  @default depends on activity type.
  */

/**
  An activity with `finalHarvest`=true ends a rotation when (successfully) executed. This resets the `{{#crossLink "Stand/age:property"}}{{/crossLink}}`
  counter and sets all activites `{{#crossLink "Activity/active:property"}}{{/crossLink}}`.

  @property finalHarvest
  @type boolean
  @default depends on activity type.
  */


/**
  The name of the activity as provided in the definition of the activity.

  See also: {{#crossLink "FMEngine/addManagement:method"}}fmengine.addManagement{{/crossLink}}

  @property name
  @type string
  @readonly
  */
