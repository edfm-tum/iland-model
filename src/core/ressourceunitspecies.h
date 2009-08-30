#ifndef RESSOURCEUNITSPECIES_H
#define RESSOURCEUNITSPECIES_H

class Species;
class RessourceUnit;

class RessourceUnitSpecies
{
public:
    RessourceUnitSpecies() : mSpecies(0), mRU(0) {}
    RessourceUnitSpecies(Species *species, RessourceUnit *ru) { mSpecies = species; mRU = ru; }
    double utilizedPARFraction() const { return mPARutilizedFraction; }
    void setUtilizedPARFraction(const double &fraction) { mPARutilizedFraction = fraction; }
private:
    double mPARutilizedFraction; ///< fraction of radiation that can be utilized by this Species on this RessourceUnit per year
    Species *mSpecies;
    RessourceUnit *mRU;
};

#endif // RESSOURCEUNITSPECIES_H
