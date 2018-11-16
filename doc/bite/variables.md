# BITE expressions, events, variables

## Dynamic expressions

Many properties of BITE elements are so called 'dynamic expressions'. 
These expressions are evaluated at run time and can be either constants, iLand expressions (see below), or 
Javascript functions. For example:

```
var elem = new BiteElement({
    // the three types of dynamic expressions:
	dynamic_expression: 5, // a constants
	dynamic_expression: 'agentBiomass*10', // an expression (provided as a string)
	dynamic_expression: function (cell) { return cell.value('agentBiomass') * 10; } // a javascript function
});
```

Dynamic expression are evaluated in a given context, i.e. either for individual trees (iLand trees), or 
cells (cells of the BITE agent). This influences which data is accessible during the evaluation of the
expression. For instance, if the context are trees, then [tree variables](iland.boku.ac.at/tree+variables) 
(such as `dbh` or `height`) can be used; if the context are cells, then cell variables (see below) can be
used. 

If the context is trees, then the result of the evaluation is often aggregated (also depending on the specific
context). For example, to determine host availability it is enough to find a single tree that meets
the given criteria, whereas in other cases a sum over the result of an expression over all trees 
(e.g. calculation of agent carrying capacity) is used.

Javascript functions are called with an object as a parameter (which depends on the context). The object is
either a [tree](/apidoc/classes/Tree.html) Javascript object (with a limited number of properties), or a
[cell object](link!!). Note that the performance of Javascript functions is generally
lower than that of expressions. *Todo*: performance considerations (parallelization, overhead)

## Events
Events are a mechanism that allows to run user-specific Javascript code to modify certain aspects of the 
model. Events are triggered by BITE and specific to the elements. Typical examples are:

* logging/debugging: events can be used to trace the execution flow; such events do not alter the agents behavior
* generation of output data: events can be used to create gridded or tabular output data
* user specific code, for example a "growth function"; such events change the behavior of the model

The naming scheme is always: "*on__EventName__*", e.g. `onEnter` or `onBeforeRun`. 

### Input
Depending on the context, events can have parameters; 

Parameters | Example
-------|------------
no     | agent level, before execution of the agent
cell | cell object with access to cell variables and functions; e.g. `onAfterImpact` which is called after a cell impact is calculated
item | the `BiteItem`(e.g. BiteDispersal, or BiteBiomass)

 
### Return value
Some events require a return value of the a function (for example the `onCalculate` event of the `BiteBiomass` element). Missing
return values trigger an error. 

## Variables

Variables are agent-specific (numerical) values that are stored for each cell of the landscape. BITE provides
three types of variables (see Figure x):

* __standard variables__: Variables that are available for each cell, e.g., the `index` 
* __element specific variables__: Variables that are specific to a used Bite-element; for example, the
`BiteBiomass` element adds the variables `agentBiomass` and `carryingCapacity` to a cell.
* __user-defined variables__: variables can be added by the user via Javascript; this mechanism can be used to
add spatial data to an agent (e.g. a map with colonization probabilities)

### Accessing variables

A list of all currently available variables can be retrieved via the `variables` property of an agent and
is also part of the agents `info()` dump. To see all variables:

```
Bite.log( agent.variables);
``` 

Variable values are accessed in Expressions simply via the variable name, e.g. `agentBiomass*10`. An error is reported if 
the variable does not exist. 

The `cell` object provides functions to access variables from Javascript. Use 
the `value()` function to read, `setValue()` to update/write a value, and `hasValue()` to test if a variable
exist. Unfortunately, The Qt Javascript framework does currently now allow to add properties dynamically 
(which would allow to use `cell.agentBiomass` instead of `cell.value('agentBiomass')`).

```
// access to the variables of the 'cell' object:
function accessCell(cell) {
	var x = cell.value('agentBiomass'); // read value
	if (x<0)
		cell.setValue('agentBiomass', 0); // modify the value
	if (!cell.hasValue('user1')) {
		Bite.log('Variable user1 not available!');
		return;
	}
} 
```

### Adding user defined variables

User-defined variables - which are essentially grids - can be added programmatically to an agent. The 
added data can be accessed by expressions or Javascript code within the agent. Examples for user-defined
data sets could be maps with initial infestation probability for an agent, 
or maps with pre-calculated climatic suitability.

The following example demonstrates the approach:

```
new BiteAgent({
   ... // other definitions
   onSetup: function(agent) { // onSetup event of the agent
	  var grid = new Grid('temp/outbreak_prob.asc'); // load a grid from file
	  agent.addVariable(grid, 'outbreakProb'); // add the grid with the name 'outbreakProb' to the agent
   }
});

```

