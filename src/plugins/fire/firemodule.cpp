#include "firemodule.h"

#include "model.h"
#include "modelcontroller.h"
#include "globalsettings.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "climate.h"

FireModule::FireModule()
{
    mFireLayers.setGrid(mGrid);
}

// access data element
FireData &FireModule::data(const ResourceUnit *ru)
{
    QPointF p = ru->boundingBox().center();
    return mGrid.valueAt(p.x(), p.y());
}
void FireModule::setup()
{
    // setup the grid (using the size/resolution)
    mGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
                GlobalSettings::instance()->model()->RUgrid().cellsize());

    // get the top node of settings for the fire module
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.fire"));
    // setup of the visualization of the grid
    GlobalSettings::instance()->controller()->addLayers(&mFireLayers);

}

void FireModule::setup(const ResourceUnit *ru)
{
    if (mGrid.isEmpty())
        throw IException("FireModule: grid not properly setup!");
    FireData &fire_data = data(ru);
    fire_data.setup();
}

/** perform the calculation of the KBDI drought index.
    see http://iland.boku.ac.at/wildfire#fire_ignition
  */
void FireModule::calculateDroughtIndex(const ResourceUnit *resource_unit, const WaterCycleData *water_data)
{
    FireData &fire_data = data(resource_unit);
    const ClimateDay *end = resource_unit->climate()->end();
    int iday = 0;
    double kbdi = 100.;
    const double mean_ap = fire_data.mRefAnnualPrecipitation; // reference mean annual precipitation
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
        fire_data.mKBDI = mean_kbdi;

    } else {
        // there is no fire season at all
        fire_data.mKBDI = 0;
    }

}


//*********************************************************************************
//******************************************** FireData ***************************
//*********************************************************************************

void FireData::setup()
{
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.fire"));
    mKBDIref = xml.valueDouble(".KBDIref", 300.);
    mRefMgmt = xml.valueDouble(".rFireSuppression", 1.);
    mRefAnnualPrecipitation = xml.valueDouble(".meanAnnualPrecipitation", -1);
}

//*********************************************************************************
//****************************************** FireLayers ***************************
//*********************************************************************************


double FireLayers::value(const FireData &data, const int param_index)
{
    switch(param_index){
    case 0: return data.mKBDI; // KBDI values
    case 1: return data.mKBDIref; // reference KBDI value
    default: throw IException(QString("invalid variable index for FireData: %1").arg(param_index));
    }
}

const QStringList FireLayers::names() const
{
    return QStringList() <<  "KBDI" << "KBDIref";

}


