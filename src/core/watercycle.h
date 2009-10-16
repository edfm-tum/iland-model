#ifndef WATERCYCLE_H
#define WATERCYCLE_H
class ResourceUnit;
class ClimateDay;

/// Water contains helper classes for the water cycle calculations
namespace Water
{

/// Class SnowPack handles the snow layer
class SnowPack
{
public:
    SnowPack(): mSnowPack(0.) {}
    void setup() { mSnowPack=0.; }
    /// process the snow layer. Returns the mm of preciptitation/melt water that leaves the snow layer.
    double flow(const double &preciptitation_mm, const double &temperature);
    double snowPack() const { return mSnowPack; } ///< current snowpack height (mm)
private:
    double mSnowPack; ///< height of snowpack (mm water column)
};

/// Canopy handles the the throughfall and evaporation from the forest canopy.
class Canopy
{
public:
    // setup
    void setup(); ///< setup and load parameter values
    void setStandParameters(const double LAIneedle, const double LAIbroadleave, const double maxCanopyConductance);
    // actions
    /// process the canopy layer. returns the amount of precipitation that leaves the canopy-layer.
    double flow(const double &preciptitation_mm, const double &temperature);
    double evapotranspirationBGC(const ClimateDay *climate, const double daylength_h);
    double evapotranspiration3PG(const ClimateDay *climate, const double daylength_h);
    // properties
    double interception() const  { return mInterception; } ///< mm water that is intercepted by the crown
    double evaporationCanopy() const { return mEvaporation; } ///< evaporation from canopy (mm)
    double avgMaxCanopyConductance() const { return mAvgMaxCanopyConductance; } ///< averaged maximum canopy conductance of current species distribution (m/s)

private:
    double mLAINeedle; // leaf area index of coniferous species
    double mLAIBroadleaved; // leaf area index of broadlevaed species
    double mLAI; // total leaf area index
    double mAvgMaxCanopyConductance; // maximum weighted canopy conductance (m/s)
    double mInterception; ///< intercepted precipitation of the current day (mm)
    double mEvaporation; ///< water that evaporated from foliage surface to atmosphere (mm)
    // Penman-Monteith parameters
    double mHeatCapacityAir; // Specific heat capacity of air [J  / (kg °C)]
    double mAirDensity; // density of air [kg / m3]
    double mPsychrometricConstant; // mbar/°C

};


} // end namespace Water


class WaterCycle
{
public:
    WaterCycle();
    void setup(const ResourceUnit *ru);
    // actions
    void run(); ///< run the current year
    // properties
    const double &relContent(const int doy) const { return mRelativeContent[doy]; }
    const double &psi_kPa(const int doy) const { return mPsi[doy]; }
    double soilDepth() const { return mSoilDepth; } ///< soil depth in mm
    double currentContent() const { return mContent; } ///< current water content in mm
    double currentRelContent() const { Q_ASSERT(mSoilDepth>0); return qMin(mContent/mSoilDepth, 1.); }
private:
    inline double psiFromHeight(const double mm) const;
    inline double heightFromPsi(const double psi_kpa) const;
    double mPsi_koeff_b; ///< see psiFromHeight()
    double mPsi_ref; ///< see psiFromHeight()
    double mRho_ref; ///< see psiFromHeight()
    const ResourceUnit *mRU; ///< resource unit to which this watercycle is connected
    Water::Canopy mCanopy; ///< object representing the forest canopy (interception, evaporation)
    Water::SnowPack mSnowPack; ///< object representing the snow cover (aggregation, melting)
    double mSoilDepth; ///< depth of the soil (without rocks) in mm
    double mContent; ///< current water content in mm water column of the soil.
    double mFieldCapacity; ///< bucket height of field-capacity (eq. -15kPa) (mm)
    double mPermanentWiltingPoint; ///< bucket "height" of PWP (eq. approx. -1.5MPa, but depends on vegetation) (mm)
    double mRelativeContent[366]; ///< relative water content for each day of the year
    double mPsi[366]; ///< soil water potential for each day in kPa
    void getStandValues(); ///< helper function to retrieve LAI per species group
    double mLAINeedle;
    double mLAIBroadleaved;
    double mCanopyConductance;
};

#endif // WATERCYCLE_H
