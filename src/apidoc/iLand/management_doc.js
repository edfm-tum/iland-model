/**
The `Management` object is used to for operations that remove trees from the simulation environment.
The iLand management is accessible using the global object `management`, which is available
when script code is invoked from the iLand management subsystem.

Main functions
--------------

+ Loading trees from iLand: use {{#crossLink "Management/load:method"}}{{/crossLink}} and {{#crossLink "Management/loadFromMap:method"}}{{/crossLink}} to load a list of trees
+ manipulating lists of trees: use {{#crossLink "Management/filter:method"}}{{/crossLink}} to select a sub set based on some criterion,
 or {{#crossLink "Management/sort:method"}}{{/crossLink}} to bring the list in a specific order; you can calculate {{#crossLink "Management/sum:method"}}{{/crossLink}}
 or {{#crossLink "Management/mean:method"}}{{/crossLink}} values from all trees in the list
+ removing trees: {{#crossLink "Management/manage:method"}}{{/crossLink}} and {{#crossLink "Management/kill:method"}}{{/crossLink}}
+ other functions such as....

Note that by default only living trees are processed.
Several special functions exists, e.g. a function to filter based on a predefined list of tree-ids (filter()), of methods
that select trees based on some rastered GIS data.

There are several distinct tree removal modes in iLand, namely by harvesting a tree, or by killing a tree, or due to a disturbance.
In general, harvesting removes all (aboveground) biomass from the system, while a killed trees' biomass is moved to the 'standing dead' pool.
Disturbed trees behave similar to killed trees. Note, that there are further subtle differences between removal modes: for instance, resprouting
is only possible from harvested trees; or, trees killed by disturbance might be treated differently by management.

The management functions typically affect only individual trees in iLand with a height >4m. However, there are some functions available
to alter the state of the saplings (tree cohorts < 4m height).



Expressions and tree variables
------------------------------
Many function of the `management` object allow to specify a filter ([Expression](http://iland.boku.ac.at/Expression)). In the context of
management, tree variables can be used within filter expressions: see http://iland.boku.ac.at/tree+variables

Tree species can be included in Expressions by using the short name as is; internally, the species (identity) is
a integer index of the species, and species short names (such as 'piab', 'fasy', or 'pico') are used as placeholders.


Examples
--------
The wiki contains a small collection of useful management scripts.

@module iLand
@class Management
*/

/**
The number of trees that are currently in the internal list.

See also: {{#crossLink "SpatialAnalysis/load:method"}}{{/crossLink}}

@property count
@type integer
*/
/**
The removal fraction for foliage [0..1]. 0: 0% will be removed, 1: 100% will be removed from the forest by management operations (i.e. calls to manage())

The default value is 0 (all foliage remains in the forest)

See also: {{#crossLink "SpatialAnalysis/removeBranch:method"}}{{/crossLink}}, {{#crossLink "SpatialAnalysis/removeStem:method"}}{{/crossLink}}

@property removeFoliage
@type double
@Example
    // full tree extraction
    management.removeFoliage = 1; management.removeBranch=1; management.removeStem = 1;
    management.manageAll();

*/
/**
The removal fraction for branches [0..1]. 0: 0% will be removed, 1: 100% will be removed from the forest by management operations (i.e. calls to manage())

The default value is 0 (all branches remain in the forest)

See also: {{#crossLink "SpatialAnalysis/removeFoliage:method"}}{{/crossLink}}, {{#crossLink "SpatialAnalysis/removeStem:method"}}{{/crossLink}}

@property removeBranch
@type double
*/
/**
The removal fraction for the stem biomass [0..1]. 0: 0% will be removed, 1: 100% will be removed from the forest by management operations (i.e. calls to manage())

The default value is 1 (100% of the stem is removed in case of management). Note that root biomass is never extracted.

See also: {{#crossLink "SpatialAnalysis/removeFoliage:method"}}{{/crossLink}}, {{#crossLink "SpatialAnalysis/removeBranch:method"}}{{/crossLink}}

@property removeStem
@type double
*/

