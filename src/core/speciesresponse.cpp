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
        mCO2Response[i]=mResponseMinima[i]=mVpdResponse[i]=mSoilWaterResponse[i]=mTempResponse[i]=mRadiation[i]=0.;

    mNitrogenResponse=mSoilWaterResponseYear=0.;

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
    const ClimateDay *begin, *end;
    double no_of_days;

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

    // calculate monthly responses
    int doy=0;
    double water_resp;
    double vpd_resp;
    double temp_resp;
    double min_resp;
    for (int mon=0;mon<12;mon++) {
        mRu->climate()->monthRange(mon, &begin, &end);
        no_of_days = 0;
        for (const ClimateDay *day=begin; day!=end; ++day, no_of_days++) {
            // VPD response
            vpd_resp =mSpecies->vpdResponse( day->vpd );
            mVpdResponse[mon]+= vpd_resp;
            // Temperature Response
            temp_resp = mSpecies->temperatureResponse(day->temp_delayed);
            mTempResponse[mon]+= temp_resp;

            // radiation: only count days in vegetation period
            water_resp = mSpecies->soilwaterResponse(water->psi_kPa(doy));
            if (doy>=veg_begin && doy<=veg_end) {
                mRadiation[mon] += day->radiation;
                mSoilWaterResponseYear += water_resp;
            }

            // minimum response
            min_resp = qMin(qMin(vpd_resp, temp_resp), water_resp);
            mResponseMinima[mon] += min_resp;

            // soil water
            mSoilWaterResponse[mon] += water_resp;

            doy++;
        }
        mVpdResponse[mon] /= no_of_days; // vpd: average of month
        mTempResponse[mon] /= no_of_days; // temperature: average value of daily responses
        mSoilWaterResponse[mon] /= no_of_days; // water response: average of daily responses
        mResponseMinima[mon] /= no_of_days; // minimum responses: average of daily responses
        // CO2 response: a monthly value based on atmospheric CO2 concentration and the response to nutrients and water in the soil.
        mCO2Response[mon] = mSpecies->speciesSet()->co2Response(ambient_co2,
                                                           mNitrogenResponse,
                                                           mSoilWaterResponse[mon]);


    }

    if (pheno.vegetationPeriodLength()>0)
        mSoilWaterResponseYear /= pheno.vegetationPeriodLength();
    else
        mSoilWaterResponseYear = 0.;



}


