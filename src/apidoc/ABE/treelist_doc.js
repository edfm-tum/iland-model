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

    trees.load(); // load all trees
    trees.load(0.2); // load (randomly selected) 20% of all trees
    trees.load("rnd(0,1)<0.2)"); // the same as above, but using an Expression
    // load a subset of trees (for which the given Expression evalulates to true):
    trees.load("species=piab and dbh>20");

@method load
@param {String} filter optional filter criterion (see above).
@return {Integer} the number of trees loaded.
**/

/**
The `harvest()` method removes trees from iLand. When trees are harvested, the biomass is removed from the system (compare kill/cut).
When `harvest()` is called without parameters, then all trees that are currently in the list are removed (see also `load()`). Using `filter` and `fraction` the
selection of trees can be refined.

    trees.load(); // load all trees
    // remove 40% of trees.
    trees.harvest("", 0.4);
    // remove all Beech trees
    trees.harvest("species=fasy");

@method harvest
@param {String} filter optional filter criterion (see above), no filter when omitted.
@param {double} fraction of trees to harvest [0..1], default=1 (i.e. all trees harvested if omitted).
@return {Integer} the number of trees harvested.
**/

/**
When `simulate` is true, harvest operations are not actually executed, but affected trees are marked as either for harvest or killing. Calling `removeMarkedTrees()` then
executes the harvest operation.
Note: tree variables `markharvest`, `markcut`, `markcrop` are available for use in `filter()` or for visualization in iLand.
@propery simulate
@type Boolean
@default true
*/
