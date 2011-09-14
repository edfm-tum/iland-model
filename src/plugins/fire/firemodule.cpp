#include "firemodule.h"

#include "grid.h"
#include "model.h"
#include "modelcontroller.h"
#include "globalsettings.h"
#include "resourceunit.h"
#include "watercycle.h"
#include "climate.h"
#include "soil.h"
#include "dem.h"
#include "outputmanager.h"
#include "3rdparty/SimpleRNG.h"

//*********************************************************************************
//******************************************** FireData ***************************
//*********************************************************************************

void FireRUData::setup()
{
    // data items loaded here are provided per resource unit
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.fire"));
    mKBDIref = xml.valueDouble(".KBDIref", 0.3);
    mRefMgmt = xml.valueDouble(".rFireSuppression", 1.);
    mRefLand = xml.valueDouble(".rLand", 1.);
    mRefAnnualPrecipitation = xml.valueDouble(".meanAnnualPrecipitation", -1);
    mAverageFireSize = xml.valueDouble(".averageFireSize", 100.);
    mFireReturnInterval =  xml.valueDouble(".fireReturnInterval", 100); // every x year
    if (mAverageFireSize * mFireReturnInterval == 0.)
        throw IException("Fire-setup: invalid values for 'averageFireSize' or 'fireReturnInterval' (values must not be 0).");
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
    case 2: return data.fireRUStats.fire_id; // the ID of the last recorded fire
    case 3: return data.fireRUStats.crown_kill; // crown kill fraction (average on resource unit)
    case 4: return data.fireRUStats.died_basal_area; // basal area died in the last fire
    case 5: return data.fireRUStats.n_trees > 0 ? data.fireRUStats.n_trees_died / double(data.fireRUStats.n_trees) : 0.;
    case 6: return data.fireRUStats.fuel; // fuel load (forest floor + dwd) kg/ha
    default: throw IException(QString("invalid variable index for FireData: %1").arg(param_index));
    }
}

const QStringList FireLayers::names() const
{
    return QStringList() <<  "KBDI" << "KBDIref" << "fireID" << "crownKill" << "diedBasalArea" << "diedStemsFrac" << "fuel";

}

//*********************************************************************************
//****************************************** FireModule ***************************
//*********************************************************************************



