# BiteLifeCycle


## Description

The `BiteLifeCycle` controls several aspects of the agent behavior.
* voltinism: execution of the agent multiple times per year (e.g. for bark beetles with more
than one generation per year)
* timing of spread (frequency less than annual)

### controlling the timing of spreading


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


## Properties

*no properties*


## Variables

*no variables*

## Events

* ### `onSetup(item)` 
The `onSetup` event is triggered after the setup of the item. 

`item` is the `BiteLfeCycle` object.

