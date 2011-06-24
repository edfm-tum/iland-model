#include "firemodule.h"

#include "grid.h"
#include "model.h"
#include "modelcontroller.h"
#include "globalsettings.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "climate.h"
#include "dem.h"

//*********************************************************************************
//******************************************** FireData ***************************
//*********************************************************************************

void FireRUData::setup()
{
    // data items loaded here are provided per resource unit
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.fire"));
    mKBDIref = xml.valueDouble(".KBDIref", 300.);
    mRefMgmt = xml.valueDouble(".rFireSuppression", 1.);
    mRefLand = xml.valueDouble(".rLand", 1.);
    mRefAnnualPrecipitation = xml.valueDouble(".meanAnnualPrecipitation", -1);
    mAverageFireSize = xml.valueDouble(".avgFireSize", 100.);
    mFireReturnInterval =  xml.valueDouble(".fireReturnInterval", 100); // every x year
    if (mAverageFireSize * mFireReturnInterval == 0.)
        throw IException("Fire-setup: invalid values for 'avgFireSize' or 'fireReturnInterval' (values must not be 0).");
    double p_base = 1. / mFireReturnInterval;
    // calculate the base ignition probabiility for a cell (eg 20x20m)
    mBaseIgnitionProb = p_base * FireModule::cellsize()*FireModule::cellsize() / mAverageFireSize;
    mFireExtinctionProb = xml.valueDouble(".fireExtinctionProbability", 0.);

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

//*********************************************************************************
//****************************************** FireModule ***************************
//*********************************************************************************



FireModule::FireModule()
{
    mFireLayers.setGrid(mRUGrid);
    mWindSpeedMin=10.;mWindSpeedMax=10.;
    mWindDirection=45.;
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

    // set some global settings
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.fire"));
    mWindSpeedMin = xml.valueDouble(".wind.speedMin", 5.);
    mWindSpeedMax = xml.valueDouble(".wind.speedMax", 10.);
    mWindDirection = xml.valueDouble(".wind.direction", 270.); // defaults to "west"

    // setup of the visualization of the grid
    GlobalSettings::instance()->controller()->addLayers(&mFireLayers, "fire");
    GlobalSettings::instance()->controller()->addGrid(&mGrid, "fire spread", GridViewRainbow,0., 50.);

    // check if we have a DEM in the system
    if (!GlobalSettings::instance()->model()->dem())
        throw IException("FireModule:setup: a digital elevation model is required for the fire module!");
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
    double kbdi = 100.; // starting value of the year
    const double mean_ap = fire_data.mRefAnnualPrecipitation; // reference mean annual precipitation
    double dp, dq, tmax;

    double kbdi_sum = 0.;
    for (const ClimateDay *day = resource_unit->climate()->begin(); day!=end; ++day, ++iday) {
        dp = water_data->water_to_ground[iday]; // water reaching the ground for this day
        double wetting = - dp/25.4 * 100.;
        kbdi += wetting;
        if (kbdi<0.) kbdi=0.;

        tmax = day->max_temperature;
        // drying is only simulated, if:
        // * the temperature > 10°
        // * there is no snow cover
        if (tmax > 10. && water_data->snow_cover[iday]==0.) {
            // calculate drying: (kbdi already includes current wetting!)
            dq = 0.001*(800.-kbdi)*( (0.9676*exp(0.0486*(tmax*9./5.+32.))-8.299) / (1 + 10.88*exp(-0.0441*mean_ap/25.4)) );

            kbdi += dq;
        }
        kbdi_sum += kbdi;
    }
    // the effective relative KBDI is calculated
    // as the year sum related to the maximum value (800*365)
    fire_data.mKBDI = kbdi_sum / (365. * 800.);
}


/** evaluates the probability that a fire starts for each cell (20x20m)
    see http://iland.boku.ac.at/wildfire#fire_ignition

*/
void FireModule::ignition()
{

    int cells_per_ru = (cRUSize / cellsize()) * (cRUSize / cellsize()); // number of fire cells per resource unit

    for (FireRUData *fd = mRUGrid.begin(); fd!=mRUGrid.end(); ++fd)
        if (fd->enabled() && fd->kbdi()>0.) {
            // calculate the probability that a fire ignites within this resource unit
            // the climate factor is the current drought index relative to the reference drought index
            double odds_base = fd->mBaseIgnitionProb / (1. - fd->mBaseIgnitionProb);
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

                    // TODO: what happens after a fire event? stop at all? or only for the resource unit?
                }
            }
        }

}

