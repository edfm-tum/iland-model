# BiteAgent

The `BiteAgent` is the entity that encapsulates a single disturbance agent (e.g. a beetle, or a fungus). 
An agent includes several "items" that represent individual processes (e.g., dispersal, host colonization,
or biomass growth). The behavior of an agent is thus the sum of pre-defined and customized building blocks.

## Setup

In order to create an agent, a `BiteAgent` object needs to be instantiated in Javascript:

```
// create an agent:
var my_agent = new BiteAgent( agent_def );
// access the agent in Javascript:
Bite.log(agent.name);
```

where `agent_def` is a Javascript object, that defines the details of the agent. An `agent_def` consists
of some global properties (that define for example the name of the agent), and one or several
"items" which itself are objects of one of the available Bite*-Items. The basic pattern is thus:

``` 
var my_agent = new BiteAgent({
	name: 'my-agent', ..., // global options
	some_name: new Bite*({ item_def }), // definition of an Item
	some_other_item: new Bite*({ item_def }), // another Item
	...,
	onSetup: function(agent) { ... }, // events
	...
});
```

See below for a description of global agent options and of Bite-Items; events are desribed at the end
of this page.


## global options
* ### `name` (string)
The name of the agent which has to be unique. The name is relevant for outputs and visualization.

* ### `description` (string)
The `description`  is a free-text element that can be used to describe the agent, or to include some
kind of versioning information.

* ### `cellSize` (integer)
The `cellSize` defines the grain at which the agent operates. Possible values are 10, 20, 50, or 100m. 
Note that multiple agents can coexist that use a different resolution.

* ### `climateVariables` (array of string)
`climateVariables` is a Javascript array that contains a list of climate variable names which can be used by the agent.
The elements of the array need to be one of the provided climate variabes (see below). Variables that are 'registered'
in this way can be subsequently used in expressions and accessed via Javascript.

Example:

```
	// define in the agent definition
	var agent = new BiteAgent({ ...,
	              climateVariables: ['MAP', 'MAT'], ... });
		      
       // later use, e.g. in an expression:
       dispersalFilter: 'MAT>7 and MAP<800' // restrict colonization to certain climatic conditions
       // or in javascript:
       onCalculate(cell) { if (cell.value('MAT')>7) { ... } }
       
```

Variables | Description
----------| -----------
MAT | mean annual temperature (°C)
MAP | annual precipitation sum (mm)
TMonthX | with `X` in 1..12, mean temperature of the month 1..12 (e.g. `TMonth6` for mean temp of June)
PMonthX | with `X` in 1..12, monthly precipitation sum of the month 1..12 (mm) (e.g. `PMonth6` for precipipation of June)
GDD | growing degree days (temperature sum of (t_mean-threshold) for all days with t_mean > threshold, with threshold=5°)
GDD10 | growing degree days (temperature sum of (t_mean-threshold) for all days with t_mean > threshold, with threshold=10°)


## Items

A `Bite*`-Item can be one of the following:


Item | Description
-----|----------
[BiteDispersal](BiteDispersal.md) | dispersal of agents on the landscape from source cells
BiteDistribution | availability of agents across the landscape (no spatial dispersal process)
BiteColonization | the colonization of individual cells by the agent
BiteBiomass | growth of biomass and mortality of agents on a cell
BiteImpact | the impact of an agent on the host vegetation on the cell (e.g. mortality of trees)
BiteLifeCycle | general agent properties such as voltinism

Note that at least one `BiteLifeCycle` object is mandatory for each agent.


## Properties

* ### `onTreeRemovedFilter`: int
The property determines for which trees that are removed in iLand the `onTreeRemoved` event handler is called. If the value = 0, the event is never triggered (the default). The value is a binary combination of one or more reasons (that are available as enumeration of the Tree Javascript object in iLand). 

See also: `onTreeRemoved()`


## Events
* ### `onSetup(agent)` 
The `onSetup` event is triggered after all items are created; this is a good place
for setting up user defined variables. 

`agent` is the `BiteAgent` object.

* ### `onYearBegin(agent)` 
The `onYearBegin` event is triggered before the agent is executed. 

`agent` is the `BiteAgent` object.

* ### `onYearEnd(agent)` 
The `onYearEnd` event is triggered after the execution of the agent.  

`agent` is the `BiteAgent` object.

* ### `onTreeRemoved(cell, tree, reason)` 
The `onTreeRemoved` event is triggered whenever a tree (>4m) is removed in iLand. The event is only triggered for specific reasons, which are specified with the `onTreeRemovedFilter` property.

