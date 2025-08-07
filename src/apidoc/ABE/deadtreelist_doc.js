// Javascript API documentation for the FMDeadTreeList class
/**
The DeadTreeList class represents a list of dead trees (snags or downed dead wood)
that can be queried and analyzed.

## Overview
### Initializing the list
The object can be created as a standalone object. Populating the dead tree list
is done using functions such as `loadFromStand()` or `loadFromRU()`.

#### Example
     var deadTrees = new DeadTreeList(); // create a DeadTreeList object
     deadTrees.loadFromStand(123, "Snags"); // load all snags from stand 123
     var totalVolume = deadTrees.sum('volume');
     console.log('Total volume of snags in stand 123: ' + totalVolume);


### Querying the content of the list
The list of dead trees can be filtered using `filter()`. Aggregate values
can be calculated using functions like `sum()` and `mean()`.

### DeadTreeType
When loading dead trees, you need to specify the type:
- `"Snags"`: Standing dead trees.
- `"DWD"`: Downed dead wood (lying on the ground).
- `"Both"`: Both snags and DWD.

### Variables
A list of available variables for dead trees would typically be found in the iLand documentation (analogous to sapling variables).
(Assuming a similar pattern to FMSaplingList regarding where variable lists are documented, e.g., https://iland-model.org/dead+tree+variables)

### General notes
The dead tree list heavily uses the expression engine of iLand (see https://iland-model.org/Expression).
Expressions are provided as strings to the respective Javascript functions (e.g., `filter`, `sum`, `mean`)
and evaluated by iLand.

## Example
     var dtl = new DeadTreeList();
     // Load all DWD from resource unit with index 0
     dtl.loadFromRU(0, "DWD");
     console.log("Number of DWD pieces: " + dtl.count);

     // Filter for DWD with a diameter greater than 20cm
     dtl.filter('dbh > 20');
     console.log("Number of large DWD pieces: " + dtl.count);

     // Calculate the mean decay status of these large DWD pieces
     var meanDecay = dtl.mean('decayClass');
     console.log("Mean decay status: " + meanDecay);

     // Load snags from stand 42, appending them to the current list,
     // only including those of species 'pisy' (Pinus sylvestris)
     dtl.loadFromStand(42, "Snags", 'species=pisy', true);
     console.log("Total dead tree elements after appending: " + dtl.count);


@class DeadTreeList
@constructor
Creates a new, empty DeadTreeList object.
    var myList = new DeadTreeList();
**/

/**
The number of dead tree elements (snags or DWD pieces) currently loaded in the list.
@property count
@type Integer
@readonly
*/

/**
Loads dead trees from a specific resource unit (RU) into the list.

@method loadFromRU
@param {Integer} ru_index The index of the resource unit from which to load dead trees.
@param {String} loadWhat Specifies the type of dead wood to load.
    Possible values are:
    - `"Snags"`: Load only standing dead trees.
    - `"DWD"`: Load only downed dead wood.
    - `"Both"`: Load both snags and DWD.
@param {Boolean} [append=false] If `true`, the newly loaded dead trees are appended to the existing list.
    If `false` (default), the list is cleared before loading.
@return {Integer} The number of dead tree elements loaded into the list.
@Example
     var deadwood = new DeadTreeList();
     // Load all snags from RU with index 5
     var numLoaded = deadwood.loadFromRU(5, "Snags");
     console.log(numLoaded + " snags loaded.");

     // Load DWD from RU 10 and append to the list
     deadwood.loadFromRU(10, "DWD", true);
     console.log("Total deadwood count: " + deadwood.count);
**/

/**
Loads dead trees from a specific stand into the list.


@method loadFromStand
@param {Integer} stand_id The ID of the stand from which to load dead trees.
@param {String} loadWhat Specifies the type of dead wood to load.
    Possible values are:
    - `"Snags"`: Load only standing dead trees.
    - `"DWD"`: Load only downed dead wood.
    - `"Both"`: Load both snags and DWD.
@param {String} [filter=""] An optional filter expression. Only dead trees for which this expression
    evaluates to true will be loaded. The list is cleared before loading new trees.
@return {Integer} The number of dead tree elements loaded into the list.
@Example
     var deadwood = new DeadTreeList();
     // Load all DWD from stand 77, with dbh > 10
     var numLoaded = deadwood.loadFromStand(77, "DWD", "dbh > 10");
     console.log(numLoaded + " DWD pieces loaded from stand 77.");

     // Load all snags from stand 88 (implicitly clears previous DWD)
     deadwood.loadFromStand(88, "Snags");
     console.log("Snags count from stand 88: " + deadwood.count);
**/

/**
Applies a filter to the current list of dead trees. Only elements for which the
`filter` expression evaluates to true remain in the list.

@method filter
@param {String} filter A valid filter expression (see iLand expression engine documentation).
@return {Integer} The number of dead tree elements remaining in the list after applying the filter.
@Example
     // Assume deadwood list is already populated
     // Keep only snags with volume > 0.5 m^3
     var remaining = deadwood.filter("snag=true and volume > 0.5");
     console.log(remaining + " large snags remain in the list.");
**/

/**
Calculates the mean value of a given `expression` for all dead tree elements in the list.
An optional `filter` can be applied to include only specific elements in the calculation.

@method mean
@param {String} expression A valid expression (e.g., a dead tree variable name) for which to calculate the mean.
@param {String} [filter=""] An optional filter expression. Only elements for which this expression
    evaluates to true will be included in the mean calculation.
@return {Number} The mean value of the `expression` for the (filtered) dead tree elements.
    Returns NaN if the list (after filtering) is empty.
@Example
     // Calculate the mean DBH of all DWD pieces
     var meanDbhDWD = deadwood.mean("dbh", "snag = false");
     console.log("Mean DBH of DWD: " + meanDbhDWD);

     // Calculate the mean age of all snags
     var meanAgeSnags = deadwood.mean("age", "snag = true'");
     console.log("Mean age of snags: " + meanAgeSnags);
**/

/**
Calculates the sum of a given `expression` for all dead tree elements in the list.
An optional `filter` can be applied to include only specific elements in the calculation.

Note: Counting elements that fulfill the `filter` expression can be expressed as: `sum('1', '<filter-expr>')`.

@method sum
@param {String} expression A valid expression (e.g., a dead tree variable name or '1' for counting)
    for which to calculate the sum.
@param {String} [filter=""] An optional filter expression. Only elements for which this expression
    evaluates to true will be included in the sum.
@return {Number} The sum of the `expression` for the (filtered) dead tree elements.
@Example
     // Calculate the total carbon content of all snags
     var totalCarbonSnags = deadwood.sum("carbon", "snag=true");
     console.log("Total carbon in snags: " + totalCarbonSnags);

     // Count the number of DWD pieces with decay class > 2
     var countAdvancedDecayDWD = deadwood.sum("1", "snag=false and decayClass > 2");
     console.log("Number of DWD pieces in advanced decay: " + countAdvancedDecayDWD);
**/
