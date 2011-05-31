#ifndef FIREMODULE_H
#define FIREMODULE_H

#include "grid.h"
#include "layeredgrid.h"

class WaterCycleData;
class ResourceUnit;
class FireModule;

/** FireData contains data items for resource units.
    Data items include:
    * parameters (KBDIref, ...)
    * fuel values
*/
class FireData
{
public:
    FireData(): mKBDIref(0.), mRefMgmt(0.), mRefAnnualPrecipitation(0.), mKBDI(0.) {}
    void setup();
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

class FireLayers: public LayeredGrid<FireData> {
  public:
    void setGrid(const Grid<FireData> &grid) { mGrid = &grid; }
    double value(const FireData& data, const int index) const;
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
    void calculateDroughtIndex(const ResourceUnit *resource_unit, const WaterCycleData *water_data);
private:
    // data
    Grid<FireData> mGrid;
    FireLayers mFireLayers;
    // functions
    FireData &data(const ResourceUnit *ru); ///< get ref to data element (FireData)
    void ignition();
    void spread();
    void severity();

};

#endif // FIREMODULE_H
