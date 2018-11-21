# BiteImpact


## Description

### Sequence of steps

The following steps are executed:
* ask the `impactFilter` if an impact should happen
* filter host trees (if provided)
* run the impact (either kill all trees, or run the `onImpact` handler)

## Setup

* ### `hostTrees` (expression)
A filter expression to select those trees on the pixel which are considered as hosts. Note that
`BiteBiomass` includes also this filter. If omitted or empty, no filtering will happen.

* ### `impactFilter` (dynamic expression)
A dynamic expression to determine whether an impact should occur on the cell. The expression
should return a boolean value.

* ### `killAllHostTrees` (boolean)
If `true` the impact is simply to kill all trees (>4m)

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


