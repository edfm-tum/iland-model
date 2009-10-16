#include "global.h"
#include "production3pg.h"

#include "resourceunit.h"
#include "species.h"
#include "speciesresponse.h"
#include "model.h"

Production3PG::Production3PG()
{
    mResponse=0;
}

/**
  This is based on the utilizable photosynthetic active radiation.
  @sa http://iland.boku.ac.at/primary+production
  The resulting radiation is MJ/m2       */
inline double Production3PG::calculateUtilizablePAR(const int month) const
{
    // calculate the available radiation

    // there is no production outside of the vegetation period
    if (mResponse->absorbedRadiation()[month]==0.)
        return 0.;
    // see Equation (3)
    // multiplicative approach: responses are averaged one by one and multiplied on a monthly basis
//    double response = mResponse->absorbedRadiation()[month] *
//                      mResponse->vpdResponse()[month] *
//                      mResponse->soilWaterResponse()[month] *
//                      mResponse->tempResponse()[month];
    // minimum approach: for each day the minimum aof vpd, temp, soilwater is calculated, then averaged for each month
    double response = mResponse->absorbedRadiation()[month] *
                      mResponse->minimumResponses()[month];

    return response;
}
/** calculate the alphac (=photosynthetic efficiency) for the given month.
   this is based on a global efficiency, and modified per species.
   epsilon is in gC/MJ Radiation
  */
inline double Production3PG::calculateEpsilon(const int month) const
{
    double epsilon = Model::settings().epsilon; // maximum radiation use efficiency
    epsilon *= mResponse->nitrogenResponse() *
               mResponse->co2Response();
    return epsilon;
}

inline double Production3PG::abovegroundFraction() const
{
    double harsh =  1 - 0.8/(1 + 2.5 * mResponse->nitrogenResponse());
    return harsh;
}

/** calculate the alphac (=photosynthetic efficiency) for given month.
  @sa http://iland.boku.ac.at/primary+production */
double Production3PG::calculate()
{
    Q_ASSERT(mResponse!=0);
    // Radiation: sum over all days of each month with foliage
    double year_raw_gpp = 0.;
    for (int i=0;i<12;i++) {
        mGPP[i] = 0.; mUPAR[i]=0.;
    }
    double utilizable_rad, epsilon;
    // conversion from gC to kg Biomass: C/Biomass=0.5
    const double gC_to_kg_biomass = 2. / 1000.;
    for (int i=0;i<12;i++) {
        utilizable_rad = calculateUtilizablePAR(i); // utilizable radiation of the month times ...
        epsilon = calculateEpsilon(i); // ... photosynthetic efficiency ...
        mUPAR[i] = utilizable_rad ;
        mGPP[i] =utilizable_rad * epsilon * gC_to_kg_biomass; // ... results in GPP of the month kg Biomass/m2 (converted from gC/m2)
        year_raw_gpp += mGPP[i]; // kg Biomass/m2
    }
    // calculate fac
    mRootFraction = 1. - abovegroundFraction();

    // global value set?
    double dbg = GlobalSettings::instance()->settings().paramValue("gpp_per_year",0);
    if (dbg) {
        year_raw_gpp = dbg ;
        mRootFraction = 0.4;
    }

    // year GPP/rad: kg Biomass/m2
    mGPPperArea = year_raw_gpp;
    return mGPPperArea; // yearly GPP in kg Biomass/m2
}
