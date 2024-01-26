// Javascript API documentation for the FMSaplingList class
/**
The SaplingList class represents a list of saplings (=cohort of trees <4m height) that can be manipulated (e.g., removed) with functions of the class.

## Overview
### initializing the list
The object is available automatically when using the Bite-submodule.

`SaplingList` can also be used as a standalone object, but in this case populating the sapling list needs to be
done by the user with functions such `loadFromStand()`.

#### Example
    var saps = new SaplingList; // create a SaplingList object
    saps.loadFromStand(123); // load saplings of stand 123
    saps.kill(); // and be nasty


### manipulating the content of the list
The list of sapling cohorts may be manipulated using functions such as `filter()`. There are functions helping to get aggregate values
(`sum()`).
### manipulationg saplings
The class provides function to kill cohorts (`kill()`) or to modify cohorts (e.g. `browse()`).

### Variables
A list of available variables for saplings:  https://iland-model.org/sapling+variables

### general notes
The sapling list heavily used the expression engine of iLand https://iland-model.org/Expression. Expressions are provided as strings to the
respective Javascript functions (e.g.,`filter`) and evaluated by iLand. Note that tree species names are treated as a special case (https://iland-model.org/Expression#Constants).

## Example
    var saplings = biteAgent.cell(0,0).saplings; // currently, only available via Bite
    // load of saplings in Bite: bitecell.reloadSaplings();

    saplings.filter('height > 2'); // keep only larger cohorts
    trees.sum('foliagemass*nrep'); // calculate total leaf mass of all trees in a cohort
    saplings.browse(); // affect saplings (browsing: reduced height growth)

@class SaplingList
***/

/**
the number of cohorts that are currently loaded in the list.
@property count
@type Integer
@readonly
*/

/**
Kill (i.e., cut down and do not remove from the forest) the saplings in the list, filtered with `filter`. If `filter` is omitted, all saplings are killed.


@method kill
@param {String} filter A valid filter Expression.
@return {Integer} the number of trees that have been removed.
@Example
    saplings.kill('age<5'); // kill young saplings
**/

/**
Clear the list without affecting the saplings in the list. Note that explcitly clearing the list is usually not necessary (e.g. when using loadFromStand() )

@method clear
@Example
    saplings.clear(); // empty lsit
    console.log(saplings.count); // -> 0
**/

/**
Load all (or a subset) of sapling that are located on the stand denoted by `standId`. The `filter` let you control
which saplings should be loaded. Control with `do_append` whether the list should be cleared before loading saplings.


@method loadFromStand
@param {Integer} standId A numeric standId that refers to the stand-grid of iLand.
@param {string} filter A valid filter Expression (see above), can be omitted
@param {bool} do_append if true, saplings are added to the list without clearing the list first. Default is false (list is cleared). Can be omitted.
@return {Integer} the number of trees that have been loaded.
@Example
    saplings.loadFromStand(123, 'species=fasy'); // load all beech saplings of stand 123

    saplings.loadFromStand(124, 'true', true); // append (all) saplings from stand 124

**/

/**
Affect all saplings in the list by browsing; this reduces the height growth in the current year to 0.

@method browse
@param {Boolean} do_browse if `true`, the cohorts are marked as browsed (`false` unmarks cohorts)
@return {Integer} the number of trees that have been removed.
@Example
    saplings.browse(); // browse saplings
**/

/**
Apply a filter on the list. Only saplings for which the filter condition `filter` evaluates to true, remain in the list.

@method filter
@param {String} filter A valid filter Expression.
@return {Integer} the number of trees that remain in the list.
@Example
    saplings.filter('species=fasy'); // remove all trees that are not Beech
    trees.filter('incsum(foliagemass*nrep)<1'); // max foliage mass after filtering 1kg

**/

/**
Calculates the sum of a given `expr` that can contain any of the available sapling variables. The optional `filter` is another expression
that filters the trees that are processed. Returns the sum of `expr`.

Note: counting saplings that fulfill the `filter` expression can be expressed as: `sum('1', '<filter-expr>')`.

@method sum
@param {string} expr a valid expression that should be processed
@param {string} filter only saplings that pass the filter are processed
@return {double} the sum over the value of `expr`of the population
@Example
    console.log('number of repr. trees: ' + saplings.sum('nrep') ); // just a count, same as saplings.length
    console.log('cohorts > 1m: ' + trees.sum('1', 'height>1') ); // note that '1' is a string (and parsed by the expression engine)
**/


