#ifndef SOIL_H
#define SOIL_H

#include "snag.h"
struct SoilParams; // forward
class Soil
{
public:
    // lifecycle
    Soil();
    /// set initial pool contents
    void setInitialState(const CNPool &young_labile_kg_ha, const CNPool &young_refractory_kg_ha, const CNPair &SOM_kg_ha);

    // actions
    void setSoilInput(const CNPool &labile_input_kg_ha, const CNPool &refractory_input_kg_ha); ///< provide values for input pools
    void setClimateFactor(const double climate_factor_re) { mRE = climate_factor_re; } ///< set the climate decomposition factor for the current year
    void calculateYear(); ///< main calculation function: calculates the update of state variables
    /// remove part of the biomass (e.g.: due to fire).
    /// @param DWDfrac fraction of downed woody debris (yR) to remove (0: nothing, 1: remove 100% percent)
    /// @param litterFrac fraction of litter pools (yL) to remove (0: nothing, 1: remove 100% percent)
    /// @param soilFrac fraction of soil pool (SOM) to remove (0: nothing, 1: remove 100% percent)
    void disturbance(double DWDfrac, double litterFrac, double soilFrac);

    // access
    const CNPool &youngLabile() const { return mYL;} ///< young labile matter (t/ha)
    const CNPool &youngRefractory() const { return mYR;} ///< young refractory matter (t/ha)
    const CNPair &oldOrganicMatter() const { return mSOM;} ///< old matter (SOM) (t/ha)
    double availableNitrogen() const { return mAvailableNitrogen; } ///< return available Nitrogen (kg/ha*yr)
    QList<QVariant> debugList(); ///< return a debug output
private:
    void fetchParameters(); ///< set iland parameters for soil
    static SoilParams *mParams; // static container for parameters
    // variables
    double mRE; ///< climate factor 're' (see Snag::calculateClimateFactors())
    double mAvailableNitrogen; ///< plant available nitrogen (kg/ha)
    double mKyl; ///< litter decomposition rate
    double mKyr; ///< downed woody debris (dwd) decomposition rate
    double mKo; ///< decomposition rate for soil organic matter (i.e. the "old" pool sensu ICBM)
    double mH; ///< humification rate

    CNPool mInputLab; ///< input pool labile matter (t/ha)
    CNPool mInputRef; ///< input pool refractory matter (t/ha)
    // state variables
    CNPool mYL; ///< C/N Pool for young labile matter (i.e. litter) (t/ha)
    CNPool mYR; ///< C/N Pool for young refractory matter (i.e. downed woody debris) (t/ha)
    CNPair mSOM; ///< C/N Pool for old matter (t/ha) (i.e. soil organic matter, SOM)
};

#endif // SOIL_H
