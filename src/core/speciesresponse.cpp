/** @class SpeciesResponse calculates production relevant responses for a tree species on resource unit level.
*/
#include "speciesresponse.h"

#include "resourceunit.h"
#include "species.h"
#include "resourceunitspecies.h"
#include "climate.h"

SpeciesResponse::SpeciesResponse()
{
}

void SpeciesResponse::setup(ResourceUnitSpecies *rus)
{
    mSpecies = rus->species();
    mRu = rus->ru();
    for (int i=0;i<12;i++)
        mVpdResponse[i]=mSoilWaterResponse[i]=mTempResponse[i]=0.;

    mCO2Response=mNitrogenResponse=0.;
}

/// Main function that calculates monthly / annual species responses
void SpeciesResponse::calculate()
{
    calcVpd();
}

void SpeciesResponse::calcVpd()
{

    ClimateDay *begin, *end;
    for (int mon=0;mon<12;mon++) {
        mRu->climate()->monthRange(mon, &begin, &end);
        for (ClimateDay *day=begin; day!=end; ++day) {
            mVpdResponse[mon]+=day->vpd;
        }
        mVpdResponse[mon] /= mRu->climate()->days(mon);


    }

}
