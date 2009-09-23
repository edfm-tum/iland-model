/** @class SpeciesResponse calculates production relevant responses for a tree species on resource unit level.
*/
#include "speciesresponse.h"

#include "resourceunit.h"
#include "species.h"
#include "resourceunitspecies.h"

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

}

void SpeciesResponse::calcVpd()
{

    /*double *vpd = mRu->climate()->vpd();

    int begin, end;
    for (int m=0;m<12;m++) {
        mRu->climate()->monthRange(m, &begin, &end);
        for (int day=begin; day<end; day++) {
            mVpdResponse[m]+=f(vpd[day]);
        }
    }
    for (int m=0;m<12;m++)
        mVpdResponse[m]/=mRu->climate()->daysOfMonth(m);*/
}
