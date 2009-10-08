/** @class ResourceUnitSpecies contains data per ResourceUnit x Species
    Data stored is either statistical (i.e. number of trees per species) or used
    within the model (e.g. fraction of utilizable Radiation)
  */
#include "global.h"
#include "resourceunitspecies.h"

#include "species.h"
#include "resourceunit.h"
void ResourceUnitSpecies::setup(Species *species, ResourceUnit *ru)
{
    mSpecies = species;
    mRU = ru;
    mResponse.setup(this);
    m3PG.setResponse(&mResponse);
    mStatistics.setResourceUnitSpecies(this);
}


void ResourceUnitSpecies::calculate()
{
    mResponse.calculate();///< calculate environmental responses per species (vpd, temperature, ...)
    m3PG.calculate();///< production of NPP
}
