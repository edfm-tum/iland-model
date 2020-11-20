#ifndef PERMAFROST_H
#define PERMAFROST_H

#include "layeredgrid.h"


class WaterCycle; // forward
struct ClimateDay; // forward
class ResourceUnit; // forward
class WaterOut; // forward

namespace Water {

/**
 * @brief The Permafrost class simulates the permafrost layer of a resource unit
 *
 *
 */
class Permafrost
{
public:
    Permafrost();
    ~Permafrost();
    void setup(WaterCycle *wc);

    //const SStats &stats() const { return stats; }

    /// start a new year
    void newYear();

    /// run the permafrost calculations for a given resource unit and day
    void run(const ClimateDay *clim_day);

    /// add permafrost related debug output to the list 'out'
    void debugData(DebugList &out);



private:
    void setupThermalConductivity();

    /// thermal conductivity of the mineral soil [W / m2 / K]
    double thermalConductivity() const;
    double thermalConductivityFrozen() const;

    struct FTResult {
        FTResult(): delta_mm(0.), delta_soil(0.), new_depth(0.), orig_depth(0.) {}
        double delta_mm; // change of water (mm within iLand water bucket), freezing is negative
        double delta_soil; // change of ice layer (m) (within iLand water bucket), freezing is negative
        double new_depth; // final depth (m)
        double orig_depth;  // starting depth (m)
    };
    FTResult calcFreezeThaw(double at, double temp, bool lowerIceEdge, bool fromAbove);
    FTResult mResult; ///< keep the results of the last run for debug output

    WaterCycle *mWC;
    /// depth of soil (m) that is currently frozen (this is a part of the soil plant accessible soil)
    double mCurrentSoilFrozen;
    /// amount of water (mm) trapped currently in ice (in mCurrentSoilFrozen)
    double mCurrentWaterFrozen;
    /// soil depth (m) of iLand
    double mSoilDepth;
    double mPWP; ///< permanent wilting point iLand of the full soil column (mm)
    double mFC; ///< field capacity iLand of the full soil column (mm)
    /// top of frozen layer (m) when thawing (above that soil is thawed)
    double mTop;
    bool mTopFrozen; ///< is the top of the soil frozen?
    /// bottom of the frozen layer (m) (important for seasonal permafrost; soil is frozen *up to* this depth)
    /// for permanent permafrost, the bottom is infinite?
    double mBottom;
    double mFreezeBack; ///< depth (m) up to which the soil is frozen again (in autumn)

    double mSOLDepth; ///< depth of the soil organic layer on top of the mineral soil (m)

    double mDeepSoilTemperature; ///< temperature (C) of the zone with secondary heat flux

    /// thermal conductivity for dry (unfrozen) soil W/m/K
    double mKdry;
    /// thermal conductivity for saturated (unfrozen) soil W/m/K
    double mKsat;
    /// thermal conductivity for saturated and frozen soil W/m/K
    double mKice;
    bool mSoilIsCoarse; ///< switch based on sand content (used for calc. of thermal conductivity)


    // stats
    struct SStats {
        void reset() { maxSnowDepth=0.; daysSnowCover=0; maxFreezeDepth=0.; maxThawDepth=0.; }
        double maxSnowDepth; ///< maxium snow depth (m) of a year
        int daysSnowCover; ///< days of the year with snow cover
        double maxFreezeDepth; ///< maximum depth of frozen soil (m)
        double maxThawDepth; ///< maximum depth of thawed soil (m)
    };
    SStats stats;

    // parameters of the permafrost model are stored in a static struct
    // (and exist only once)
    struct SParam {
        void init() { EFusion = 0.333; // MJ/litre H20
                      maxPermafrostDepth = 2.; // 2m
                 }
        // physical constants
        double EFusion; ///< latent heat of fusion, i.e. energy requied to thaw/freeze 1l of water
        double maxPermafrostDepth; ///< maximum depth (m) of active layer

        double lambdaSnow; ///< thermal conductivity [W/m*K] of snow
        double lambdaOrganicLayer; ///< thermal conductivity [W/m*K] of the organic layer

        double organicLayerDensity; ///< density (kg/m3) of the organic layer

        // parameters
        double deepSoilDepth; ///< depth (m) of the zone below the active layer from where the secondary heat flux originates
        double maxFreezeThawPerDay; ///< maximum amount of freeze/thaw (mm watercolumn) per day

        bool onlySimulate; ///< if true, peramfrost has no effect on the available water (and soil depth)
    };

    static SParam par;
    friend class PermafrostLayers;
    friend class ::WaterOut;
};

class PermafrostLayers: public LayeredGrid<ResourceUnit*> {
  public:
    ~PermafrostLayers() {}
    void setGrid(const Grid<ResourceUnit*> &grid) { mGrid = &grid; }
    void clearGrid() { mGrid = nullptr; }
    double value(ResourceUnit* const &data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> &names();
    //bool onClick(const QPointF &world_coord) const;
private:
    QVector<LayeredGridBase::LayerElement> mNames;
};


} // end namespace

#endif // PERMAFROST_H
