#ifndef RESOURCEUNITSPECIES_H
#define RESOURCEUNITSPECIES_H
#include "production3pg.h"
#include "standstatistics.h"
#include "speciesresponse.h"

class Species;
class ResourceUnit;

class ResourceUnitSpecies
{
public:
    ResourceUnitSpecies() : mSpecies(0), mRU(0) {}
    ResourceUnitSpecies(Species *species, ResourceUnit *ru) { mSpecies = species; mRU = ru; }

    const Species *species() const { return mSpecies; }
    const ResourceUnit *ru() const { return mRU; }
    Production3PG &prod3PG()  { return m3PG; }
    StandStatistics &statistics() { return mStatistics; }
    const StandStatistics &constStatistics() const { return mStatistics; }
    // action

private:
    StandStatistics mStatistics;
    Production3PG m3PG;
    Species *mSpecies;
    SpeciesResponse mResponse;
    ResourceUnit *mRU;
};

#endif // RESSOURCEUNITSPECIES_H
