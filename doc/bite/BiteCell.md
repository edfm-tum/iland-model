# BiteCell

A cell is the smalled spatial execution unit of an agent. Each agent has a grid of cells that covers the full simulated landscape with
the cell size defined by the agent.

## Properties
* ### `active` (bool)
A cell is `active` when currently colonized by the agent. Setting `active` to `true` initiates the agent for the given cell.

* ### `spreading` (bool)
is `true` when a cell is activelty dispersing the agent. Can be set via Javascript.

* ### `yearsLiving` (int, read-only)
The number of years a cell is continuously occupied by the agent (the value is reset if a cell dies).

* ### `outbreakYears` (int, read-only)
The number of years the current agent is in an outbreak phase. The value is 0 if outbreaks are not 
simulated (see [LifeCycle](BiteLifeCycle.md) for details).


* ### `agent` ([agent](BiteAgent.md))
A reference to the agent object.

* ### `trees` (TreeList)
A list of trees (see http://iland.boku.ac.at/apidoc/classes/TreeList.html) on the cell (trees >4m). The `trees` can be queried, filtered or modified. The tree
list is populated automatically during execution of the agent (but see `reloadTrees()`).

```
// use in dynamic expressions: an aggregate over all trees in the list
agentBiomassCell: function(cell) { cell.trees.sum('stemmass*0.23'); }, ...
```

* ### `saplings` (SaplingList)
A list of saplings (tree cohorts <4m) (see http://iland.boku.ac.at/apidoc/classes/SaplingList.html) on the cell. The `saplings` can be queried, filtered or modified. The sapling list is populated automatically during execution of the agent (but see `reloadSaplings()`).

__Note__: this is under construction - you may have to reload saplings explicitly. 

## Methods
* ### `hasValue(string var_name)`: bool
returns `true` if a variable with the name `var_name` is available.

* ### `value(string var_name)`: numeric
returns the value of the variable `var_name`, or throws an error if the variable is not available.

* ### `setValue(string var_name, numeric value)`
updates the value of `var_name` with `value`. Note that not all variables can be updated (e.g. climate variables are read only).

* ### `reloadTrees()`
(re-)loads all trees on the cell to the internal list (see `trees`). Reloading ignores all potential filters that have been 
applied previously to the list of trees.

* ### `reloadSaplings()`
(re-)loads all saplings on the cell to the internal list (see `saplings`). Reloading ignores all potential filters that have been 
applied previously to the list.

* ### `die()`
lets the agent on a cell die (i.e. set biomass to 0, `active` and `spreading` to `false`).
