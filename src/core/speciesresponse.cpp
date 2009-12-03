/** @class SpeciesResponse
    Environmental responses relevant for production of a tree species on resource unit level.
    SpeciesResponse combines data from different sources and converts information about the environment
    into responses of a species. The spatial level is the "ResourceUnit", where homogenetiy of environmental factors
    is assumed. The temporal aggregation depends on the factor, but usually, the daily environmental data is
    aggregated to monthly response values (which subsequently are used during 3PG production).
 Sources are:
    - vapour pressure deficit (dryness of atmosphere): directly from climate data (daily)
    - soil water status (dryness of soil): TODO (daily)
    - temperature: directly from climate data (daily)
    - phenology: @sa Phenology, combines several sources (quasi-monthly)
    - CO2: @sa SpeciesSet::co2Response() based on ambient CO2 level (climate data), nitrogen and soil water responses (yearly)
    - nitrogen: based on the amount of available nitrogen (yearly)
*/
#include "speciesresponse.h"

#include "resourceunit.h"
#include "species.h"
#include "resourceunitspecies.h"
#include "climate.h"
#include "model.h"
#include "watercycle.h"

SpeciesResponse::SpeciesResponse()
{
    mSpecies=0;
    mRu=0;
}
void SpeciesResponse::clear()
{
    for (int i=0;i<12;i++)
        mCO2Response[i]=mSoilWaterResponse[i]=mTempResponse[i]=mRadiation[i]=mUtilizableRadiation[i]=0.;

    mNitrogenResponse=0.;

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

    clear(); // reset values

    // calculate yearly responses
    const WaterCycle *water = mRu->waterCycle();
    const Phenology &pheno = mRu->climate()->phenology(mSpecies->phenologyClass());
    int veg_begin = pheno.vegetationPeriodStart();
    int veg_end = pheno.vegetationPeriodEnd();

    // yearly response
    const double nitrogen = mRu->resouceUnitVariables().nitrogenAvailable;
    // Nitrogen response: a yearly value based on available nitrogen
    mNitrogenResponse = mSpecies->nitrogenResponse( nitrogen );
    const double ambient_co2 = mRu->climate()->begin()->co2; // CO2 level of first day of year

    double water_resp, vpd_resp, temp_resp, min_resp;
    double  utilizeable_radiation;
    int doy=0;
    const ClimateDay  *end = mRu->climate()->end();
    for (const ClimateDay *day=mRu->climate()->begin(); day!=end; ++day) {
        // environmental responses
        water_resp = mSpecies->soilwaterResponse(water->psi_kPa(doy));
        vpd_resp = mSpecies->vpdResponse( day->vpd );
        temp_resp = mSpecies->temperatureResponse(day->temp_delayed);
        mSoilWaterResponse[day->month] += water_resp;
        mTempResponse[day->month] += temp_resp;

        if (doy>=veg_begin && doy<=veg_end) {
            // environmental responses for the day
            // combine responses
            min_resp = qMin(qMin(vpd_resp, temp_resp), water_resp);
            // calculate utilizable radiation, Eq. 4, http://iland.boku.ac.at/primary+production
            utilizeable_radiation = day->radiation * min_resp;
            mRadiation[day->month] += day->radiation;
        } else {
            utilizeable_radiation = 0.; // no utilizable radiation outside of vegetation period
        }
        mUtilizableRadiation[day->month]+= utilizeable_radiation;
        doy++;
    }
    // monthly values
    for (int i=0;i<12;i++) {
        mSoilWaterResponse[i]/=mRu->climate()->days(i);
        mTempResponse[i]/=mRu->climate()->days(i);
        mCO2Response[i] = mSpecies->speciesSet()->co2Response(ambient_co2,
                                                           mNitrogenResponse,
                                                           mSoilWaterResponse[i]);
    }

}


