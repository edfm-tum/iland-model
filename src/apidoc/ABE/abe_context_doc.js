
/**
  The global variable `fmengine` (of type `FMEngine`) variable lets you access the ABE core engine.

  Use `fmengine` to register agents and stand treatment programmes. `fmengine` also provides some additional functionalities, such as logging (`log`)
  or executing functions/activities in the ABE context.

  @class FMEngine
  */
var fmengine= {
/**
  `log` writes a log message. Each message is prefixed with a code for identifying the current stand and the current year of the simulation.
  The format of the prefix is: 'S_standid_Y_year_:'.

        fmengine.log('log message for stand ' + stand.id);
        // produces (in year 0 and for stand 7)
        abe: "S7Y0:" log message for stand 7

  @method log
  @param {string} message The message to be printed.
  */
    log: function(x){},

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
    abort: function(x),

/**
    `runActivity` executes an {{#crossLink "Activity"}}{{/crossLink}} for stand given by `standId`. This bypasses the normal scheduling (useful for debugging/testing).
    This function calls the main execution function of the activity.

  @method runActivity
  @param {int} standId the (integer) id of the stand in which context the activity should be executed.
  @param {string} activity the name of the activity that should be executed.
  @return {boolean} returns false if the stand or activity were not found.

  */
    runActivity: function(id, activity),

/**
  `runActivityEvaluate` executes an {{#crossLink "Activity"}}{{/crossLink}} for stand given by `standId`. This bypasses the normal scheduling (useful for debugging/testing).
  'runActivityEvaluate' invokes the evaluation code of scheduled activites (e.g., ActThinning, ActScheduled).

  @method runActivity
  @param {int} standId the (integer) id of the stand in which context the activity should be executed.
  @param {string} activity the name of the activity that should be executed.
  @return {boolean} returns false if the stand or activity were not found.

  */
    runActivityEvaluate: function(id, activity),

/**
  adds a management program (STP) whose definition is provided by the Javascript object `program`. The `name` of the program is used internally.

  See also: {{#crossLink "Activity"}}{{/crossLink}}

  @method addManagement
  @param {object} program The javascript object that defines the STP.
  @param {string} name The name that ABE should be use for this STP.
  @return {boolean} true on success.
  */
     addManatgement: function(obj, name),

/**
  add an agent definition (Javascript) and gives the agent the name `name`.

  @method addAgent
  @param {object} program The javascript object that defines the {{#crossLink "Agent"}}{{/crossLink}}.
  @param {string} name The name that ABE should be use for this {{#crossLink "Agent"}}{{/crossLink}}.
  @return {boolean} true on success.
  */
    addAgent: function(obj, name),

/**
  checks if a given `stand_id` is valid (i.e., part of the currently simulated area).

  @method isValidStand
  @param {int} stand_id The id of the stand to check.
  @return {boolean} true, if 'stand_id' is a valid stand in the current setup.
  */
    isValidStand: function(id),

/**
  returns a list of valid stand-ids within the model.
  See also: {{#crossLink "standId:property"}}{{/crossLink}}


  @method standIds
  @return {array} Array of valid stand-ids.
  */
    standIds: function(),

/**
  Runs a planting activity (without the context of stand treatment programmes). This is especially useful for
  setting up initial stand conditions. The `planting` defines the activity according to the syntax of the planting activity.


        // global 'onInit' function is called during startup
        function onInit() {
          // run a planting activity for the stand 235 (30cm spruce trees on 90% of the pixels)
          fmengine.runPlanting( 235, { species: "piab", fraction: 0.9, height: 0.3 });
        }

  @method runPlanting
  @param {int} standId (integer) of the stand for which a planting activity should be executed.
  @param {object} planting A javascript object with the definition of the planting activity (see ABE documentation).
  */
    runPlanting: function(standid, obj),

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
    verbose: false,

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
    standId: -1,
/**
  This function adds quicklinks that trigger Javascript functions to the user interface of iLand. Each name-value pair
  defines one function call (name) and its description (value).


        // call this anywhere (e.g. in the body of the main javascript)
        Globals.setUIShortcuts( { 'Globals.reloadABE()': 'full reload of ABE', 'test()': 'my dearest test function' } );


  @method setUIShortcuts
  @param {object} A object containt name/value pairs that are used in the iLand UI to create quicklinks for calling javascript functions.
  */
    setUIShortcuts: function(obj)

}


