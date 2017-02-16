// documentation for the FMTreeList API


/**
The TreeList class (**`trees`** variable) represents a list of trees that can be manipulated (e.g., harvested) with functions of the class.
When javascript code is executed in the context of an Activity, a variable 'trees' is available in the global context. This instance
of a TreeList is linked automatically to the forest stand that is currently managed.

## Overview
### initializing the list
The main function for loading (i.e. making available for manipulation) trees of a forest stand is `load()`. `load()` can be used to load either
all trees, or a subset of trees (based on filter criteria).
### manipulating the content of the list
The trees in a list (loaded by `load()`) may be manipulated using functions such as `filter()` or `sort()`. There are functions helping to get aggregate values
(`mean()`, `sum()`), or to get the value of a given `percentile()`.
### manipulationg trees
Trees present in the tree list may be harvested or simply cut down. Main functions are `kill()`, `harvest()`. If `simulate` is true, harvested/killed trees
are only marked for removal. At a later point in time, all marked trees can be removed using `removeMarkedTrees()`.
### extra features
something here...

## Examples
    // 'trees' variable is always available.
    // this loads *all* trees of the current forest stand (i.e. trees>4m)
    trees.load();
    trees.simulate = true; // enable simulation mode (this is the default)
    // keep only spruce trees with a dbh>20cm in the list:
    trees.filter("species = piab and dbh>20");
    // now harvest all trees (since simulate=true, the trees are only marked for harvesting)
    trees.harvest();
    ...
    // later: remove all trees from the system that are marked for removal (harvest/kill)
    trees.removeMarkedTrees();

@class TreeList
***/


/**
The `load()` method is the main means to get all the trees of a forest stand into the list.
Per default, all trees (i.e. trees>4m height) that are located on the forest stand are loaded. If `filter` is provided, only a subset
of the trees are loaded. `filter` can either be a probability (between 0..1) for selecting individual trees, or an Expression using tree
variables.

    trees.load(0.2); // load (randomly selected) 20% of all trees
    trees.load("rnd(0,1)<0.2)"); // the same as above, but using an Expression
    // load a subset of trees (for which the given Expression evalulates to true):
    trees.load("species=piab and dbh>20");

@method load
@param {String} filter optional filter criterion (see above).
@return {Integer} the number of trees loaded.
**/


/**
When `simulate` is true, harvest operations are not actually executed, but affected trees are marked as either for harvest or killing. Calling `removeMarkedTrees()` then
executes the harvest operation.
Note: tree variables `markharvest`, `markcut`, `markcrop` are available for use in `filter()` or for visualization in iLand.
@property simulate
@type Boolean
@default true
*/

/**
the ID of the currently active stand (or -1).
@property stand
@type Integer
@readonly
*/

/**
the number of trees that are currently loaded in the list.
@property count
@type Integer
@readonly
*/

/**
Load all trees (living trees, >4m height) of the current `stand` into the internal list.

@method loadAll
@return {Integer} the number of loaded harvested.
**/


/**
Check all trees of the stand and either kill or harvest those trees that are marked for that operation.

See also: {{#crossLink "TreeList/simulate:property"}}{{/crossLink}}, {{#crossLink "TreeList/harvest:method"}}{{/crossLink}}

@method removeMarkedTrees
@return {Integer} the number of trees that have been removed.
**/

/**
Kill (i.e., cut down and do not remove from the forest) the trees in the list, filtered with `filter`.

See also: {{#crossLink "TreeList/simulate:property"}}{{/crossLink}}

@method kill
@param {String} filter A valid filter Expression.
@return {Integer} the number of trees that have been removed.
@Example
    trees.loadAll();
    trees.kill('dbh<10'); // kill all trees with a dbh<10cm
**/


/**
Apply a filter on the list. Only trees for which the filter condition `filter` is true, remain in the list.

See also: {{#crossLink "TreeList/simulate:property"}}{{/crossLink}}

@method filter
@param {String} filter A valid filter Expression.
@return {Integer} the number of trees that remain in the list.
@Example
    trees.loadAll();
    trees.filter('dbh>10'); // only trees with dbh>10 remain in the list
    trees.filter('incsum(volume)<100'); // keep trees with a total of 100m3 in the list


**/

/**
Remove the `fraction` of all trees [0..1] for which `filter` evalulates to `true`. Return number of removed trees.
When trees are harvested, the biomass is removed from the system (compare kill/cut).
When `harvest()` is called without parameters, then all trees that are currently in the list are removed (see also `load()`).


See also: {{#crossLink "TreeList/simulate:property"}}{{/crossLink}}, {{#crossLink "TreeList/kill:method"}}{{/crossLink}}

@method harvest
@param {string} filter A valid filter Expression.
@param {double} fraction The fraction [0..1] of trees that should be harvested. Default value is 1.
@return {Integer} the number of trees that have been removed.
@Example
    trees.loadAll();
    trees.harvest('species=piab', 0.2); // remove 20% (selected randomly) from all spruce
    trees.harvest('dbh>30'); // harvest all trees with dbh>30cm
**/

