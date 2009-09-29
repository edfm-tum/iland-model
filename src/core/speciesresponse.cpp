/** @class SpeciesResponse calculates production relevant responses for a tree species on resource unit level.
*/
#include "speciesresponse.h"

#include "resourceunit.h"
#include "species.h"
#include "resourceunitspecies.h"
#include "climate.h"

SpeciesResponse::SpeciesResponse()
{
    mSpecies=0;
    mRu=0;
}
void SpeciesResponse::clear()
{
    for (int i=0;i<12;i++)
        mVpdResponse[i]=mSoilWaterResponse[i]=mTempResponse[i]=0.;

    mCO2Response=mNitrogenResponse=0.;

}
void SpeciesResponse::setup(ResourceUnitSpecies *rus)
{
    mSpecies = rus->species();
    mRu = rus->ru();
    clear();
}

/// Main function that calculates monthly / annual species responses
void SpeciesResponse::calculate()
{
    return; // do nothing!
    const ClimateDay *begin, *end;
    double no_of_days;

    clear(); // reset values
    for (int mon=0;mon<12;mon++) {
        mRu->climate()->monthRange(mon, &begin, &end);
        no_of_days = 0;
        for (const ClimateDay *day=begin; day!=end; ++day, no_of_days++) {
            // VPD response
            mVpdResponse[mon]+=mSpecies->vpdResponse( day->vpd );
            // Temperature Response
            mTempResponse[mon]+=mSpecies->temperatureResponse(day->temp_delayed);
        }
        mVpdResponse[mon] /= no_of_days; // vpd: average of month
        mTempResponse[mon] /= no_of_days; // temperature: average value of daily responses
    }
}


