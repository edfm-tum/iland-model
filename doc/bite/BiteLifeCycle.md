# BiteLifeCycle


## Description

The `BiteLifeCycle` controls several aspects of the agent behavior.
* voltinism: execution of the agent multiple times per year (e.g. for bark beetles with more
than one generation per year)
* timing of spread (frequency less than annual)
* the mortality of cells (if not covered by the [Biomass item](BiteBiomass.md) )

### controlling the timing of spreading
tbc

### outbreak waves
Some agents show outbreak waves, i.e. phases (usually multiple years) with increased activity. This behavior
is also implemented in Bite: to control the start and duration of outbreak waves, the properties  
`outbreakDuration` and `outbreakStart` of the LifeCycle item can be used; the cell variable `outbreakYears` reflects
the state of the cell and its value can be used to alter the behavior of the agent. `outbreakYears` is 0 for non-outbreak
years (or if the outbreak mechanism is not used). The start of an outbreak is determined by the `outbreakStart` property.
During an outbreak `outbreakYears` counts the years while in an outbreak phase (starting with 1). The end of an outbreak
is initiated by the `outbreakDuration` property.

__Example__

The example uses Javascript to *prescribe* the start of each outbreak, and a variable outbreak length. The impact ...

```
// helper function in global scope that is called 
// from the LifeCyclce item during the simulation
function outbreakSeries() {
   var _list = [10, 23, 37, 45]; // (dummy) years on which to start an outbreak
   for (i in _list)
      if (_list[i] > Globals.year)
        return _list[i]-Globals.year; // Globals.year: iLand global year counter
   return 0;
}

// agent definition
  ...
	lifecycle: new BiteLifeCycle({  
        ....
				outbreakStart: function(cell) { return outbreakSeries(); }, // force the start of outbreak waves at fixed years
				outbreakDuration: 'rnd(4,5.999)'   // the duration is 4 or 5 years
		}), ....
    
  // biomass item:
  growth: new BiteBiomass({ ...
  // lower mortality probability during outbreaks:
  mortality: function(cell) {
     if (cell.outbreakYears == 0) 
        return 0.9; 
     else
       return return (0.9/(1+Math.exp(-(cell.outbreakYears-2.5))));
  },
  ...

```


## Setup

* ### `voltinism` (dynamic expression)
*tbc* (not implemented)

* ### `spreadDelay` (integer) (optional)
if a non-0 value is provided, a cell can only start spreading when occupied for `spreadDelay` years.


* ### `spreadFilter` (dynamic expression) *TODO: bad name*
Filter (cell context); a cell may spread with a probability given by that expression.

* ### `spreadInterval` (dynamic expression) 
controls how frequent a cell can spread; at least `spreadInterval` years have to be between consecutive
spread from a cell.

* ### `mortality` (dynamic expression) 
The cell dies, if the expression (with cell context) returns `true`. 

* ### `outbreakDuration` (dynamic expression, optional) 
The number of years that an outbreak lasts (evaluted at the begin of an outbreak). The value is truncated to an integer.

* ### `outbreakStart` (dynamic expression, optional) 
The number of years until the next outbreak (0 or missing means no outbreaks at all). Evaluated at the beginning of the
simulation and after each outbreak wave.


## Properties

*no properties*


## Variables

*no variables*

## Events

* ### `onSetup(item)` 
The `onSetup` event is triggered after the setup of the item. 

`item` is the `BiteLfeCycle` object.

