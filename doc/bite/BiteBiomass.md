# BiteBiomass


## Description

### Simple javascript based growth

If no `growthFunction` is provided, the growth of the cell is calculated in javascript using the `onCalculate` handler.

### logistic growth functions

A logistic growth function (e.g. `K / (1 + ( (K - M) / M))*exp(-r*t)`) can be used to model the biomass growth of the agent (see `growthFunction` for variable description). This mode is active when a value for `growthFunction` is provided.
The following steps are executed:

* calculate initial host biomass (see `hostBiomassTree` or `hostBiomassCell`), and initial value for `K`
* calcuate the annual growth `r` rate for the agent (`growthRateFunction`)
* for `growthIterations` steps:
  * estimate the agent biomass (via `growthFunction`,  and `t=1/growthIterations`)
  * calculate the amount of consumed biomass: `delta_bm = consumtion * agent_biomass * t`, with `agent_biomass` the average 
    agent biomass before and after the step
  * update the host biomass, and calculate a new value for `K` ( `K=remaining_host_biomass/consumption`)
* the algorithm stops, if either host or agent biomass is depleted (`agentBiomass` is set to 0 in this case).
* the variable `agentImpact` is set to the sum of consumed biomass during the year.


## Setup

* ### `hostTrees` (expression)
A filter expression to select those trees on the pixel which are considered as hosts.

* ### `hostBiomass` (dynamic expression)
A dynamic expression to calculate the host biomass. If `hostBiomass` is an expression, it  is evaluated for each host tree (>4m) (see `hostTrees`), and consequently the total host biomass is the sum over all trees. If `hostBiomass` is a Javascript
function the host biomass is the result of the function which is evaluated with the parameter `cell`.

Javascript can be used to access sapling biomass (and to combine trees and saplings):

```
   hostBiomass: function(cell) {
       cell.reloadSaplings(); // trees are loaded automatically
       cell.saplings.filter('species=fasy and height>0.5'); // only beech and >0.5
       var bm_saplings = cell.saplings.sum('nrep*foliagemass'); // foliage * number of represented stems per cohort
       // or: bm_saplings = cell.saplings.sum('nrep*foliagemass', 'species=fasy and height>0.5');
       var bm_trees = cell.trees.sum('foliagemass', 'species=fasy'); // foliage of all beech trees
       Bite.log(cell.info() + ": sap: " + bm_saplings + " tree: " + bm_trees); 
       return bm_saplings + bm_trees;
   }
```

* ### `mortality` (dynamic expression)
A dynamic expression, whose result is interpreted as the probability of mortality for the agent on the cell.
The expression is evaluated within the cell context.

* ### `growthFunction` (Expression)
The main growth function used to calcuate the the amount of agent biomass on a cell as a function of host biomass and environmental
conditions. The growth function has a number of pre-defined variables (case-sensitive!):

variable | description 
----------| ----------
M | current agent biomass on the cell
K | current maximum agent biomass (carrying capacity), calculated as `host_biomass / consumption` 
r | The growth rate, result of the `growthRateFunction`
t | the time step, set to `1/growthIterations`


* ### `growthRateFunction` (Expression)
calculates the growth rate used in `growthFunction`; the growth rate can be dependent on any cell variables (e.g. climatic variables).

* ### `consumption` (dynamic expression)
 consumption rate of the agent, defined as kg host biomass / units agent per year. It means, that an unit of agent
 consumes `consumption` kg of host biomass per year (e.g. a defoliater might consume x kg leaves / kg agent biomass per year). The property is a dynamic expression evaluated in the context of the current cell.

* ### `growthIterations` (integer)
The growth function 

* ### `verbose` (bool)
if `true` detailed log output of the growth cylce is written to the console.
## Properties

*no properties*

## Variables

* ### `hostBiomass` 
`hostBiomass` is a measure of the relevant host biomass that can be consumed/occupied by an agent. The value is the result of the host biomass calculation (see `hostBiomass` dynamic expression).
The value is set to 0 when a cell dies.

* ### `agentBiomass` 
`agentBiomass` is the total biomass of the agent on a cell. The `agentBiomass` is set to 0 when
a cell dies. The value of the variable is the result of the Javascript handler (`onCalculate`) or the result of the logistic growth.

* ### `agentImpact`
The `agentImpact` is a measure for the consumed host biomass during the year. Note that the type
of impact (e.g., foliage, roots) can differ. The value is the set as the difference in host biomass before and after the logistic growth.


## Events

* ### `onSetup(item)` 
The `onSetup` event is triggered after the setup of the item. 

`item` is the BiteBiomass object.

* ### `onCalculate(cell)` 
The handler is responsible for calculating the new state of the agent biomass. The handler is called
after the calcuation of the carrying capacity, and the the varible `agentBiomas` is set to the return value
of the function. Is only used when no `growthFunction` is present.


* ### `onMortality(cell)` 
The handler is invoked *if* the cell dies (i.e. only if the random choice based on the probability (see `mortality`)
decides the death of the agent on the cell).



