#ifndef SPECIES_H
#define SPECIES_H


#include "expression.h"

#include "speciesset.h"

class StampContainer; // forwards
class Stamp;


/// parameters for establishment
struct EstablishmentParameters
{
    double min_temp; //degC
    int chill_requirement; // days of chilling requirement
    int GDD_min, GDD_max; // GDD thresholds
    double GDD_baseTemperature; // for GDD-calc: GDD=sum(T - baseTemp)
    int bud_birst; // GDDs needed until bud burst
    int frost_free; // minimum number of annual frost-free days required
    double frost_tolerance; //factor in growing season frost tolerance calculation
    EstablishmentParameters(): min_temp(-37), chill_requirement(56), GDD_min(177), GDD_max(3261), GDD_baseTemperature(3.4),
                               bud_birst(255), frost_free(65), frost_tolerance(0.5) {}
};


class Species
{
public:
    Species(SpeciesSet *set) { mSet = set; mIndex=set->count(); mSeedDispersal=0; }
    ~Species();
    // maintenance
    void setup();
    void newYear();

    const SpeciesSet *speciesSet() const { return mSet; }
    // properties
    SeedDispersal *seedDispersal() const { return mSeedDispersal; }
    /// @property id 4-character unique identification of the tree species
    const QString &id() const { return mId; }
    /// the full name (e.g. Picea Abies) of the species
    const QString &name() const { return mName; }
    int index() const { return mIndex; } ///< unique index of species within current set
    bool active() const { return true; } ///< active??? todo!
    int phenologyClass() const { return mPhenologyClass; } ///< phenology class defined in project file. class 0 = evergreen
    bool isConiferous() const { return mConiferous; }
    bool isEvergreen() const { return mEvergreen; }
    bool isSeedYear() const { return mIsSeedYear; }


    // calculations: allometries
    double biomassFoliage(const double dbh) const;
    double biomassWoody(const double dbh) const;
    double biomassRoot(const double dbh) const;
    double allometricRatio_wf() const { return mWoody_b / mFoliage_b; }
    double allometricFractionStem(const double dbh) const;
    double finerootFoliageRatio() const { return mFinerootFoliageRatio; } ///< ratio of fineroot mass (kg) to foliage mass (kg)

    // turnover rates
    double turnoverLeaf() const { return mTurnoverLeaf; }
    double turnoverRoot() const { return mTurnoverRoot; }
    // hd-values
    void hdRange(const double dbh, double &rMinHD, double &rMaxHD) const;
    // growth
    double volumeFactor() const { return mVolumeFactor; } ///< factor for volume calculation: V = factor * D^2*H (incorporates density and the form of the bole)
    double density() const { return mWoodDensity; } ///< density of stem wood [kg/m3]
    double specificLeafArea() const { return mSpecificLeafArea; }
    // mortality
    double deathProb_intrinsic() const { return mDeathProb_intrinsic; }
    inline double deathProb_stress(const double &stress_index) const;
    // aging
    double aging(const float height, const int age) const;
    int estimateAge(const float height) const;///< estimate age for a tree with the current age
    // regeneration
    void seedProduction(const int age, const QPoint &position_index);
    void setSeedDispersal(SeedDispersal *seed_dispersal) {mSeedDispersal=seed_dispersal; }
    // environmental responses
    double vpdResponse(const double &vpd) const;
    inline double temperatureResponse(const double &delayed_temp) const;
    double nitrogenResponse(const double &availableNitrogen) const { return mSet->nitrogenResponse(availableNitrogen, mRespNitrogenClass); }
    double canopyConductance() const { return mMaxCanopyConductance; } ///< maximum canopy conductance in m/s
    inline double soilwaterResponse(const double &psi_kPa) const; ///< input: matrix potential (kPa) (e.g. -15)
    double lightResponse(const double lightResourceIndex) {return mSet->lightResponse(lightResourceIndex, mLightResponseClass); }
    double psiMin() const { return mPsiMin; }
    // parameters for seed dispersal
    void treeMigKernel(double &ras1, double &ras2, double &ks) const { ras1=mTM_as1; ras2=mTM_as2; ks=mTM_ks; }
    double fecundity_m2() const { return mFecundity_m2; }
    double nonSeedYearFraction() const { return mNonSeedYearFraction; }
    const EstablishmentParameters &establishmentParameters() const { return mEstablishmentParams; }

    const Stamp* stamp(const float dbh, const float height) const { return mLIPs.stamp(dbh, height);}
private:
    Q_DISABLE_COPY(Species);
    // helpers during setup
    bool boolVar(const QString s) { return mSet->var(s).toBool(); } ///< during setup: get value of variable @p s as a boolean variable.
    double doubleVar(const QString s) { return mSet->var(s).toDouble(); }///< during setup: get value of variable @p s as a double.
    int intVar(const QString s) { return mSet->var(s).toInt(); } ///< during setup: get value of variable @p s as an integer.
    QString stringVar(const QString s) { return mSet->var(s).toString(); } ///< during setup: get value of variable @p s as a string.

