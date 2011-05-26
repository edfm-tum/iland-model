#include "global.h"
#include "fireplugin.h"

#include <QVector3D>

#include "model.h"
#include "globalsettings.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "climate.h"

Q_EXPORT_PLUGIN2(iland_fire, FirePlugin)

QString FirePlugin::name()
{
    return "fire";
}

QString FirePlugin::version()
{
    return "0.1";
}

QString FirePlugin::description()
{
    return "Fire disturbance module for iLand. The fire ignition and fire spread follows the FireBGC v2 model (Keane et al 2011), " \
            "the estimation of severity and fire effects Schumacher et al (2006). See http://iland.boku.ac.at/wildfire for details.\n" \
            "Designed and written by by Rupert Seidl/Werner Rammer.";
}


FirePlugin::FirePlugin()
{
    qDebug() << "Fire plugin created";
//    foreach (const ResourceUnit *ru, GlobalSettings::instance()->model()->ruList())
    //        qDebug() << ru->boundingBox() << ru->constTrees().count();
}


/** perform the calculation of the KBDI drought index.
    see http://iland.boku.ac.at/wildfire#fire_ignition
  */
void FirePlugin::calculateWater(const ResourceUnit *resource_unit, const WaterCycleData *water_data)
{
    ClimateDay *end = resource_unit->climate()->end();
    int iday = 0;
    double kbdi = 100.;
    const double mean_ap = 3000; // reference mean annual precipitation
    double dp, dq, tmax;
    int first_day = -1, last_day = -1; // to calculate fire season
    double kbdis[366];
    for (const ClimateDay *day = resource_unit->climate()->begin(); day!=end; ++day, ++iday) {
        dp = water_data->water_to_ground[iday]; // water reaching the ground for this day
        double wetting = - dp/25.4 * 100.;
        kbdi += wetting;
        if (kbdi<0.) kbdi=0.;

        tmax = day->temperature; // !!! TODO!!! use max temperature!!!!
        // drying is only simulated, when:
        // * the temperature > 10°
        // * there is no snow cover
        if (tmax > 10. && water_data->snow_cover[iday]==0.) {
            // calculate drying: (kbdi already includes current wetting!)
            dq = 0.001*(800.-kbdi)*( (0.9676*exp(0.0486*(tmax*9./5.+32.))-8.299) / (1 + 10.88*exp(-0.0441*mean_ap/25.4)) );

            kbdi += dq;
        }
        kbdis[iday] = kbdi;
        // calculate length of fire season dynamically:
        // use a threshold value of 200 because this relates to "stage 1"
        if (kbdi>200.) {
            if (first_day == -1)
                first_day = iday;
            last_day = iday;
        }
    }

    // now calculate a mean value
    if (first_day>=0 && last_day>=0) {
        double mean_kbdi=0.;
        for (iday = first_day; iday<=last_day; ++iday)
            mean_kbdi += kbdis[iday];
        mean_kbdi /= (last_day - first_day) + 1;

    } else {
        // there is no fire season at all
    }


}