FireModule::FireModule()
{
    mFireLayers.setGrid(mRUGrid);
    mWindSpeedMin=10.;mWindSpeedMax=10.;
    mWindDirection=45.;
    mFireId = 0;
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
    mFireSizeSigma = xml.valueDouble(".fireSizeSigma", 0.25);

    // fuel parameters
    mFuelkFC1 = xml.valueDouble(".fuelkFC1", 0.8);
    mFuelkFC2 = xml.valueDouble(".fuelkFC2", 0.2);
    mFuelkFC3 = xml.valueDouble(".fuelkFC3", 0.4);

    // parameters for crown kill
    mCrownKillkCK1 = xml.valueDouble(".crownKill1", 0.21111);
    mCrownKillkCK2 = xml.valueDouble(".crownKill2", 0.00445);
    mCrownKillDbh = xml.valueDouble(".crownKillDbh", 40.);

    QString formula = xml.value(".mortalityFormula", "1/(1 + exp(-1.466 + 1.91*bt - 0.1775*bt*bt - 5.41*ck*ck))");
    mFormula_bt = mMortalityFormula.addVar("bt");
    mFormula_ck = mMortalityFormula.addVar("ck");
    mMortalityFormula.setExpression(formula);

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
                    fireStats.startpoint = startcoord;
                    QPoint startpoint = mGrid.indexAt(QPointF(startcoord.x() + ix*cellsize() + 1., startcoord.y() + iy*cellsize() + 1.));

                    // check if we have enough fuel to start the fire: TODO

                    // now start the fire!!!
                    mFireId++; // this fire gets a new id
                    spread(startpoint);

                    // TODO: what happens after a fire event? stop at all? or only for the resource unit?

                    // finalize statistics after fire event
                    for (FireRUData *fds = mRUGrid.begin(); fds!=mRUGrid.end(); ++fds)
                        fds->fireRUStats.calculate(mFireId);

                    // provide outputs: This calls the FireOut::exec() function
                    GlobalSettings::instance()->outputManager()->execute("fire");
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
double FireModule::calculateFireSize(const double average_fire_size)
{
    SimpleRNG rng;
    rng.SetState(irandom(0, std::numeric_limits<unsigned int>::max()-1), irandom(0, std::numeric_limits<unsigned int>::max()-1));
    double size = rng.GetLogNormal(log(average_fire_size), mFireSizeSigma);
    return size;
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


/** calculates probability of spread from one pixel to one neighbor.
    In this functions the effect of the terrain, the wind and others are used to estimate a probability.
    @param fire_data reference to the variables valid for the current resource unit
    @param height elevation (m) of the origin point
    @param pixel_from pointer to the origin point in the fire grid
    @param pixel_to pointer to the target pixel
    @param direction codes the direction from the origin point (1..8, N, E, S, W, NE, SE, SW, NW)
  */
void FireModule::calculateSpreadProbability(const FireRUData &fire_data, const double height, const float *pixel_from, float *pixel_to, const int direction)
{
    const double directions[8]= {0., 90., 180., 270., 45., 135., 225., 315. };

    double spread_metric; // distance that fire supposedly spreads

    // calculate the slope from the curent point (pixel_from) to the spreading cell (pixel_to)
    double h_to = GlobalSettings::instance()->model()->dem()->elevation(mGrid.cellCenterPoint(mGrid.indexOf(pixel_to)));
    if (h_to==-1) {
        qDebug() << "invalid elevation for pixel during fire spread: " << mGrid.cellCenterPoint(mGrid.indexOf(pixel_to));
        return;
    }
    double pixel_size = cellsize();
    // if we spread diagonal, the distance is longer:
    if (direction>4)
        pixel_size *= 1.41421356;

    double slope = (h_to - height) / pixel_size;

    double r_wind, r_slope; // metric distance for spread
    r_slope = calcSlopeFactor( slope ); // slope factor (upslope / downslope)

    r_wind = calcWindFactor(directions[direction-1]); // metric distance from wind

    spread_metric = r_slope + r_wind;

    double spread_pixels = spread_metric / pixel_size;
    if (spread_pixels<=0.)
        return;

    double p_spread = pow(0.5, 1. / spread_pixels);
    // apply the r_land factor that accounts for different land types
    p_spread *= fire_data.mRefLand;
    // add probabilites
    *pixel_to = 1. - (1. - *pixel_to)*(1. - p_spread);

}

/** a cellular automaton spread algorithm.
    @param start_point the starting point of the fire spread as index of the fire grid
*/
void FireModule::probabilisticSpread(const QPoint &start_point)
{
    QRect max_spread = QRect(start_point, start_point);
    // grow the rectangle by one row/column but ensure validity
    max_spread.setCoords(qMax(start_point.x()-1,0),qMax(start_point.y()-1,0),
                         qMin(start_point.x()+2,mGrid.sizeX()),qMin(start_point.y()+2,mGrid.sizeY()) );

    FireRUData *rudata = &mRUGrid.valueAt( mGrid.cellCenterPoint(start_point) );
    double fire_size_m2 = calculateFireSize(rudata->mAverageFireSize);
    fireStats.fire_size_plan_m2 = fire_size_m2;
    fireStats.iterations = 0;
    fireStats.fire_size_realized_m2 = 0;
    double sum_fire_size = fire_size_m2; // cumulative fire size
    // calculate a factor describing how much larger/smaller the selected fire is compared to the average
    // fire size of the ignition cell
    double fire_scale_factor = fire_size_m2 / rudata->mAverageFireSize;

    int cells_to_burn = fire_size_m2 / (cellsize() * cellsize());
    int cells_burned = 1;
    int last_round_burned = cells_burned;
    int iterations = 1;
    // main loop
    float *neighbor[8];
    float *p;

    rudata->fireRUStats.enter(mFireId);
    if (!burnPixel(start_point, *rudata)) {
        // no fuel / no trees on the starting pixel
        return;
    }
    while (cells_burned < cells_to_burn) {
        // scan the current spread area
        // and calcuate for each pixel the probability of spread from a burning
        // pixel to a non-burning pixel
        GridRunner<float> runner(mGrid, max_spread);
        while ((p = runner.next())) {
            if (*p == 1.f) {
                const QPointF pt = mGrid.cellCenterPoint(mGrid.indexOf(p));
                FireRUData &fire_data = mRUGrid.valueAt(pt);
                fire_data.fireRUStats.enter(mFireId); // setup/clear statistics if this is the first pixel in the resource unit
                double h = GlobalSettings::instance()->model()->dem()->elevation(pt);
                if (h==-1)
                    throw IException(QString("Fire-Spread: invalid elevation at %1/%2.").arg(pt.x()).arg(pt.y()));

                // current cell is burning.
                // check the neighbors: get an array with neighbors
                // 1-4: north, east, west, south
                // 5-8: NE/NW/SE/SW
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
                if (neighbor[6] && *(neighbor[6])<1.f)
                    calculateSpreadProbability(fire_data, h, p, neighbor[6], 7);
                if (neighbor[7] && *(neighbor[7])<1.f)
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
                    FireRUData &fire_data = mRUGrid.valueAt(mGrid.cellCenterPoint(mGrid.indexOf(p)));
                    cells_burned++;
                    // do the severity calculations:
                    // the function returns false if no trees are on the pixel
                    bool really_burnt = burnPixel(mGrid.indexOf(p), fire_data);
                    // update the fire size
                    sum_fire_size += fire_data.mAverageFireSize * fire_scale_factor;
                    if (!really_burnt || fire_data.mFireExtinctionProb>0.) {
                        if (!really_burnt || drandom() < fire_data.mFireExtinctionProb) {
                            *p = iterations + 1;
                        }
                    }
                } else {
                    *p = 0.f; // if the fire does note spread to the cell, the value is cleared again.
                }
            }
        }

        cells_to_burn = (sum_fire_size/(double)cells_burned) / (cellsize() * cellsize());
        if (cells_to_burn <= cells_burned)
            break;

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


        //qDebug() << "Iter: " << iterations << "totalburned:" << cells_burned << "spread-rect:" << max_spread << "cells to burn" << cells_to_burn;
        iterations++;
        if (last_round_burned == cells_burned) {
            qDebug() << "Firespread: a round without new burning cells - exiting!";
            break;
        }
        last_round_burned = cells_burned;
        if (iterations > 10000) {
            qDebug() << "Firespread: maximum number of iterations (10000) reached!";
            break;
        }
    }
    qDebug() << "Fire:probabilstic spread: used " << iterations
             << "iterations. Planned (m2/cells):" << fire_size_m2 << "/" << cells_to_burn
             << "burned (m2/cells):" << cells_burned*cellsize()*cellsize() << "/" << cells_burned;

    fireStats.fire_size_realized_m2 = cells_burned*cellsize()*cellsize();

}

void FireModule::testSpread()
{
//    QPoint pt = mGrid.indexAt(QPointF(1000., 600.));
//    spread( pt );
    SimpleRNG rng;
    rng.SetState(irandom(0, std::numeric_limits<unsigned int>::max()-1), irandom(0, std::numeric_limits<unsigned int>::max()-1));
    int bins[20];
    for(int i=0;i<20;i++) bins[i]=0;
    for (int i=0;i<10000;i++) {
        double value = rng.GetLogNormal(log(2000.),0.25);
        if (value>=0 && value<10000.)
            bins[(int)(value/500.)]++;
    }
    for(int i=0;i<20;i++)
        qDebug() << bins[i];

    for (int r=0;r<360;r+=90) {
        mWindDirection = r;
        for (int i=0;i<5;i++) {
            QPoint pt = mGrid.indexAt(QPointF(730., 610.)); // was: 1100/750
            mFireId++; // this fire gets a new id

            spread( pt );
            // stats
            for (FireRUData *fds = mRUGrid.begin(); fds!=mRUGrid.end(); ++fds)
                fds->fireRUStats.calculate(mFireId);

            GlobalSettings::instance()->controller()->repaint();
            GlobalSettings::instance()->controller()->saveScreenshot(GlobalSettings::instance()->path(QString("%1_%2.png").arg(r).arg(i), "temp"));
        }
    }
}


/** burning of a single 20x20m pixel. see http://iland.boku.ac.at/wildfire.
   The function is called from the fire spread function.
   @return boolean true, if any trees were burned on the pixel

  */
bool FireModule::burnPixel(const QPoint &pos, FireRUData &ru_data)
{
    // extract a list of trees that are within the pixel boundaries
    QRectF pixel_rect = mGrid.cellRect(pos);
    ResourceUnit *ru = GlobalSettings::instance()->model()->ru(pixel_rect.center());
    if (!ru)
        return false;

    // retrieve a list of trees within the active pixel
    // NOTE: the check with isDead() is necessary because dead trees could be already in the trees list
    QVector<Tree*> trees;
    QVector<Tree>::iterator tend = ru->trees().end();
    for (QVector<Tree>::iterator t = ru->trees().begin(); t!=tend; ++t) {
        if ( pixel_rect.contains( (*t).position() ) && !(*t).isDead())
            trees.push_back(&(*t));
    }

    if (trees.isEmpty())
        return false;

    // calculate mean values for dbh
    double sum_dbh = 0.;
    foreach (const Tree* t, trees)
        sum_dbh += t->dbh();
    double avg_dbh = sum_dbh / (double) trees.size();

    // (1) calculate fuel
    const double kfc1 = mFuelkFC1;
    const double kfc2 = mFuelkFC2;
    const double kfc3 = mFuelkFC3;
    // retrieve values for fuel.
    // forest_floor: sum of leaves and twigs (t/ha) = yR pool
    // DWD: downed woody debris (t/ha) = yL pool
    const double pixel_fraction = cellsize()*cellsize() / (cRUSize*cRUSize); // fraction of one pixel, default: 0.04 (20x20 / 100x100)

    // fuel in cell (kg biomass): derive fraction of available fuel and use the KBDI as estimate for humidity.
    double fuel_ff = (kfc1 + kfc2*ru_data.kbdi()) * ru->soil()->youngLabile().biomass() * 1000. * pixel_fraction;
    double fuel_dwd = kfc3*ru_data.kbdi() * ru->soil()->youngRefractory().biomass() * 1000. * pixel_fraction;
    // calculate fuel (scaled to kg biomass / ha)
    double fuel = (fuel_ff + fuel_dwd) / pixel_fraction;

    ru_data.fireRUStats.fuel += fuel; // fuel in kg/ha Biomass
    ru_data.fireRUStats.n_trees += trees.size();

    // if fuel level is below 0.05kg BM/m2, then no burning happens
    if (fuel < 500.)
        return false;

    // (2) calculate the "crownkill" fraction
    const double dbh_trehshold = mCrownKillDbh; // dbh
    const double kck1 = mCrownKillkCK1;
    const double kck2 = mCrownKillkCK2;
    if (avg_dbh > dbh_trehshold)
        avg_dbh = dbh_trehshold;

    double crown_kill_fraction = (kck1+kck2*avg_dbh)*fuel/1000.; // fuel: to t/ha
    if (crown_kill_fraction > 1.)
        crown_kill_fraction = 1.;


    // (3) derive mortality of single trees
    double p_mort;
    int died = 0;
    double died_basal_area=0.;
    foreach (Tree* t, trees) {
        // the mortality probability depends on the thickness of the bark:
        *mFormula_bt = t->barkThickness(); // cm
        *mFormula_ck = crown_kill_fraction; // fraction of crown that is killed (0..1)
        p_mort = mMortalityFormula.execute();
        // note: 5.41 = 0.000541*10000, (fraction*fraction) = 10000 * pct*pct
        //p_mort = 1. / (1. + exp(-1.466 + 1.91*bt - 0.1775*bt*bt - 5.41*crown_kill_fraction*crown_kill_fraction));
        if (drandom() < p_mort) {
            // the tree actually dies.
            died_basal_area += t->basalArea();
            t->die();
            ++died;
            // some statistics???
        }
    }

    // update statistics
    ru_data.fireRUStats.n_trees_died += died;
    ru_data.fireRUStats.died_basal_area += died_basal_area;
    ru_data.fireRUStats.crown_kill += crown_kill_fraction;


    // (4) effect of forest fire on the dead wood pools. remove fuel of the current pixel
    ru->soil()->disturbanceBiomass(fuel_dwd, fuel_ff, 0.);

    // (5) effect of forest fire on saplings: all saplings are killed.
    //     Because regeneration happens before the fire routine, any newly regenarated saplings are killed as well.
    ru->clearSaplings(pixel_rect, true);
    return true;
}









