# Bite
## General information

BITE (the _BIotic disTurbance Engine_), is a general model to simulate biotic disturbances in forest ecosystems. BITE is a modeling framework that is general enough to simulate a wide range of biotic disturbance agents, from fungi to large mammals. The framework is also simple and modular (in order to also be applicable in situations where knowledge about an agent is limited, as is the case with new invaders). BITE allows the quantification of the impacts of emerging pests and pathogens on forests in time and space.

The current implementation of BITE is coupled with the forest landscape simulation model [iLand](http://iland.boku.ac.at). See [iLand BITE](http://iland.boku.ac.at/BITE).


An important design strategy for achieving general applicability across a wide range of biotic disturbance agents is modularity. BITE models biotic disturbance agents in six distinct modules, i) potential habitat, ii) introduction, iii) dispersal, iv) colonization, v) population dynamics, and vi) impact (see figure below). Each module represents an important aspect of agent biology – such as agent dispersal – and provides specific options in the parameterization of a specific agent (e.g. a different dispersal kernel functions) to accurately characterize agent behavior. The level of detail implemented in each module can vary from simple to very complex, and each module can potentially use state variables such as agent biomass in the previous time step, vegetation structure and composition, and environmental conditions (Table 1). Modules can also be bypassed for selected agents if they are not applicable or if not enough information for their parametrization is available. 

![Bite](img/bite_overview.png ':size=600')

_The overall structure of the BITE framework for simulating biotic disturbance agents, consisting of six core modules of agent dynamics, and a coupling to a landscape model for simulating feedbacks between biotic disturbance agents and forest vegetation._

## Implementation

The BITE model is an modelling software component that simulates biotic disturbance agents. The core of BITE is implemented in C++ and relies on the [Qt-Library](https://qt.io), particularly on the integrated JavaScript engine. The C++ core provides an application programming interface (API) to JavaScript, a higher-level scripting language. By making use of the API, agent behavior is defined in the scripting environment by the user. Here, BITE is coupled with the individual-based forest landscape model iLand (http://iland.boku.ac.at). 

In this coupled version, vegetation (i.e., vegetation on a given cell) and environmental data (e.g., current climate data) are retrieved from iLand, and agent impacts (e.g., agent-induced mortality of trees) are reported back and executed in the landscape model. Thus, the interactions of different biotic disturbance agents with the vegetation are simulated explicitly on the landscape level. 


### Building blocks of an agent

A single biotic disturbance agent in BITE is created by defining a [BiteAgent](BiteAgent.md) entity in JavaScript.  The definition of an agent consists of several “items” each representing an individual biological process (e.g. dispersal, host colonization, or agent population dynamics). For instance, the [BiteDispersal](BiteDispersal.md) item handles the dispersal of the agent and uses a user-defined dispersal kernel function (e.g, with the property ‘kernel’). 

Note that the complexity of agents can vary: on the one hand, items can be excluded from the agent description if information is not available; on the other hand, much more complex agent behavior can be implemented by making use of custom JavaScript code (see for example the maxBiomass property of the BiteImpact item in Fig. S1). 

Most processes in BITE are executed on the level of individual grid cells. Therefore, each agent maintains a grid of cells [BiteCell](BiteCell.md) that covers the simulated landscape with an agent specific cell size. Vegetation information of each BiteCell is delivered and updated at each time step by iLand.


![Bite Example](img/bite_example.png ':size=600') 

_Building blocks in the definition of an agent in JavaScript._


## Reference

### Main components
* [BiteEngine](BiteEngine.md): the Bite module
* [BiteAgent](BiteAgent.md): a biotic disturbance agent
* [BiteCell](BiteCell.md): a single cell of an agent
* [expressions, events, variables](variables.md)


### Items

Item | Description
-----|----------
[BiteDispersal](BiteDispersal.md) | dispersal of agents on the landscape from source cells
[BiteDistribution](BiteDistribution.md) | availability of agents across the landscape (no spatial dispersal process)
[BiteColonization](BiteColonization.md) | the colonization of individual cells by the agent
[BiteBiomass](BiteBiomass.md) | growth of biomass and mortality of agents on a cell
[BiteImpact](BiteImpact.md) | the impact of an agent on the host vegetation on the cell (e.g. mortality of trees)
[BiteLifeCycle](BiteLifeCycle.md) | general agent properties such as voltinism
[BiteOutput](BiteOutput.md) | cell level detailed outputs per agent

## Code examples

[BITE Example code](CodeExamples.md)