    SpeciesSet *mSet; ///< ptr. to the "parent" set
    StampContainer mLIPs; ///< ptr to the container of the LIP-pattern
    QString mId;
    QString mName;
    int mIndex; ///< internal index within the SpeciesSet
    bool mConiferous; ///< true if confierous species (vs. broadleaved)
    bool mEvergreen; ///< true if evergreen species
    // biomass allometries:
    double mFoliage_a, mFoliage_b;  ///< allometry (biomass = a * dbh^b) for foliage
    double mWoody_a, mWoody_b; ///< allometry (biomass = a * dbh^b) for woody compartments aboveground
    double mRoot_a, mRoot_b; ///< allometry (biomass = a * dbh^b) for roots (compound, fine and coarse roots as one pool)
    double mBranch_a, mBranch_b; ///< allometry (biomass = a * dbh^b) for branches

    double mSpecificLeafArea; ///< conversion factor from kg OTS to m2 LeafArea
    // turnover rates
    double mTurnoverLeaf; ///< yearly turnover rate leafs
    double mTurnoverRoot; ///< yearly turnover rate root
    double mFinerootFoliageRatio; ///< ratio of fineroot mass (kg) to foliage mass (kg)
    // height-diameter-relationships
    Expression mHDlow; ///< minimum HD-relation as f(d) (open grown tree)
    Expression mHDhigh; ///< maximum HD-relation as f(d)
    // stem density and taper
    double mWoodDensity; ///< density of the wood [kg/m3]
    double mFormFactor; ///< taper form factor of the stem [-] used for volume / stem-mass calculation calculation
    double mVolumeFactor; ///< factor for volume calculation
    // mortality
    double mDeathProb_intrinsic;  ///< prob. of intrinsic death per year [0..1]
    double mDeathProb_stress; ///< max. prob. of death per year when tree suffering maximum stress
    // Aging
    double mMaximumAge; ///< maximum age of species (years)
    double mMaximumHeight; ///< maximum height of species (m) for aging
    Expression mAging;
    // environmental responses
    double mRespVpdExponent; ///< exponent in vpd response calculation (Mäkela 2008)
    double mRespTempMin; ///< temperature response calculation offset
    double mRespTempMax; ///< temperature response calculation: saturation point for temp. response
    double mRespNitrogenClass; ///< nitrogen response class (1..3). fractional values (e.g. 1.2) are interpolated.
    double mPsiMin; ///< minimum water potential (MPa), i.e. wilting point (is below zero!)
    // water
    double mMaxCanopyConductance; ///< maximum canopy conductance for transpiration (m/s)
    int mPhenologyClass;
    double mLightResponseClass; ///< light response class (1..5) (1=shade intolerant)
    // regeneration
    SeedDispersal *mSeedDispersal; ///< link to the seed dispersal map of the species
    int mMaturityYears; ///< a tree produces seeds if it is older than this parameter
    double mSeedYearProbability; ///< probability that a year is a seed year (=1/avg.timespan between seedyears)
    bool mIsSeedYear; ///< true, if current year is a seed year. see also:
    double mNonSeedYearFraction;  ///< fraction of the seed production in non-seed-years
    // regeneration - seed dispersal
    double mFecundity_m2; ///< "surviving seeds" (cf. Moles et al) per m2, see also http://iland.boku.ac.at/fecundity
    double mTM_as1; ///< seed dispersal paramaters (treemig)
    double mTM_as2; ///< seed dispersal paramaters (treemig)
    double mTM_ks; ///< seed dispersal paramaters (treemig)
    EstablishmentParameters mEstablishmentParams;

};


// inlined functions...
inline void Species::hdRange(const double dbh, double &rLowHD, double &rHighHD) const
{
    rLowHD = mHDlow.calculate(dbh);
    rHighHD = mHDhigh.calculate(dbh);
}
/** vpdResponse calculates response on vpd.
    Input: vpd [kPa]*/
inline double Species::vpdResponse(const double &vpd) const
{
    return exp(mRespVpdExponent * vpd);
}

/** temperatureResponse calculates response on delayed daily temperature.
    Input: average temperature [°C]
    Note: slightly different from Mäkela 2008: the maximum parameter (Sk) in iLand is interpreted as the absolute
          temperature yielding a response of 1; in Mäkela 2008, Sk is the width of the range (relative to the lower threhold)
*/
inline double Species::temperatureResponse(const double &delayed_temp) const
{
    double x = qMax(delayed_temp-mRespTempMin, 0.);
    x = qMin(x/(mRespTempMax-mRespTempMin), 1.);
    return x;
}
/** soilwaterResponse is a function of the current matrix potential of the soil.

  */
inline double Species::soilwaterResponse(const double &psi_kPa) const
{
    const double psi_mpa = psi_kPa / 1000.; // convert to MPa
    double result = limit( 1. - psi_mpa / mPsiMin, 0., 1.);
    return result;
}

/** calculate probabilty of death based on the current stress index. */
inline double Species::deathProb_stress(const double &stress_index) const
{
    if (stress_index==0)
        return 0.;
    double result = 1. - exp(-mDeathProb_stress*stress_index);
    return result;
}

#endif // SPECIES_H
