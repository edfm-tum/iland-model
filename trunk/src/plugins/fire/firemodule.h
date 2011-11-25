/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef FIREMODULE_H
#define FIREMODULE_H

#include "grid.h"
#include "layeredgrid.h"
#include "expression.h"

class WaterCycleData;
class ResourceUnit;
class FireModule;
class FireOut;

/** FireRUData contains data items for resource units.
    Data items include:
    * parameters (KBDIref, ...)
    * fuel values
*/
class FireRUData
{
public:
    FireRUData(): mKBDIref(0.), mRefMgmt(0.), mRefAnnualPrecipitation(0.), mKBDI(0.) { fireRUStats.clear(); }
    void setup();
    bool enabled() const { return mRefMgmt>0.; }
    void reset() { mKBDI = 0.; }
    double kbdi() const { return mKBDI; }
    // access data
    struct {
        int fire_id;
        // statistics for:
        int n_trees_died; ///< number of trees that are killed
        int n_trees; ///< number of trees that are on burning cells
        int n_cells; ///< number of burning cells
        double died_basal_area; ///< basal area (m2) of died trees
        double basal_area; ///< basal area (m2) of all trees on burning pixels
        double fuel_ff; ///< average fuel fine material (kg/ha)
        double fuel_dwd; ///< average fuel dead wood (kg/ha)
        double crown_kill; ///< average crown kill percent
        double avg_dbh; ///< average dbh (arithmetic mean) and limited to threshold
        // enter() can be called multiple times
        void enter(const int this_fire_id) {
            if (fire_id!=this_fire_id) {
                fire_id = this_fire_id;
                // clear all stats
                n_trees_died = n_trees = n_cells = 0;
                died_basal_area = basal_area = fuel_ff = fuel_dwd = crown_kill = avg_dbh = 0.;
            }
        }
        // call once after fire is finished
        void calculate(const int this_fire_id) {
            if (fire_id==this_fire_id) {
                // calculate averages
                if (n_cells>0) {
                    crown_kill /= double(n_cells);
                    fuel_ff /= double(n_cells);
                    fuel_dwd /= double(n_cells);
                    avg_dbh /= double(n_cells);
                }
            }
        }
        void clear() { fire_id=-1; enter(0);}
    } fireRUStats;
private:
    // parameters
    double mKBDIref; ///< reference value for KBDI drought index
    double mRefMgmt; ///< r_mgmt (fire suppression value)
    double mRefLand; ///< fixed multiplier for the fire spread probabilites (e.g. for riparian land) [0..1], default 1
    double mRefAnnualPrecipitation; ///< mean annual precipitation (mm)
    double mFireReturnInterval; ///< mean fire return interval (yrs)
    double mAverageFireSize; ///< mean average fire size (m2)
    double mMinFireSize; ///< minimum fire size (m2)
    double mMaxFireSize; ///< maximum fire size (m2)
    double mBaseIgnitionProb; ///< ignition probabilty for r_climate = r_mgmt = 1 (value is for the prob. for a cell, e.g. 20x20m)
    double mFireExtinctionProb; ///< gives the probabilty that a fire extincts on a pixel without having a chance to spread further
    // variables
    double mKBDI; ///< Keetch Byram Drought Index (0..800, in 1/100 inch of water)

    friend class FireModule; // allow access to member values
    friend class FireLayers;
};

class FireLayers: public LayeredGrid<FireRUData> {
  public:
    void setGrid(const Grid<FireRUData> &grid) { mGrid = &grid; }
    double value(const FireRUData& data, const int index) const;
    const QStringList names() const;
};
/** FireModule is the main class of the fire sub module and
    holds all the relevant data/actions for the iLand fire module.
    See http://iland.boku.ac.at/wildfire and http://iland.boku.ac.at/fire+module

    The fire module has conceptually three parts that stand more or less on its own:
     * Fire ignition
     * Fire spread
     * Fire severity/effect
*/
class FireModule
{
public:
    FireModule();
    /// the setup function sets up the grids and parameters used for the fire module.
    /// this should be called when the main model is already created.
    void setup(); ///< general setup
    void setup(const ResourceUnit *ru); ///< setup for a specific resource unit
    static const double cellsize() { return 20.; }

