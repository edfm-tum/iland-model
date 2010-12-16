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
    /// additional precipitation (e.g. non evaporated water of canopy interception).
    inline double add(const double &preciptitation_mm, const double &temperature);
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
    double evapotranspirationBGC(const ClimateDay *climate, const double daylength_h); ///< evapotranspiration from soil
    double evapotranspiration3PG(const ClimateDay *climate, const double daylength_h, const double combined_response); ///< evapotranspiration from soil (mm). returns
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
    double mAirDensity; // density of air [kg / m3]

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
    double fieldCapacity() const { return mFieldCapacity; } ///< field capacity (mm)
    const double &psi_kPa(const int doy) const { return mPsi[doy]; } ///< soil water potential for the day 'doy' (0-index) in kPa
    double soilDepth() const { return mSoilDepth; } ///< soil depth in mm
    double currentContent() const { return mContent; } ///< current water content in mm
    double canopyConductance() const { return mCanopyConductance; } ///< current canopy conductance (LAI weighted CC of available tree species) (m/s)
private:
    int mLastYear; ///< last year of execution
    inline double psiFromHeight(const double mm) const; // kPa for water height "mm"
    inline double heightFromPsi(const double psi_kpa) const; // water height (mm) at water potential psi (kilopascal)
    inline double calculateBaseSoilAtmosphereResponse(const double psi_kpa, const double vpd_kpa); ///< calculate response for ground vegetation
    double mPsi_koeff_b; ///< see psiFromHeight()
    double mPsi_ref; ///< see psiFromHeight(), kPa
    double mRho_ref; ///< see psiFromHeight(), [-], m3/m3
    const ResourceUnit *mRU; ///< resource unit to which this watercycle is connected
    Water::Canopy mCanopy; ///< object representing the forest canopy (interception, evaporation)
    Water::SnowPack mSnowPack; ///< object representing the snow cover (aggregation, melting)
    double mSoilDepth; ///< depth of the soil (without rocks) in mm
    double mContent; ///< current water content in mm water column of the soil.
    double mFieldCapacity; ///< bucket height of field-capacity (eq. -15kPa) (mm)
    double mPermanentWiltingPoint; ///< bucket "height" of PWP (is fixed to -4MPa) (mm)
    double mPsi[366]; ///< soil water potential for each day in kPa
    void getStandValues(); ///< helper function to retrieve LAI per species group
    inline double calculateSoilAtmosphereResponse(const double psi_kpa, const double vpd_kpa);
    double mLAINeedle;
    double mLAIBroadleaved;
    double mCanopyConductance; ///< m/s
};

#endif // WATERCYCLE_H
