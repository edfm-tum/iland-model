#ifndef ESTABLISHMENT_H
#define ESTABLISHMENT_H
#include <QtCore/QPoint>
class Climate;
class ResourceUnitSpecies;
class Establishment
{
public:
    Establishment();
    Establishment(const Climate *climate, const ResourceUnitSpecies *rus);
    /// setup function that links to a climate and the resource unit / species
    void setup(const Climate *climate, const ResourceUnitSpecies *rus);
    /// main function "calculate()": process the establishment routine
    void calculate();
    // some informations after execution
    double avgSeedDensity() const { return mPxDensity;} ///< average seed density on the RU
    double abioticEnvironment() const {return mPAbiotic; } ///< integrated value of abiotic environment (i.e.: TACA-climate + total iLand environment)
    int numberEstablished() const { return mNumberEstablished; } ///< return number of newly established trees in the current year
    bool TACAminTemp() const { return mTACA_min_temp;} ///< TACA flag for minimum temperature
    bool TACAchill() const { return mTACA_chill;} ///< TACA flag chilling requirement
    bool TACgdd() const { return mTACA_gdd;} ///< TACA flag for growing degree days
    bool TACAfrostFree() const { return mTACA_frostfree;} ///< TACA flag for number of frost free days
    int TACAfrostDaysAfterBudBirst() const { return mTACA_frostAfterBuds; } ///< number of frost days after bud birst
    double avgLIFValue() const { return mLIFcount>0?mSumLIFvalue/double(mLIFcount):0.; } ///< average LIF value of LIF pixels where establishment is tested


private:
    double mPAbiotic; ///< abiotic probability for establishment (climate)
    inline bool establishTree(const QPoint &pos_lif, const float lif_value, const float seed_value); ///< do the final check whether a seedling can establish at given location
    void calculateAbioticEnvironment(); ///< calculate the abiotic environment (TACA model)
    const Climate *mClimate; ///< link to the current climate
    const ResourceUnitSpecies *mRUS; ///< link to the resource unit species (links to production data and species respones)
    double mRegenerationProbability; ///< prob. of regeneration in the current year
    // some statistics
    double mPxDensity;
    int mNumberEstablished; // number of established trees in the current year
    // TACA switches
    bool mTACA_min_temp; // minimum temperature threshold
    bool mTACA_chill;  // (total) chilling requirement
    bool mTACA_gdd;   // gdd-thresholds
    bool mTACA_frostfree; // frost free days in vegetation period
    int mTACA_frostAfterBuds; // frost days after bud birst
    double mSumLIFvalue;
    int mLIFcount;

};

#endif // ESTABLISHMENT_H