/**
Load all trees into the internal list and return the number of trees.

@method loadAll
@return {integer} the number of trees that were loaded.
*/
/**
Load all trees passing the filter criterion specified in `filter` and return number of trees in the list.
`filter` can be any tree-related Expression.

@method load
@param {string} filter A valid filter Expression that is applied during the loading of the list.
@return {integer} the number of trees that were loaded.
@Example
    // load all trees with a dbh >30 cm
    management.load('dbh>30');
*/
/**
Load all trees of a resource unit with the index `ruindex` and return the number of loaded trees.

@method loadResourceUnit
@param {int} ruindex The index (0-based) of the resource unit that should be loaded
@return {integer} the number of trees that were loaded.
@Example
    for (var i=0; i<Globals.resourceUnitCount; ++) {
        management.loadResourceIndex(i);
        // further processing....
    }
*/

/**
Load all trees that are located on grid pixels with the value `standID` on the grid `map`.

@method loadFromMap
@param {Map} map a GIS grid that defines stand IDs.
@param {integer} standID the ID of the stand that should be loaded.
@return {integer} the number of trees that were loaded.
@Example
    // Access to the global stand grid (required only once)
    var stand_grid = Factory.newMap();
    // load all trees of the forest stand with ID=1
    management.loadFromMap(stand_grid, 1);
*/

/**
Sort the trees in the internal list in ascending order according to a criterion given
by `expression` (a valid [iLand Expression](http://iland.boku.ac.at/Expression)).

See also: {{#crossLink "Management/percentile:method"}}{{/crossLink}}

@method sort
@param {string} expression Expression used for sorting
@Example
    management.sort('dbh'); // sort by diameter, largest tree are now in the end of the list
    management.sort('-dbh'); // to sort in descending order, reverse the sign of the expression
*/
/**
Apply a filter `expression` on the list of trees (`expression`is a valid [iLand Expression](http://iland.boku.ac.at/Expression))
and return the number of trees remaining in the lists. After calling this function, the list of
trees is typically reduced and contains only those trees, who meet the condition in `expression`.

See the example how to filter by species, and how to combine multiple criteria.

See also: {{#crossLink "Management/sort:method"}}{{/crossLink}}, {{#crossLink "Management/load:method"}}{{/crossLink}}

@method filter
@param {string} expression Expression used for filtering
@return {integer} the number of trees in the list after filtering.
@Example
    management.loadAll();
    management.filter('dbh>10');
    // is the same as:
    management.load('dbh>10');
    // using tree species names: note, that no "'" or '"" signs
    // are used with species IDs; note also the boolean 'and' operator
    management.filter('species=piab and dbh>20');
    management.loadAll(); // all trees
    management.filter('dbh>10 and species=piab'); // reduce list
    // ... some processing....
    management.filter('dbh>20'); // now only spruce trees > 20cm are still in the list
    management.filter('stress>0'); // now only spruce trees >20cm, that have a non-zero stress rating are in the list
    // ...

*/
/**
Apply a filter in form of a `list` of ids, return number of remaining trees. This can be useful
to pre-define a management on individual trees.


See the example below.


See also: {{#crossLink "Management/filter:method"}}{{/crossLink}}, http://iland.boku.ac.at/initialize+trees

@method filterIdList
@param {array} list A list of unique tree IDs.
@return {integer} the number of trees in the list after filtering.
@Example
    // array of tree IDs
    var treelist = [10,20,40,43]; // can be loaded form file...
    management.load(); // load all trees
    management.filter(treelist); // filter trees using the tree list
    management.kill(); // remove all trees remaining, i.e. the trees in the tree list.

*/
/**
Shuffle all trees in the list randomly.

@method randomize
*/

/**
Retrieve the value associated with the `pct` percentile [0..100] of the currently loaded trees. A call
to {{#crossLink "Management/sort:method"}}{{/crossLink}}() is required in order to prepare valid
data.

See also: {{#crossLink "Management/sort:method"}}{{/crossLink}}

@method percentile
@param {integer} pct the percentile [0..100] for which to return the value
@return {dobule} the value associated with the `pct` percentile
*/

