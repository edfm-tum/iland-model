#include "global.h"
#include "production3pg.h"

#include "ressourceunit.h"
#include "species.h"

Production3PG::Production3PG()
{
}

// fake: monthly radiation (sum month MJ/m2)
const double radMonth[] = {30., 80., 200., 350., 450., 500., 550., 400., 300., 150., 100.,30. };
// fake: aggregated response values per month GO to webbrowser!!
const double totalResponses[] = {0., 0.05, 0.4, 0.6, 0.8, 0.8, 0.8, 0.5, 0.5, 0.1, 0. ,0. };
const double radYear = 3140.; // the sum of radMonth [MJ/m2]

double Production3PG::calculate()
{
    double month_gpp[12];
    double year_raw_gpp = 0.;
    for (int i=0;i<12;i++) {
        month_gpp[i] = radMonth[i] * 2 * totalResponses[i];
        year_raw_gpp += month_gpp[i];
    }
    // calculate harshness factor
    mHarshness = 0.2; // fake

    // global value set?
    double dbg = GlobalSettings::instance()->settings().paramValue("npp_per_year",0);
    if (dbg)
        year_raw_gpp = dbg;


    // PARutilized - fraction:.... to GPP:
    // year GPP/rad: gC / (yearly MJ/m2)
    mGPPperRad = year_raw_gpp / radYear;
    return mGPPperRad;
}
