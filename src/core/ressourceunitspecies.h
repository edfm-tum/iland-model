#ifndef RESSOURCEUNITSPECIES_H
#define RESSOURCEUNITSPECIES_H

class Species;
class RessourceUnit;

class RessourceUnitSpecies
{
public:
    RessourceUnitSpecies() : mSpecies(0), mRU(0) {}
    RessourceUnitSpecies(Species *species, RessourceUnit *ru) { mSpecies = species; mRU = ru; }
    double rawGPPperRad() const { return mRawGPPperRad; }
    void setRawGPPperRad(const double &fraction) { mRawGPPperRad = fraction; }
    const Species *species() const { return mSpecies; }
    const RessourceUnit *ru() const { return mRU; }
private:
    double mRawGPPperRad;
    Species *mSpecies;
    RessourceUnit *mRU;
};

#endif // RESSOURCEUNITSPECIES_H
