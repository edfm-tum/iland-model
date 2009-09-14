#ifndef SPECIES_H
#define SPECIES_H


#include "expression.h"

#include "speciesset.h"

class StampContainer; // forwards
class Stamp;


class Species
{
public:
    Species(SpeciesSet *set) { mSet = set; mIndex=set->count(); }
    // properties
    /// @property id 4-character unique identification of the tree species
    const QString &id() const { return mId; }
    /// the full name (e.g. Picea Abies) of the species
    const QString &name() const { return mName; }
    int index() const { return mIndex; } ///< unique index of species within current set

    // calculations: allometries
    double biomassFoliage(const double dbh) const;
    double biomassWoody(const double dbh) const;
    double biomassRoot(const double dbh) const;
    double allometricRatio_wf() const { return mWoody_b / mFoliage_b; }
    double allometricFractionStem(const double dbh) const;

    // turnover rates
    double turnoverLeaf() const { return mTurnoverLeaf; }
    double turnoverRoot() const { return mTurnoverRoot; }
    // hd-values
    void hdRange(const double dbh, double &rMinHD, double &rMaxHD);
    // growth
    double volumeFactor() const { return mVolumeFactor; } ///< factor for volume calculation: V = factor * D^2*H (incorporates density and the form of the bole)
    double density() const { return mWoodDensity; } ///< density of stem wood [kg/m3]
    double specificLeafArea() const { return mSpecificLeafArea; }
    // mortality
    double deathProb_intrinsic() const { return mDeathProb_intrinsic; }
    double deathProb_stress() const { return mDeathProb_stress; }

    const Stamp* stamp(const float dbh, const float height) const { return mLIPs.stamp(dbh, height);}
    // maintenance
    void setup();
private:
    Q_DISABLE_COPY(Species);
    // helpers during setup
    double doubleVar(const QString s) { return mSet->var(s).toDouble(); }///< during setup: get value of variable @p s as a double.
    double intVar(const QString s) { return mSet->var(s).toInt(); } ///< during setup: get value of variable @p s as an integer.
    QString stringVar(const QString s) { return mSet->var(s).toString(); } ///< during setup: get value of variable @p s as a string.

    SpeciesSet *mSet; ///< ptr. to the "parent" set
    StampContainer mLIPs; ///< ptr to the container of the LIP-pattern
    QString mId;
    QString mName;
    int mIndex; ///< internal index within the SpeciesSet
    // biomass allometries:
    double mFoliage_a, mFoliage_b;  ///< allometry (biomass = a * dbh^b) for foliage
    double mWoody_a, mWoody_b; ///< allometry (biomass = a * dbh^b) for woody compartments aboveground
    double mRoot_a, mRoot_b; ///< allometry (biomass = a * dbh^b) for roots (compound, fine and coarse roots as one pool)
    double mBranch_a, mBranch_b; ///< allometry (biomass = a * dbh^b) for branches

    double mSpecificLeafArea; ///< conversion factor from kg OTS to m2 LeafArea
    // turnover rates
    double mTurnoverLeaf; ///< yearly turnover rate leafs
    double mTurnoverRoot; ///< yearly turnover rate root
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
};


// inlined functions...
inline void Species::hdRange(const double dbh, double &rLowHD, double &rHighHD)
{
    rLowHD = mHDlow.calculate(dbh);
    rHighHD = mHDhigh.calculate(dbh);
}

#endif // SPECIES_H
