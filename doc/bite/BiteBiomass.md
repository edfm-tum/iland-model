# BiteBiomass


## Description

### Sequence of steps

The following steps are executed:
* calculation of the carrying capacity (based on the current vegetation on the cell)
* calculation of the new agent biomass (currently only via the `onCalculate` handler)
* estimate mortality probability

## Setup

* ### `hostTrees` (expression)
A filter expression to select those trees on the pixel which are considered as hosts.

* ### `carryingCapacityTree` (dynamic expression)
A dynamic expression to calculate the carrying capacity. The expression is evaluated for each host tree (>4m), 
and consequently the carrying capacity is the sum over all trees. 

* ### `carryingCapacityCell` (dynamic expression)
A dynamic expression to calculate the carrying capacity. The expression is evaluated once for the cell,
and the carrying capacity is set to the result of the expression.

* ### `mortality` (dynamic expression)
A dynamic expression, whose result is interpreted as the probability of mortality for the agent on the cell.
The expression is evaluated within the cell context.

## Properties

*no properties*

## Variables

* ### `carryingCapacity` 
`carryingCapacity` is a measure of the relevant host biomass that can be consumed/occupied by an agent. 
The value is set to 0 when a cell dies.

* ### `agentBiomass` 
`agentBiomass` is the total biomass of the agent on a cell. The `agentBiomass` is set to 0 when
a cell dies.


## Events

* ### `onSetup(item)` 
The `onSetup` event is triggered after the setup of the item. 

`item` is the BiteBiomass object.

* ### `onCalculate(cell)` 
The handler is responsible for calculating the new state of the agent biomass. The handler is called
after the calcuation of the carrying capacity, and the the varible `agentBiomas` is set to the return value
of the function. 


* ### `onMortality(cell)` 
The handler is invoked *if* the cell dies (i.e. only if the random choice based on the probability (see `mortality`)
decides the death of the agent on the cell).



