# BiteDistribution


## Description



## Setup

* ### `map` (string) (optional)
Filename of a raster file that is loaded into the internal grid (see `grid`). If no map is 
loaded, then the internal grid is set to 1 on all cells.


## Properties

* ### `grid` [Grid](http://iland.boku.ac.at/apidoc/classes/Grid.html)
The internal dispersal grid (see also `dispersalGrid` variable). 



## Variables

* ### `dispersalGrid` 
`dispersalGrid` is the grid holding either a constant value (1), or the content of a file (see `map`).
Note that the grid can also be accessed via the `grid` property.
 

## Events

* ### `onSetup(item)` 
The `onSetup` event is triggered after the setup of the item. 

`item` is the BiteDispersal object.

* ### `onCalculate(item)` 
called when the item is executed.

`item` is the BiteDistribution object.




