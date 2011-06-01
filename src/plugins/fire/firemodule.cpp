#include "firemodule.h"

#include "model.h"
#include "modelcontroller.h"
#include "globalsettings.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "climate.h"

FireModule::FireModule()
{
    mFireLayers.setGrid(mRUGrid);
}

// access data element
FireRUData &FireModule::data(const ResourceUnit *ru)
{
    QPointF p = ru->boundingBox().center();
    return mRUGrid.valueAt(p.x(), p.y());
}
void FireModule::setup()
{
    // setup the grid (using the size/resolution)
    mRUGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
                GlobalSettings::instance()->model()->RUgrid().cellsize());
    // setup the fire spread grid
    mGrid.setup(mRUGrid.metricRect(), cellsize());
    mGrid.initialize(0.f);

    // get the top node of settings for the fire module
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.fire"));
    double avg_fire_size = xml.valueDouble(".avgFireSize", 100.);
    double return_interval = xml.valueDouble(".fireReturnInterval", 100); // every x year
    if (avg_fire_size * return_interval == 0.)
        throw IException("Fire-setup: invalid values for 'avgFireSize' or 'fireReturnInterval' (values must not be 0).");
    double p_base = 1. / return_interval;
    mBaseIgnitionProb = p_base * cellsize()*cellsize() / avg_fire_size;
    // setup of the visualization of the grid
    GlobalSettings::instance()->controller()->addLayers(&mFireLayers, "fire");
    GlobalSettings::instance()->controller()->addGrid(&mGrid, "fire spread", GridViewGray,0., 1.);


}

void FireModule::setup(const ResourceUnit *ru)
{
    if (mRUGrid.isEmpty())
        throw IException("FireModule: grid not properly setup!");
    FireRUData &fire_data = data(ru);
    fire_data.setup();
}

/** yearBegin is called at the beginnig of every year.
    do some cleanup here.
  */
void FireModule::yearBegin()
{
for (FireRUData *fd = mRUGrid.begin(); fd!=mRUGrid.end(); ++fd)
    fd->reset(); // reset drought index
}

/** main function of the fire module.

*/
void FireModule::run()
{
    // ignition() calculates ignition and calls 'spread()' if a new fire is created.
    ignition();
}


/** perform the calculation of the KBDI drought index.
    see http://iland.boku.ac.at/wildfire#fire_ignition
  */
void FireModule::calculateDroughtIndex(const ResourceUnit *resource_unit, const WaterCycleData *water_data)
{
    FireRUData &fire_data = data(resource_unit);
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
        // * the temperature > 10�
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


/** evaluates the probability that a fire starts for each cell (20x20m)
    see http://iland.boku.ac.at/wildfire#fire_ignition

*/
void FireModule::ignition()
{
    // calculate base probability for a 20x20m cell:
    double odds_base = mBaseIgnitionProb / (1. - mBaseIgnitionProb);
    int cells_per_ru = (cRUSize / cellsize()) * (cRUSize / cellsize()); // number of fire cells per resource unit

    for (FireRUData *fd = mRUGrid.begin(); fd!=mRUGrid.end(); ++fd)
        if (fd->enabled() && fd->kbdi()>0.) {
            // calculate the probability that a fire ignites within this resource unit
            // the climate factor is the current drought index relative to the reference drought index
            double r_climate = fd->mKBDI / fd->mKBDIref;
            double odds = odds_base * r_climate / fd->mRefMgmt;
            // p_cell is the ignition probability for one 20x20m cell
            double p_cell = odds / (1. + odds);
            if (!p_cell)
                continue;
            for (int i=0;i<cells_per_ru;++i) {
                double p = drandom();
                if (p < p_cell) {
                    // We have a fire event on the particular pixel
                    // get the actual pixel...
                    int ix = i % (int((cRUSize / cellsize())));
                    int iy = i / (int((cRUSize / cellsize())));
                    QPointF startcoord = mRUGrid.cellRect(mRUGrid.indexOf(fd)).bottomLeft();
                    QPoint startpoint = mGrid.indexAt(QPointF(startcoord.x() + ix*cellsize() + 1., startcoord.y() + iy*cellsize() + 1.));

                    // check if we have enough fuel to start the fire: TODO

                    // now start the fire!!!
                    spread(startpoint);

                    // TODO: what after a fire happened? stop at all? or only for the resource unit?
                }
            }
        }

}

/** calculate the actual fire spread.
*/
void FireModule::spread(const QPoint &start_point)
{
    qDebug() << "fire event starting at position" << start_point;
    mGrid.valueAtIndex(start_point) = 1.f;
}


//*********************************************************************************
//******************************************** FireData ***************************
//*********************************************************************************

void FireRUData::setup()
{
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.fire"));
    mKBDIref = xml.valueDouble(".KBDIref", 300.);
    mRefMgmt = xml.valueDouble(".rFireSuppression", 1.);
    mRefAnnualPrecipitation = xml.valueDouble(".meanAnnualPrecipitation", -1);
}

//*********************************************************************************
//****************************************** FireLayers ***************************
//*********************************************************************************


double FireLayers::value(const FireRUData &data, const int param_index) const
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







