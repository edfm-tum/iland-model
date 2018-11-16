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

## Events
* ### `onSetup(agent)` 
The `onSetup` event is triggered after all items are created; 

`agent` is the `BiteAgent` object.

* ### `onYearBegin(agent)` 
The `onYearBegin` event is triggered before the agent is executed. This is a good place
for setting up user defined variables. 

`agent` is the `BiteAgent` object.

* ### `onYearEnd(agent)` 
The `onYearEnd` event is triggered after the execution of the agent.  

`agent` is the `BiteAgent` object.
