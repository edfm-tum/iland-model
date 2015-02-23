#include "bbgenerations.h"

#include "resourceunit.h"
#include "climate.h"

BBGenerations::BBGenerations()
{
}


/**
 * Calculate the bark temperatures for this year and a given resource unit.
 * Input: climate data (tmax (C), tmean (C), radiation (MJ/m2)
 * the LAI to estimate the radiation on the ground (Wh/m2)
 * Output: calculates for each day of the year the "effective" bark-temperature
 * Source: Schopf et al 2004: Risikoabschaetzung von Borkenkaefermassenkalamitaeten im Nationalpark Kalkalpen
 */
void BBGenerations::calculateBarkTemperature(const ResourceUnit *ru)
{
    // estimate the fraction of light on the ground (multiplier from 0..1)
    const double k = 0.5; // constant for the beer lambert function
    double ground_light_fraction = exp(-k * ru->leafAreaIndex() );


    for (int i=0;i<ru->climate()->daysOfYear();++i) {
        const ClimateDay *clim = ru->climate()->dayOfYear(i);
        // radiation: MJ/m2/day -> the regression uses Wh/m2/day -> conversion-factor: 1/0.0036

        // calc. maximum bark temperature
        double rt_max=1.656 + 0.002955*clim->radiation*ground_light_fraction/0.0036 + 0.534*clim->max_temperature + 0.01884 * clim->max_temperature*clim->max_temperature;
        double DiffRT=0;
    }

}
