#ifndef FIREMODULE_H
#define FIREMODULE_H

#include "grid.h"
#include "layeredgrid.h"

class WaterCycleData;
class ResourceUnit;
class FireModule;

/** FireRUData contains data items for resource units.
    Data items include:
    * parameters (KBDIref, ...)
    * fuel values
*/
class FireRUData
{
public:
    FireRUData(): mKBDIref(0.), mRefMgmt(0.), mRefAnnualPrecipitation(0.), mKBDI(0.) {}
    void setup();
    bool enabled() const { return mRefMgmt>0.; }
    void reset() { mKBDI = 0.; }
    double kbdi() const { return mKBDI; }
    // access data
private:
    // parameters
    double mKBDIref; ///< reference value for KBDI drought index
    double mRefMgmt; ///< r_mgmt (fire suppression value)
    double mRefAnnualPrecipitation; ///< mean annual precipitation (mm)
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
    See http://iland.boku.ac.at/wildfire
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
    void spread(const QPoint &start_point);
    void severity();

private:
    /// estimate fire size from a distribution
    double calculateFireSize();
    const double cellsize() const { return 20.; }

    // functions for the cellular automata
    void probabilisticSpread(const QPoint &start_point);
    /// calculates the probabibilty of spreading the fire from \p pixel_from to \p pixel_to.
    /// the \p direction provides encodes the cardinal direction.
    void calculateSpreadProbability(const float *pixel_from, float *pixel_to, const int direction);

    // data
    Grid<FireRUData> mRUGrid;
    Grid<float> mGrid;
    FireLayers mFireLayers;
    double mBaseIgnitionProb; ///< ignition probabilty for r_climate = r_mgmt = 1
    // functions
    FireRUData &data(const ResourceUnit *ru); ///< get ref to data element (FireData)

};

#endif // FIREMODULE_H
