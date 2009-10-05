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

// fake: aggregated response values per month GO to webbrowser!!
const double totalResponses[] = {0., 0.05, 0.4, 0.6, 0.8, 0.8, 0.8, 0.5, 0.5, 0.1, 0. ,0. };
const double radYear = 3140.; // the sum of radMonth [MJ/m2]

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
    double response = mResponse->absorbedRadiation()[month] *
                      mResponse->vpdResponse()[month] *
                      mResponse->soilWaterResponse()[month] *
                      mResponse->tempResponse()[month];
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
    double year_raw_gpp_c = 0.;
    for (int i=0;i<12;i++) {
        mGPP[i] = 0.; mUPAR[i]=0.;
    }
    double utilizable_rad, epsilon;
    for (int i=0;i<12;i++) {
        mUPAR[i] = calculateUtilizablePAR(i); // utilizable radiation of the month times ...
        epsilon = calculateEpsilon(i); // ... photosynthetic efficiency ...
        mGPP[i] =utilizable_rad * epsilon; // ... results in GPP of the month (gC/m2)
        year_raw_gpp_c += mGPP[i]; // gC/m2
    }
    // calculate fac
    mRootFraction = 1. - abovegroundFraction();

    // global value set?
    double dbg = GlobalSettings::instance()->settings().paramValue("gpp_per_year",0);
    if (dbg) {
        year_raw_gpp_c = dbg * 1000. / 2.; // to g Carbon / m2
        mRootFraction = 0.4;
    }

    // year GPP/rad: kg Biomass (Carbon) * 2 / (yearly MJ/m2)
    double year_gpp_biomass = year_raw_gpp_c * 2 / 1000.; // conversion from gC/m2 -> kg/m2
    mGPPperRad = year_gpp_biomass / radYear;
    return mGPPperRad; // kg Biomass/MJ
}