    // actions
    /// main function that is executed yearly (called by the plugin)
    /// performs the major processes (ignition, spread, fire effect)
    void run();
    /// called yearly from the plugin to perform some
    /// cleanup.
    void yearBegin();
    /// called from the plugin to perform calculations (drought indices)
    /// during the water cycle routine.
    void calculateDroughtIndex(const ResourceUnit *resource_unit, const WaterCycleData *water_data);

    // main fire functions
    /// calculate the start and starting point of a possible fire
    void ignition();
    ///  spread a fire starting from 'start_point' (index of the 20m grid)
    void spread(const QPoint &start_point, const bool prescribed = false);
    void severity();

    // helper functions
    int fireId() const { return mFireId; } ///< return the ID of the last fire event
    void testSpread();
    void prescribedIgnition(const double x_m, const double y_m, const double firesize, const double windspeed, const double winddirection);

private:
    /// estimate fire size from a distribution
    double calculateFireSize(const FireRUData *data);

    // functions for the cellular automata
    void probabilisticSpread(const QPoint &start_point);
    /// calculates the probabibilty of spreading the fire from \p pixel_from to \p pixel_to.
    /// the \p direction provides encodes the cardinal direction.
    void calculateSpreadProbability(const FireRUData &fire_data, const double height, const float *pixel_from, float *pixel_to, const int direction);

    /// calc the effect of slope on the fire spread
    double calcSlopeFactor(const double slope) const;

    /// calc the effect of wind on the fire spread
    double calcWindFactor(const double direction) const;

    /// calculate the "severity", i.e. burn individual trees within the pixels
    bool burnPixel(const QPoint &pos, FireRUData &ru_data);
    /// calculate statistics, burn snags, soil of the resource units
    void afterFire();

    int mFireId; ///< running id of a fire event
    // parameters
    double mFireSizeSigma; ///< parameter of the log-normal distribution to derive fire size
    double mWindSpeedMin;
    double mWindSpeedMax;
    double mWindDirection;
    double mCurrentWindSpeed;
    double mCurrentWindDirection;
    double mPrescribedFiresize; // fire size from javascript
    // fuel parameters
    double mFuelkFC1; ///< params (Schumacher 2006) to calculate amount of fuel
    double mFuelkFC2;
    double mFuelkFC3;
    // crown kill
    double mCrownKillkCK1; ///< parameter to calculate fraction of crowns killed by the fire (Schumacher)
    double mCrownKillkCK2;
    double mCrownKillDbh; ///< dbh threshold (cm) for crown kill calculation
    Expression mMortalityFormula; ///< formula to calculate mortality caused by fire
    double *mFormula_bt;
    double *mFormula_ck;
    double mBurnSoilBiomass; ///< fraction of soil biomass that is to be removed when burning
    double mBurnStemFraction; ///< fraction of stem biomass burned by fire (if a tree dies)
    double mBurnBranchFraction; ///< fraction of branch biomass burned by fire (if a tree dies)
    double mBurnFoliageFraction; ///< fraction of foliage biomass burned by fire (if a tree dies)

    bool mOnlyFireSimulation; ///< if true, trees/snags etc. are not really affected by fire
    // event handler
    QString mAfterFireEvent; ///< javascript event after fire

    // data
    Grid<FireRUData> mRUGrid; ///< grid with data values per resource unit
    Grid<float> mGrid; ///< fire grid (20x20m)
    FireLayers mFireLayers;
    // functions
    FireRUData &data(const ResourceUnit *ru); ///< get ref to data element (FireData)

    // statistics
    struct {
        int iterations;
        int fire_size_plan_m2;
        int fire_size_realized_m2;
        double fire_psme_total; ///< psme (doug fir) on burning pixels (m2 basal area)
        double fire_psme_died; ///<  psme (doug fir) that died during the fire (based on m2)
        QPointF startpoint;
    } fireStats;
    friend class FireOut;
    friend class FireScript;

};

#endif // FIREMODULE_H