`cell` is the `BiteCell` where the tree is located, `tree` the Javascript representation of the affected tree (http://iland.boku.ac.at/apidoc/classes/Tree.html), and `reason` one of the following:

Value | Enumeration | Description
------|----------| -----------
1 | Tree.RemovedDeath | natural mortality
2 | Tree.RemovedHarvest | Harvested by management
4 | Tree.RemovedDisturbance | killed by disturbance event (e.g. wind)
8 | Tree.RemovedSalavaged | tree removed by salvage operation (management)
16 | Tree.RemovedKilled | tree killed by management but remains as standing dead tree (but not removed from the forest)
32 | Tree.RemovedCutDown | tree killed and cut down by management (not removed from the forest)

```
new BiteAgent({
 ...
 	onTreeRemoved: function(cell, tree, reason) {  
		if (reason == Tree.RemovedHarvest)
			Bite.log(cell.info() + ":" + tree.species);
		// calculate basal area from the tree
		var ba = cell.agent.exprBasalArea.value(tree);
		// alternatively: var ba = tree.expr('basalarea'); or var ba=tree.dbh*tree.dbh/40000*3.141592;
		// update the user defined variable stockBasalArea
		cell.setValue('stockBasalArea', cell.value('stockBasalArea') + ba );
	},
	onSetup: function(agent) {
		// called during setup of the agent; 
		// event should be triggered when a tree is harvested or killed by disturbance
		// note the bitwise-OR operation
		agent.onTreeRemovedFilter = Tree.RemovedHarvest | Tree.RemovedDisturbance; 
		// create a variable that is used for tracking tree harvests
		agent.addVariable('stockBasalArea');
		// add a TreeExpr for efficient access to tree variables to the agent JS object
		agent.exprBasalArea = new TreeExpr("basalarea");
	}

})

```

## Methods

* ### `cell(int x, int y)`: BiteCell
returns a reference to the cell at index `x/y`.  An error is thrown if the coordinates are not valid.

See also: `isCellValid()`

* ### `isCellValid(int x, int y)`: bool
returns `true` if the a valid cell is at index `x/y`, `false` otherwise.

* ### `run()`
executes the agent for a cylce (i.e. a  year)

* ### `run(BiteCell cell)`
executes the agent for a given cell. Note: only cell-level items are executed.

* ### `item(string name)`: Item
retrieves the BiteItem with the name `name` (as a reference). Returns `null` if the there is no item with the given name.

* ### `info()`: string
returns a info dump about the agent as a string. The dump includes characteristics such as the cell size, item-related information and a list of all available variables.

* ### `evaluate(BiteCell cell, string expression)`: numeric
returns the result of the evaluation of `expression` in the context of `cell`. Cell variables are available.

* ### `addVariable(string var_name)`
adds a numeric grid as a variable to the agent that can be accessed with the name `var_name`. This allows to add custom variables to the agent that can be accessed by all items of the agent.

* ### `addVariable(Grid grid, string var_name)`
adds the numeric grid `grid` as variable to the agent that can be accessed with the name `var_name`. This allows to add spatial data
to the agent (e.g. for management purposes).

```
	onSetup: function(item) { 
		Bite.log( 'add the variable mgmtgrid to the agent' ); 
		var gr = Factory.newGrid();
		gr.load('temp/dgrid9.asc');
		item.agent.addVariable(gr, 'mgmtgrid');
		}, // part of the Item
```

* ### `updateDrawGrid(string expression)`
fills the internal draw grid of the agent (which is also for used for visualization) with the result of `expression`.

```
   my_agent.updateDrawGrid('mgmtgrid'); // fills the internal grid with the 'mgmtgrid' variable
   my_agent.drawGrid().save('temp/test.asc'); // access the draw grid
```
* ### `updateDrawGrid(JSfunction function)`
fills the internal draw grid of the agent (which is also for used for visualization) with the result of the javascript function `function`. The `function` is called with the BiteCell as variable.

```
   my_agent.updateDrawGrid(function(cell){return cell.value('mgmtgrid'); }); // fills the internal grid with the 'mgmtgrid' variable
   my_agent.drawGrid().save('temp/test.asc'); // access the draw grid
```

* ### `updateVariable(string var_name, double value)`
sets the values of the grid variable `var_name` to `value`.

See also `addVariable()`.

* ### `updateVariable(string var_name, string expression)`
sets the values of the grid variable `var_name` with the result of `expression`.

See also `addVariable()`.

* ### `updateVariable(string var_name, JSfunction function)`
sets the values of the grid variable `var_name` with the result of the Javascript function `function`. The function is called with the BiteCell as parameter.

See also `addVariable()`.

```
// sets 'my_var' to 'index' + some random noise (admittedly not useful)
my_agent.updateVariable('myvar', function(cell) { return cell.value('index')+1000*Math.random(); } );
// to save 'my_var' as a grid:
my_agent.saveGrid('my_var', 'temp/test.asc');
```
      
* ### `drawGrid()`: Grid
returns a Javascript reference to the internal grid of the agent (see http://iland.boku.ac.at/apidoc/classes/Grid.html).

* ### `saveGrid(string expression, string file_name)`
convenience function to save the expression `expression` to a file (relative paths are relative to the project folder).


```
