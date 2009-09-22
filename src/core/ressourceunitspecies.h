#ifndef RESSOURCEUNITSPECIES_H
#define RESSOURCEUNITSPECIES_H
#include "production3pg.h"
#include "standstatistics.h"

class Species;
class RessourceUnit;

class RessourceUnitSpecies
{
public:
    RessourceUnitSpecies() : mSpecies(0), mRU(0) {}
    RessourceUnitSpecies(Species *species, RessourceUnit *ru) { mSpecies = species; mRU = ru; }

    const Species *species() const { return mSpecies; }
    const RessourceUnit *ru() const { return mRU; }
    Production3PG &prod3PG()  { return m3PG; }
    StandStatistics &statistics() { return mStatistics; }
    // action

private:
    StandStatistics mStatistics;
    Production3PG m3PG;
    Species *mSpecies;
    RessourceUnit *mRU;
};

#endif // RESSOURCEUNITSPECIES_H
