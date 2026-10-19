# BiteImpact


## Description

### Sequence of steps

The following steps are executed:
* ask the `impactFilter` if an impact should happen
* filter host trees (if provided)
* run the impact (run either the `onImpact` handler, or the impact specified at the `impact` property)

### Specifying the impact
BiteImpact supports different impact targets (i.e. which plants/part of plants to affect), and ways to further specify the impact (i.e. which fraction of the population to affect). The total impact is a combination of multiple impacts.

#### Impact targets (`target`):

* `tree`: Trees (>4m height) are affected (mortality)
* `foliage`: The foliage of trees (>4m) is reduced by the agent
* `roots`: The root biomass is reduced (trees >4m) (not impl)
* `sapling`: Sapling cohorts (trees <4m height) are affected (i.e. mortality)
* `browsing`: The height growth of saplings (<4m) is set to 0 for the year of impact

#### Declaring impacts
The total impact is specified as an array of impacts:

```
...
impact: [ { impact 1}, {impact 2}, ... ],
...
```

An single impact is specified as a Javascript object with the following elements:
* `target`: The impact target as a string (e.g. `tree`, see list above)
* `fractionOfTrees`: a fraction specifiying which plants to affect (0..1). If omitted, all trees are affected.
* `fractionPerTree`: the fraction of the respective biomass compartment to remove (0..1). Only applicable for biomass targets.
* `maxTrees`: the maximum (absolute) number of plants to affect (optional)
* `maxBiomass`: if provided, maxBiomass caps the removal at the given amount of biomass.
* `order`: `order` specifies the order in which trees are processed. `order` is an Expression (provided as a string). For example, if `order` is '-dbh', then the biggest trees would be first in the list, and thus affected first. If omitted, the trees are selected randomly.
* `treeFilter`: an additional filter for trees / saplings (an expression). Applying a `treeFilter` within an item does not change the current tree / sapling list.

__Notes__:
* if both specified, the smaller value of `fractionOfTrees` and `maxTrees` is effective; e.g. if the total number of trees is 100, fractionOfTrees 0.5, and maxTrees 80, then 50 trees are affected. If also a `maxBiomass` is provided, it acts as an additional limit (i.e. if in the above example maxBiomass is achieved after 20 trees, then execution stops there).
* impact items are processed in the given sequence.
* the properties `fractionOfTrees`, `fractionPerTree`, `maxTrees`, and `maxBiomass` are dynamic expressions (i.e. can be calculated dynamically).
* if `maxBiomass` is provided, the model accounts for overshooting the threshold by scaling the impact of the last tree so that the total impact on the cell equals `maxBiomass`. 

Example:
```
impact: [ {target: 'tree', fractionOfTrees: 0.1, maxTrees: 100}, // 10 % of all trees die, but not more than 100 (selected randomly)
          {target: 'browsing'}, // all saplings are browsed within the cell
          {target: 'foliage', fractionPerTree: 0.5, maxBiomass: 100, order: 'dbh' } ], // remove 50% of foliage (starting with the smallest) and stop when the cumulative removal reaches 100kg 
```



## Setup

* ### `hostTrees` (expression)
A filter expression to select those trees on the pixel which are considered as hosts. Note that
`BiteBiomass` includes also this filter. If omitted or empty, no filtering will happen.

* ### `impactFilter` (dynamic expression)
A dynamic expression to determine whether an impact should occur on the cell (context is cell). The expression
should return a boolean value. 

Example:
```
impactFilter: 'agentImpact>0'  // impact only on cells where the biomass growth module indicates biomass loss
```

* ### `impact` (array of Javascript objects)
See the section about "Declaring impacts above". 

* ### `verbose` (boolean)
If `true`, the log contains more details. Mainly for testing purposes. Default: false.

* ### `simulate` (boolean)
If `true` no trees are harmed.


## Properties

*no properties*

## Variables

*no variables*

## Events

* ### `onImpact(cell)` 
The `onImpact` event is responsible to implement the agents impact on a cell and should
return the number of killed trees during the calculation. 