/** calculate the actual fire spread.
*/
void FireModule::spread(const QPoint &start_point)
{
    qDebug() << "fire event starting at position" << start_point;

    mGrid.initialize(0.f);
    mGrid.valueAtIndex(start_point) = 1.f;

    // randomly choose windspeed and wind direction
    mCurrentWindSpeed = nrandom(mWindSpeedMin, mWindSpeedMax);
    mCurrentWindDirection = fmod(mWindDirection + nrandom(-45., 45.)+360., 360.);

    // choose spread algorithm
    probabilisticSpread(start_point);

}

/// Estimate fire size (m2) from a fire size distribution.
double FireModule::calculateFireSize()
{
    return 100000.; // TODO implement
}

/// calculate effect of slope on fire spread
/// for upslope following Keene and Albini 1976
///  It was designed by RKeane (2/2/99) (calc.c)
/// the downslope function is "not based on empirical data" (Keane in calc.c)
/// return is the metric distance to spread (and not number of pixels)
double FireModule::calcSlopeFactor(const double slope) const
{
    double slopespread;       /* Slope spread rate in pixels / timestep   */
    static double firebgc_cellsize = 30.; /* cellsize for which this functions were originally designed */

    if (slope < 0.) {
        // downslope effect
        slopespread = 1.0 - ( 20.0 * slope * slope );

    } else {
        // upslope effect
        static double alpha = 4.0; /* Maximum number of pixels to spread      */
        static double beta  = 3.5; /* Scaling coeff for inflection point      */
        static double gamma = 10.0;/* Scaling coeff for graph steepness       */
        static double zeta  = 0.0; /* Scaling coeff for y intercept           */

        slopespread = zeta + ( alpha / ( 1.0 + ( beta * exp( -gamma * slope ) ) ) );
    }


    return( slopespread ) * firebgc_cellsize;

}

/// calculate the effect of wind on the spread.
/// function designed by R. Keane, 2/2/99
/// @param direction direction (in degrees) of spread (0=north, 90=east, ...)
/// @return spread (in meters)
double FireModule::calcWindFactor(const double direction) const
{
    const double firebgc_cellsize = 30.; /* cellsize for which this functions were originally designed */
    double windspread;         /* Wind spread rate in pixels / timestep   */
    double coeff;              /* Coefficient that reflects wind direction*/
    double lwr;                /* Length to width ratio                   */
    const double alpha = 0.6; /* Wind spread power coeffieicnt           */
    const double MPStoMPH = 1. / 0.44704;

    /* .... If zero wind speed return 1.0 for the factor .... */
    if ( mCurrentWindSpeed <= 0.5 )
        return ( 1.0 ) * firebgc_cellsize; // not 0????

    /* .... Change degrees to radians .... */
    coeff = fabs( direction - mCurrentWindDirection ) * M_PI/180.;

    /* .... If spread direction equal zero, then spread direction = wind direct */
    if ( direction <= 0.01 )
        coeff = 0.0;

    /* .... Compute the length:width ratio from Andrews (1986) .....  */

    lwr = 1.0 + ( 0.125 * mCurrentWindSpeed * MPStoMPH );

    /* .... Scale the difference between direction between 0 and 1.0 .....  */
    coeff = ( cos( coeff ) + 1.0 ) / 2.0;

    /* .... Scale the function based on windspeed between 1 and 10...  */
    windspread = pow( coeff, pow( (mCurrentWindSpeed * MPStoMPH ), alpha ) ) * lwr;

    return( windspread ) * firebgc_cellsize;

}



void FireModule::calculateSpreadProbability(const FireRUData &fire_data, const double height, const float *pixel_from, float *pixel_to, const int direction)
{
    const double directions[8]= {0., 90., 180., 270., 45., 135., 225., 315. };
    // TODO implement
    double p;
    p = 0.4;
    if (direction>2)
        p = 0.85;

    double spread_metric; // distance that fire supposedly spreads

    // calculate the slope from the curent point (pixel_from) to the spreading cell (pixel_to)
    double h_to = GlobalSettings::instance()->model()->dem()->elevation(mGrid.cellCenterPoint(mGrid.indexOf(pixel_to)));
    if (h_to==-1) {
        qDebug() << "invalid elevation for pixel during fire spread: " << mGrid.cellCenterPoint(mGrid.indexOf(pixel_to));
        return;
    }
    double slope = (h_to - height) / (cellsize());
    // if we spread diagonal, the distance is longer:
    if (direction>4)
        slope /= 1.41421356;

    double r_wind, r_slope; // metric distance for spread
    r_slope = calcSlopeFactor( slope ); // slope factor (upslope / downslope)

    r_wind = calcWindFactor(directions[direction]); // metric distance from wind

    spread_metric = r_slope + r_wind;

    double spread_pixels = spread_metric / cellsize();
    if (spread_pixels==0.)
        return;

    double p_spread = pow(0.5, 1. / spread_pixels);
    // apply the r_land factor that accounts for different land types
    p_spread *= fire_data.mRefLand;
    // add probabilites
    *pixel_to = 1. - (1. - *pixel_to)*(1. - p_spread);

}

