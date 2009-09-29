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
    ResourceUnitSpecies(Species *species, ResourceUnit *ru) { mSpecies = species; mRU = ru; mResponse.setup(this); }

    void calculateResponses() { mResponse.calculate(); } ///< calculate environmental responses per species (vpd, temperature, ...)
    const SpeciesResponse *speciesResponse() const { return &mResponse; }
    const Species *species() const { return mSpecies; } ///< return pointer to species
    const ResourceUnit *ru() const { return mRU; } ///< return pointer to resource unit
    Production3PG &prod3PG()  { return m3PG; } ///< the 3pg production model of this speies x resourceunit
    StandStatistics &statistics() { return mStatistics; } ///< statistics of this species on the resourceunit
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
