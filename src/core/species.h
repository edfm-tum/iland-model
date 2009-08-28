#ifndef SPECIES_H
#define SPECIES_H
#include "speciesset.h"

#include "expression.h"

class StampContainer; // forwards
class Stamp;

class Species
{
public:
    Species(SpeciesSet *set) { mSet = set; }
    // properties
    /// @property id 4-character unique identification of the tree species
    const QString &id() { return mId; }
    /// the full name (e.g. Picea Abies) of the species
    const QString &name() { return mName; }
    // calculations: allometries
    double biomassLeaf(const float dbh) { return mBiomassLeaf.calculate(); }
    double biomassStem(const float dbh) { return mBiomassLeaf.calculate(); }
    double biomassRoot(const float dbh) { return mBiomassLeaf.calculate(); }

    const Stamp* stamp(const float dbh, const float height) const;
    // maintenance
    void setup();
    void setStampContainer(const StampContainer *writer) { m_stamps = writer; }
private:
    Q_DISABLE_COPY(Species);
    /// during setup: get value of variable @p s as a double.
    double doubleVar(const QString s) { return mSet->var(s).toDouble(); }
    /// during setup: get value of variable @p s as an integer.
    double intVar(const QString s) { return mSet->var(s).toInt(); }
    /// during setup: get value of variable @p s as a string.
    QString stringVar(const QString s) { return mSet->var(s).toString(); }
    SpeciesSet *mSet; ///< ptr. to the "parent" set
    const StampContainer *m_stamps;
    QString mId;
    QString mName;
    // Allometric Functions (use expressions)
    Expression mBiomassLeaf; ///< formula for calc. of leaf-biomass as f(dbh)
    Expression mBiomassStem; ///< formula for calc. of stem-biomass as f(dbh)
    Expression mBiomassRoot; ///< formula for calc. of biomass-biomass as f(dbh)
};


#endif // SPECIES_H
