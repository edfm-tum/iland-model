# BiteEngine

The `BiteEngine` is the main container of the biotic disturbance engine that can contain multiple agents at the same time.

When the Bite system is enabled, a global variable `Bite` is available in the global Javascript context.

Bite is automatically run by iLand, but execution can be triggered via Javascript (`run()`).

## Properties
* ### `agents` (array of strings)
The `agents` property is a list of the names of currently available agents.

```
// loop over all agents, and dump info
for (var i=0;i<Bite.agents.length;++i)
   Bite.log(Bite.agent(Bite.agents[i]).info())
```
## Methods
* ### `agent(string agent_name)`: [agent](BiteAgent.md)
returns the [agent](BiteAgent.md) with the given name `agent_name`. 

* ### `log(string text)`
writes a log message to the console.

* ### `log(Object obj)`
writes a string representation of the Javascript object `obj` to the console.

* ### `run(int year)`
executes Bite (all agents). `year` is used internally as the curernt year (note: might be different than `Globals.year`)
