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
    const int index() const { return mIndex; } ///< unique index of species within current set
    // calculations: allometries
    const double biomassLeaf(const double dbh) { return mBiomassLeaf.calculate(dbh); }
    const double biomassStem(const double dbh) { return mBiomassStem.calculate(dbh); }
    const double biomassRoot(const double dbh) { return mBiomassRoot.calculate(dbh); }
    // turnover rates
    const double turnoverLeaf() const { return mTurnoverLeaf; }
    const double turnoverStem() const { return mTurnoverStem; }
    const double turnoverRoot() const { return mTurnoverRoot; }

    const double specificLeafArea() const { return mSpecificLeafArea; }

    const Stamp* stamp(const float dbh, const float height) const { return mStamp.stamp(dbh, height);}
    // maintenance
    void setup();
private:
    Q_DISABLE_COPY(Species);
    /// during setup: get value of variable @p s as a double.
    double doubleVar(const QString s) { return mSet->var(s).toDouble(); }
    /// during setup: get value of variable @p s as an integer.
    double intVar(const QString s) { return mSet->var(s).toInt(); }
    /// during setup: get value of variable @p s as a string.
    QString stringVar(const QString s) { return mSet->var(s).toString(); }
    SpeciesSet *mSet; ///< ptr. to the "parent" set
    StampContainer mStamp;
    QString mId;
    QString mName;
    int mIndex; ///< internal index within the SpeciesSet
    // Allometric Functions (use expressions)
    Expression mBiomassLeaf; ///< formula for calc. of leaf-biomass as f(dbh)
    Expression mBiomassStem; ///< formula for calc. of stem-biomass as f(dbh)
    Expression mBiomassRoot; ///< formula for calc. of biomass-biomass as f(dbh)
    double mSpecificLeafArea; ///< conversion factor from kg OTS to m2 LeafArea
    // turnover rates
    double mTurnoverLeaf; ///< yearly turnover rate leafs
    double mTurnoverStem; ///< yearly turnover rate stem
    double mTurnoverRoot; ///< yearly turnover rate root
};


#endif // SPECIES_H