/**
* Access to properties of the current stand.
* The `stand` variable is available in the execution context of forest management and provides access to properties and functions
* of the stand that is currently processed.
*
* Note that the variable `stand` is provided through the C++ framework and does not need to be created separately.
*
* Use the 'flag' and 'setFlag' methods to (persistently) modify / read user-specific properties for each stand. This is a means
* to pass stand-specific information between activities (or between different events within one activity).
*
*
@class Stand
*/
var stand = {
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
    trace: false,

/**
  The id of the stand that is currently processed.

  See also: {{#crossLink "FMEngine/standId:property"}}{{/crossLink}} in `fmengine`.

  @property id
  @type integer
  @default -1
*/
    id: -1,

/**
  The basal area / ha stocking on the stand (living trees, >4m).

  @property basalArea
  @type double
*/
    basalArea: 0,


/**
  The total standing timber volume / ha of the stand (living trees, >4m).

  @property volume
  @type double
*/
    volume: 0,

/**
  The mean height of the stand (meter). It is calculated as basal area weighted mean height of all trees on the stand (>4m).
  See also {{#crossLink "Stand/topHeight:property"}}{{/crossLink}}.

  @property height
  @type double
*/
    height: 0,

/**
  The top height (in meters) is defined as the mean height of the 100 thickest trees per ha. For larger/ smaller stands, the number of trees is scaled accordingly.
  See also {{#crossLink "Stand/height:property"}}{{/crossLink}}.

  @property topHeight
  @type double
*/
    topHeight: 0,

/**
  The mean age of the stand (years). It is calculated as basal area weighted mean age of all trees on the stand (>4m).
  Note the difference to `absoluteAge`, which is the number of years since the rotation started.

  See also {{#crossLink "Stand/absoluteAge:property"}}{{/crossLink}}.

  @property age
  @type double
*/
    age: 0,

/**
    The age of the stand given in years since the rotation started. At startup, the `absoluteAge` is estimated from
    the `age` of the stand (i.e. the mean age of the initialized trees). Later, the stand age counter is reset
    by management activities. Note that this property is writable.

    See also {{#crossLink "Stand/age:property"}}{{/crossLink}}.

  @property absoluteAge
  @type double
*/
    absoulteAge: 0,

/**
  The number of different tree species present on the stand (trees >4m). Use to iterate over the available species on the stand:

        // print the species id and the basal area for each available species.
        // note that the species are ordered by the basal area share.
        for (var i=0;i<stand.nspecies;++i)
            log(stand.speciesId(i) + ": " + stand.basalArea(i));

  @property nspecies
  @type int
*/
    nspecies: 0,

/**
  The total area of the stand in hectares.


  @property area
  @type double
*/
    area: 0,
/**
  Retrieve the species id at position `index`.

  @method speciesId
  @param {integer} index The index of the species (valid between 0 and `nspecies`-1).
  @return {string} The unique id of the species.*/
    speciesId: function(index) {},

/**
  Retrieve the basal area of the species at position `index`.

  @method basalArea
  @param {integer} index The index of the species (valid between 0 and `nspecies`-1).
  @return {double} The basal area (m2/ha) of the species.*/
    speciesBasalArea: function(index){},

/**
  Retrieve the basal area of the species with the species code 'speciescode'.
  Note that only trees with height > 4m are included.

  @method basalAreaOf
  @param {string} speciescode The code of the species (e.g., 'piab').
  @return {double} The basal area (m2/ha) of the species, or 0 if the species is not present.*/
    speciesBasalAreaOf: function(name){},

 /**
  Retrieve the relative basal area of the species 'speciescode'.

  @method relBasalAreaOf
  @param {string} speciescode The code of the species (e.g., 'piab').
  @return {double} The basal area (m2/ha) of the species, or 0 if the species is not present.*/
    relSpeciesBasalAreaOf: function(name){},

/**
*  Retrieve the basal area share (0..1) of the species at position `index`.
*
*      // get the share of the dominant species:
*      log( stand.relBasalArea(0) * 100 + "%");
*
*  @method basalAreaRel
*  @param {integer} index The index of the species (valid between 0 and `nspecies`-1).
*  @return {double} The basal area share (0..1) of the species.*/
    relSpeciesBasalArea: function(index){},

/**
  Force a reload of the stand data, i.e. fetch stand statistics (e.g. basal area, age)
  from the trees comprising the stand.

  Usually, this is done automatically by ABE, however, it might be useful in some rare circumstances.

  @method reload

*/
    reload: function(){},

/**
  The ´sleep´ method suspends the activities on the stand for `years` years. Only after the specified has elapsed,
  ABE continues to examine the stand.

  @method sleep
  @param {integer} years The number of years that the stand should sleep.

*/
    reload: sleep(years){},

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
    activity: function(name),

/**
  Retrieves the stand-specific property associated with the name 'name' for the stand of the current execution context.

        stand.setFlag('test', 3); // simple values
        stand.setFlag('my_goal', { s1: 10, s2: 20, s3: function(){return this.s1+this.s2;} } ); // complex objects (including functions are allowed)

        fmengine.log( stand.flag('my_goal').s3 + stand.flag('test') + stand.U  ); // -> 133 (if U=100 of the stand)

@method flag
@param {string} name The (user-defined) property name of the stored parameter.
@return {value} The associated value for the given 'name'. Returns 'undefined' if no value is assigned.*/
    flag: function(name){},

/**
  Sets 'value' as the stand-specific property associated with the name 'name' for the stand of the current execution context.


  @method setFlag
  @param {string} name The (user-defined) property name of the stored parameter.
  @param {value} value The value that should be stored for 'name'. 'value' can be any valid Javascript expression (including objects).
*/
    setFlag: function(name, value){},

/**
  The number of years since the execution of the last activity for the current stand. Value is -1 if no activity was executed previously.

  @property elapsed
  @type int
*/
    elapsed: 0,


/**
  The name of the last previously executed activity, or an empty string if no activity was executed before. The name can be used to access
  properties of the activity.


           if (stand.lastActivity == "thinning1")
                stand.activity( stand.lastActivity ).enabled = true; // re-enable last activity if it was 'thinning1'

  @property lastActivity
  @type string
*/
    lastActivity: '',

/**
  The rotation length of the current stand. The rotation length is defined by the stand treatment programme that is currently assigned to a given
  stand. The 'U' is frequently used for timing activites relative to the length of the period.

  @property U
  @type double
*/
    U: 0
}







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
var activity: {

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
    active: false,

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
    enabled: false,

/**
  This flag indicates whether the {{#crossLink "Scheduler"}}{{/crossLink}} is used when this activity is run, i.e. if the
  exact date of execution is selected by the scheduling algorithm of ABE. The default value depends on the type of the activity,
  e.g., default is false for activities of type __general__, but true for __scheduled__ activities.

  @property scheduled
  @type boolean
  @default depends on activity type.
  */
    scheduled: false,

/**
  An activity with `finalHarvest`=true ends a rotation when (successfully) executed. This resets the `{{#crossLink "Stand/age:property"}}{{/crossLink}}`
  counter and sets all activites `{{#crossLink "Activity/active:property"}}{{/crossLink}}`.

  @property finalHarvest
  @type boolean
  @default depends on activity type.
  */
    finalHarvest: false,


/**
  The name of the activity as provided in the definition of the activity.

  See also: {{#crossLink "FMEngine/addManagement:method"}}fmengine.addManagement{{/crossLink}}

  @property name
  @type string
  @readonly
  */
    name: ''
}