/** a cellular automaton spread algorithm.
*/
void FireModule::probabilisticSpread(const QPoint &start_point)
{
    QRect max_spread = QRect(start_point, start_point);
    // grow the rectangle by one row/column but ensure validity
    max_spread.setCoords(qMax(start_point.x()-1,0),qMax(start_point.y()-1,0),
                         qMin(start_point.x()+2,mGrid.sizeX()),qMin(start_point.y()+2,mGrid.sizeY()) );
    double fire_size_m2 = calculateFireSize();
    int cells_to_burn = fire_size_m2 / (cellsize() * cellsize());
    int cells_burned = 1;
    int iterations = 1;
    // main loop
    float *neighbor[8];
    float *p;
    while (cells_burned < cells_to_burn) {
        // scan the current spread area
        // and calcuate for each pixel the probability of spread from a burning
        // pixel to a non-burning pixel
        GridRunner<float> runner(mGrid, max_spread);
        while ((p = runner.next())) {
            if (*p == 1.f) {
                const QPointF pt = mGrid.cellCenterPoint(mGrid.indexOf(p));
                const FireRUData &fire_data = mRUGrid.constValueAt(pt);
                double h = GlobalSettings::instance()->model()->dem()->elevation(pt);
                if (h==-1)
                    throw IException(QString("Fire-Spread: invalid elevation at %1/%2.").arg(pt.x()).arg(pt.y()));

                // current cell is burning.
                // check the neighbors: get an array with neigbors
                // north, east, west, south
                // NE/NW/SE/SW
                runner.neighbors8(neighbor);
                if (neighbor[0] && *(neighbor[0])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[0], 1);
                if (neighbor[1] && *(neighbor[1])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[1], 2);
                if (neighbor[2] && *(neighbor[2])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[2], 3);
                if (neighbor[3] && *(neighbor[3])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[3], 4);
                if (neighbor[4] && *(neighbor[4])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[4], 5);
                if (neighbor[5] && *(neighbor[5])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[5], 6);
                if (neighbor[3] && *(neighbor[6])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[6], 7);
                if (neighbor[3] && *(neighbor[7])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[7], 8);
                *p = iterations + 1;
            }
        }
        // now draw random numbers and calculate the real spread
        runner.reset();
        while ((p = runner.next())) {
            if (*p<1.f && *p>0.f) {
                if (drandom() < *p) {
                    // the fire spreads:
                    *p = 1.f;
                    cells_burned++;
                    const FireRUData &fire_data = mRUGrid.constValueAt(mGrid.cellCenterPoint(mGrid.indexOf(p)));
                    if (fire_data.mFireExtinctionProb>0.) {
                        if (drandom() < fire_data.mFireExtinctionProb) {
                            *p = iterations + 1;
                        }
                    }
                }
            }
        }
        // now determine the maximum extent with burning pixels...
        runner.reset();
        int left = mGrid.sizeX(), right = 0, top = mGrid.sizeY(), bottom = 0;
        while ((p = runner.next())) {
            if (*p == 1.f) {
                QPoint pt = mGrid.indexOf(p);
                left = qMin(left, pt.x()-1);
                right = qMax(right, pt.x()+2);
                top = qMin(top, pt.y()-1);
                bottom = qMax(bottom, pt.y()+2);
            }
        }
        max_spread.setCoords(left, top, right, bottom);
        max_spread = max_spread.intersected(QRect(0,0,mGrid.sizeX(), mGrid.sizeY()));


        qDebug() << "Iter: " << iterations << "totalburned:" << cells_burned << "spread-rect:" << max_spread;
        iterations++;
        if (iterations > 1000) {
            qDebug() << "Firespread: maximum number of iterations reached!";
            break;
        }
    }
    qDebug() << "probabilstic spread: used " << iterations << "iterations found" << cells_burned << "burning pixels";
}

void FireModule::testSpread()
{

}









