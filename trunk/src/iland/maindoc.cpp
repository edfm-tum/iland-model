/** \mainpage Welcome to the iLand Source Code Documentation!

\section intro_sec Introduction

The iLand wiki is located <a href="http://iland.boku.ac.at">here</a>

\section About the source code
to come.

 */

/** @defgroup core iLand core classes
  This module contains more/less core model of iLand.
  The class Model is the top level container of iLand. The Model holds a collection of ResourceUnits, links to SpeciesSet and Climate.
  ResourceUnit are grid cells with (currently) a size of 1 ha (100x100m). Many stand level processes (NPP produciton, WaterCycle) operate on this
  level.
  The Model also contain the landscape-wide 2m LIF-grid (http://iland.boku.ac.at/competition+for+light).

  The basic simulation entity of iLand is the individual Tree. Trees live on a ResourceUnit (i.e. trees are stored in lists owned by ResourceUnits).



 */

/** @defgroup tools iLand tools

 */
/** @defgroup script iLand scripting
The iLand classes and APIs for scripting (i.e.\ JavaScript) are described on this page.
 */

/** \page Javascript Objects

 Some general intro to javascript in iLand

- \subpage globals Object in global space
- \subpage climateconverter ClimateConverter
- \subpage csvfile CSVFile


Now you can proceed to the \ref advanced "advanced section".

*/