/**
* Access to properties of the current stand treatment program.
* The `stp` variable is available in the execution context of forest management and provides access to properties STP
* activity (linked to a `stand`).
*
* Note that the variable `stp` is provided through the C++ framework and does not need to be created separately.
*
* See also: Variables `stand` ({{#crossLink "Stand"}}{{/crossLink}})

*
@class STP
*/
var stp: {
    /**
      The name of the stand treatment program as provided when called the respecitve 'fmengine' functions.

      See also: {{#crossLink "FMEngine/addManagement:method"}}fmengine.addManagement{{/crossLink}}

      @property name
      @type string
      @readonly
      */
        name: ''
    },
    /**
      * The 'options' property provides a link to the 'options' property of the STP *definition*, i.e., this is a mechanism
      * that can be used to pass purely javascript options/values to other javascript code (e.g., when an activity within the
      * stand context is executed). Note that the C++ part of ABE does not interfere with these options.

                // definition of the STP
                var stp: { U: [110, 130, 150], // define the rotation period
                    clearcut: a_clearcut, // some activities
                    options: { dbh: 5, someOtherParameter: 50}
                }

                // definition of activities… here the ‘dbh’ options is used
                // in the Javascript code to select trees to harvest.
                var a_clearcut: {
                   type: "scheduled", … ,
                   onEvaluate: function() {
                            trees.loadAll();
                            trees.harvest("dbh>" + stp.options.dbh );}
                         }
                };



      @property options
      @type object
        */
        options: {}


}