/**
Calculate the mean value for all trees in the internal list for `expression` (optionally filtered by the `filter` criterion).

See also: {{#crossLink "Management/sum:method"}}{{/crossLink}}

@method mean
@param {string} expression The expression used for calculating the mean value
@param {string} filter If not empty, the mean is calculated only from those trees that meet the criterion in the expression
@return {dobule} the mean value of `expression`
@Example
    var mean_dbh = management.mean('dbh');

*/
/**
Calculate the sum of `expression` for all trees in the internal list (optionally filtered by the `filter` criterion).

See also: {{#crossLink "Management/sum:method"}}{{/crossLink}}

@method sum
@param {string} expression The expression used for calculating the mean value
@param {string} filter If not empty, the mean is calculated only from those trees that meet the criterion in the expression
@return {dobule} the mean value of `expression`
@Example
    // select trees that represent 50% of the total basal area
    management.loadAll();
    var total_ba = management.sum('basalarea');
    management.randomize();
    management.filter('incsum(basalarea)<' + total_ba * 0.5 );
    console.log(management.sum('basalarea'));
*/


/**
Use `killPct` to remove `n` trees sampled randomly from a given percentile range (given by `from` and `to`). All
trees are removed, if `n` is higher than the number of trees in that range.

The tree list needs to be sorted, before percentile based selection makes sense.

See also: {{#crossLink "Management/sort:method"}}{{/crossLink}}, {{#crossLink "Management/percentile:method"}}{{/crossLink}}

@method killPct
@param {integer} from lower percentile of the current tree distribution (0..100)
@param {integer} to higher percentile of the current tree distribution (0..100)
@param {integer} n the number of trees to kill in the given percentile
@return {integer} the number of killed trees
@Example
    var n = management.loadAll();
    management.sort('dbh');
    var n_killed = management.killPct(0,50, n*0.25);
    console.log('killed ' + n_killed + ' below median: ' + management.percentile(50) );
    // kill 33% of the trees between the 80th and the 100th percentile.
    management.killPct(80,100, n*0.2 * 0.33);
*/
/**
Kill all trees which are currently in the tree list.

See also: {{#crossLink "Management/kill:method"}}{{/crossLink}}, {{#crossLink "Management/manageAll:method"}}{{/crossLink}}
@method killAll
@return {integer} the number of killed trees.
*/
/**
Kill all and cut down all trees in the list. The biomass of cut down trees bypasses the standing dead wood pool, and
such trees can act as trap trees with regard to bark beetles.

See also: {{#crossLink "Management/kill:method"}}{{/crossLink}}, {{#crossLink "Management/manageAll:method"}}{{/crossLink}}
@method cutAndDrop
*/
/**
Remove all selected trees with with the _disturbance_ flag set. Disturbed trees are treated
differently with regard to carbon flows and {{#crossLinkModule "ABE"}}{{/crossLinkModule}}. Biomass of the stem and
the branches is routed to the soil and snag pools as indicated by the parameters of the function. The rest of the biomass
is removed from the system (e.g., consumed by fire). For example, if `stem_to_soil_fraction`=0.2, `stem_to_snag_fraction`=0.3, then
50% of the biomass leaves the system, 20% are added to the soil, 30% to the snag pools.

If the `agent` parameter is `fire`, then serotinous trees produce extra seeds.

See also: {{#crossLink "Management/killAll:method"}}{{/crossLink}}, {{#crossLink "Management/manageAll:method"}}{{/crossLink}}
@method disturbanceKill
@param {double} stem_to_soil_fraction (0..1) of stem biomass that is routed to the soil
@param {double} stem_to_snag_fraction (0..1) of the stem biomass continues as standing dead
@param {double} branch_to_soil_fraction (0..1) of branch biomass that is routed to the soil
@param {double} branch_to_snag_fraction (0..1) of the branch biomass continues as standing dead
@param {string} agent the disturbance agent that is mimicked ('fire' 'wind', 'bb', ...)
@return {integer} the number of killed trees.
*/
/**
Kill `fraction` (0..1) of all trees for which the Expression `filter` is _true_.

See also: {{#crossLink "Management/killAll:method"}}{{/crossLink}}, {{#crossLink "Management/manage:method"}}{{/crossLink}}
@method kill
@param {string} filter A expression to select a subset of the trees.
@param {double} fraction give the fraction (0..1) of trees that should be killed.
@return {integer} the number of killed trees.
@Example
    management.loadAll();
    // kill 30% of larch trees, and 60% of pine
    management.kill('species = lade', 0.3);
    management.kill('species = pisy', 0.6);
*/


