#include "global.h"
#include "production3pg.h"

#include "ressourceunit.h"
#include "species.h"

Production3PG::Production3PG()
{
}

// fake: monthly radiation (sum month MJ/m2)
const double radMonth[] = {30., 80., 200., 350., 450., 500., 550., 400., 300., 150., 100.,30. };
// fake: aggregated response values per month
const double totalResponses[] = {0., 0.05, 0.4, 0.6, 0.4, 0.4, 0.6, 0.3, 0.2, 0.1, 0. ,0. };
const double radYear = 3140.; // the sum of radMonth [MJ/m2]

double Production3PG::calculate(RessourceUnitSpecies &rus)
{
    double month_gpp[12];
    double year_raw_gpp = 0.;
    for (int i=0;i<12;i++) {
        month_gpp[i] = radMonth[i] * 0.5 * totalResponses[i];
        year_raw_gpp += month_gpp[i];
    }
    // PARutilized - fraction:.... to GPP:
    // year GPP/rad: gC / (yearly MJ/m2)
    double year_gpp_per_rad = year_raw_gpp / radYear;
    return year_gpp_per_rad;
}
