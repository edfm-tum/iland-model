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
    void setup(Species *species, ResourceUnit *ru);

    const SpeciesResponse *speciesResponse() const { return &mResponse; }
    const Species *species() const { return mSpecies; } ///< return pointer to species
    const ResourceUnit *ru() const { return mRU; } ///< return pointer to resource unit
    const Production3PG &prod3PG() const { return m3PG; } ///< the 3pg production model of this speies x resourceunit
    StandStatistics &statistics() { return mStatistics; } ///< statistics of this species on the resourceunit
    const StandStatistics &constStatistics() const { return mStatistics; }
    // action
    void calculate();

private:
    StandStatistics mStatistics; ///< statistics per of a species on this resource unit
    Production3PG m3PG; ///< NPP prodution unit of this species
    SpeciesResponse mResponse; ///< calculation and storage of species specific respones on this resource unit
    Species *mSpecies; ///< speices
    ResourceUnit *mRU; ///< resource unit
};

#endif // RESSOURCEUNITSPECIES_H
