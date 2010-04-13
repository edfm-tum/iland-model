/** @class ResourceUnitSpecies
    The class contains data available at ResourceUnit x Species scale.
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
    mStatisticsDead.setResourceUnitSpecies(this);
    mStatisticsMgmt.setResourceUnitSpecies(this);
    mRemovedGrowth = 0.;
}


void ResourceUnitSpecies::calculate()
{
    if (mLAIfactor>0) {
        mResponse.calculate();///< calculate environmental responses per species (vpd, temperature, ...)
        m3PG.calculate();///< production of NPP
    } else {
        // if no LAI is present, then just clear the respones.
        // note: must be changed when regeneration is added...
        mResponse.clear();
        m3PG.clear();
    }
}


void ResourceUnitSpecies::updateGWL()
{
    // removed growth is the running sum of all removed
    // tree volume. the current "GWL" therefore is current volume (standing) + mRemovedGrowth.
    mRemovedGrowth+=statisticsDead().volume() + statisticsMgmt().volume();
}
