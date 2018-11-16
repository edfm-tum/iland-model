# BiteColonization


## Description

The colonization process is a series of "filters" - in order to successfully colonize a cell,
the agent must pass all the filters:

Filter | Description
-------| -----------
active  | no colonization if the agent is already present on the cell (implicit)
dispersalFilter | no colonization if there is no (or not enough) dispersal input 
speciesFilter | no colonization if potential host tree species are not present (not implemented)
cellFilter | filter based on cell variables 
treeFilter | filter based on tree properties
onCalculate | Javascript event, that can prevent colonization



## Setup

* ### `dispersalFilter` (dynamic expression)
`dispersalFilter` has access to cell variables; no colonization if the result of the 
expression is either 0, or "false".

* ### `cellFilter` (dynamic expression)
"Constraint" object, i.e. either 1 or more dynamic expressions. Access to cell variables.

* ### `treeFilter` (dynamic expression)
"Constraint" object, i.e. either 1 or more dynamic expressions. Access to tree variables.
The filter is passed if at least one tree on the cell evaluates to 'true'.

For example: `treeFilter: 'species=piab and dbh>20'` is passed if one tree with the specified
properties exist on the cell (i.e., a spruce with a dbh >20cm)


## Properties

no properties.


## Variables

no variables.

## Events

* ### `onSetup(item)` 
The `onSetup` event is triggered after the setup of the item. 

`item` is the BiteColonization object.

* ### `onCalculate(cell)` 
called after all filters have been passed already. The cell is not colonized, if the function 
returns `false`.

`cell` is the Cell object.