/**
Use `managePct` to remove `n` trees sampled randomly from a given percentile range (given by `from` and `to`). All
trees are removed, if `n` is higher than the number of trees in that range.

The tree list needs to be sorted, before percentile based selection makes sense.

See also: {{#crossLink "Management/sort:method"}}{{/crossLink}}, {{#crossLink "Management/percentile:method"}}{{/crossLink}}

@method managePct
@param {integer} from lower percentile of the current tree distribution (0..100)
@param {integer} to higher percentile of the current tree distribution (0..100)
@param {integer} n the number of trees to harvest in the given percentile
@return {integer} the number of harvested trees
@Example
    var n = management.loadAll();
    management.sort('dbh');
    var n_rem = management.managePct(0,50, n*0.25);
    console.log('removed ' + n_rem + ' below median: ' + management.percentile(50) );
    // harvest 33% of the trees between the 80th and the 100th percentile.
    management.managePct(80,100, n*0.2 * 0.33);
*/
/**
Harvest all trees which are currently in the tree list.

See also: {{#crossLink "Management/manage:method"}}{{/crossLink}}, {{#crossLink "Management/killAll:method"}}{{/crossLink}}
@method manageAll
@return {integer} the number of harvested trees.
*/

/**
Remove a `fraction` (0..1) of all trees for which the Expression `filter` is _true_.

See also: {{#crossLink "Management/manageAll:method"}}{{/crossLink}}, {{#crossLink "Management/kill:method"}}{{/crossLink}}
@method manage
@param {string} filter A expression to select a subset of the trees.
@param {double} fraction give the fraction (0..1) of trees that should be harvested.
@return {integer} the number of harvested trees.
@Example
    management.loadAll();
    // kill 30% of larch trees, and 60% of pine
    management.manage('species = lade', 0.3);
    management.manage('species = pisy', 0.6);
*/


/**
Kill all saplings (i.e. trees <4m) which are located on grid pixels with the value `standID` on the grid `map`.

@method killSaplings
@param {Map} map A Map object with a stand grid.
@param {integer} standID the ID of the stand to process.
*/

/**
Kill all saplings (i.e. trees <4m) which are located on the resource unit identified by `ruindex`.

@method killSaplingsResourceUnit
@param {integer} ruindex the index of the resource unit to process.
*/

/**
(Hacky) function to modify the carbon content of the snag/soil pools of resource units covered by pixels with `standID` on the `map`.
If the resource unit is only partially covered, the factors are scaled accordingly.
The parameters are remove-fractions, i.e. values of 0 mean no change, values of 1 mean removal of 100% of the carbon of the respective pool.

@method removeSoilCarbon
@param {Map} map A Map object with a stand grid.
@param {integer} standID the ID of the stand to process.
@param {double} SWDfrac fraction of standing woody debris to remove.
@param {double} DWDfrac  fraction of downed woody debris to remove.
@param {double} litterFrac fraction of litter (yL) to remove.
@param {double} soilFrac fraction of SOM to remove..
*/

/**
Function to 'manage' snags, i.e. to cut down and move biomass to the soil pools.
The spatial approach is as described for {{#crossLink "Management/removeSoilCarbon:method"}}{{/crossLink}}.
The parameter `slash_fraction` describes (from 0..1) the fraction of biomass that should be transferred to the soil
(0: nothing, 1: 100%). Currently, the standing woody debris pool (SWD) and the 'other wood' pools are treated equally.

@method killSaplings
@param {Map} map A Map object with a stand grid.
@param {integer} standID the ID of the stand to process.
*/




Management = {}
