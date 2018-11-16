# BiteDispersal


## Description

* probabilistic: spread adds probabilities, i.e: p(cell) = 1 - (1-p(cell))*(1-kernel)



## Setup

* ### `kernel` (expression)
A mathematical expression that defines the dispersal kernel. The function is a
(probability) density at distance `x`. Internally, the kernel function is discretized
to the cell size of the agent. The value for a cell is calculated as:

`p(cell) = f(distance_to_centerpoint_m)`

with `distance_to_centerpoint_m` the distance between the center point of `cell` and
the center point of the spreading cell.

In addition, the kernel is cut at `maxDistance`, and scaled to 1, i.e., the sum over all
cells of the kernel (including the center cell) after scaling is 1. 

* ### `maxDistance` (numeric, meters)
The dispersal kernel includes only cells for which the distance between the center point of the 
cell and the center point of the spreading cell is <= `maxDistance`.

* ### `debugKernel` (filename) (optional)
If provided, a ASCII raster file of the kernel is saved to `debugKernel`.

kernel value of a grid cell

## Properties

* ### `grid` [Grid](http://iland.boku.ac.at/apidoc/classes/Grid.html)
The internal dispersal grid (see also `dispersalGrid` variable). 

Note that the `grid` can be manipulated programmatically; for example, to add active
cells before the spreading algorithm:
```
... (definition of the BiteDispersal item) ...
onBeforeSpread: function(bit) { randomInitiate(10, bit.grid); Bite.log("added 10 px"); }

// set randomly 'n' values in grid 'gr' to '1'.
function randomInitiate(n, gr) {
	for (var i=0;i<n;++i) {
		var x = Math.random()*gr.width;
    var y = Math.random()*gr.height;
    gr.setValue(x,y,1);
	}
}
```
Note that functions for loading and saving grids are available; for instance to save the grid
after each execution the `onAfterSpread` handler can be used:
```
... (BiteDispersal definition) ...
onAfterSpread: function(item) {  item.grid.save(Globals.path("temp/dgrid" + step + ".asc")); }
```


## Variables

* ### `dispersalGrid` 
`dispersalGrid` is (after execution) the total aggregated input value for each cell (a value between 0 and 1).
Note that the grid can also be accessed via the `grid` property of BiteDispersal.
 

## Events

* ### `onSetup(item)` 
The `onSetup` event is triggered after the setup of the item. 

`item` is the BiteDispersal object.

* ### `onBeforeSpread(item)` 
called immediately before the spreading algorithm is executed. Useful for "injecting"
newly infested cells.

`item` is the BiteDispersal object.

* ### `onAfterSpread(item)` 
called immediately after the spreading algorithm is executed. Useful for inspecting or modifying the post-spread
state.

`item` is the BiteDispersal object.


