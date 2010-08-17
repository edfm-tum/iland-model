#ifndef RESOURCEUNITSPECIES_H
#define RESOURCEUNITSPECIES_H
#include "production3pg.h"
#include "standstatistics.h"
#include "speciesresponse.h"
#include "establishment.h"
#include "sapling.h"

class Species;
class ResourceUnit;

class ResourceUnitSpecies
{
public:
    ResourceUnitSpecies() : mLAIfactor(0.), mSpecies(0), mRU(0) {}
    void setup(Species *species, ResourceUnit *ru);

    // access
    const SpeciesResponse *speciesResponse() const { return &mResponse; }
    const Species *species() const { return mSpecies; } ///< return pointer to species
    const ResourceUnit *ru() const { return mRU; } ///< return pointer to resource unit
    const Production3PG &prod3PG() const { return m3PG; } ///< the 3pg production model of this speies x resourceunit
    StandStatistics &statistics() { return mStatistics; } ///< statistics of this species on the resourceunit
    StandStatistics &statisticsDead() { return mStatisticsDead; } ///< statistics of died trees
    StandStatistics &statisticsMgmt() { return mStatisticsMgmt; } ///< statistics of removed trees
    const StandStatistics &constStatistics() const { return mStatistics; } ///< const accessor
    const StandStatistics &constStatisticsDead() const { return mStatisticsDead; } ///< const accessor
    const StandStatistics &constStatisticsMgmt() const { return mStatisticsMgmt; } ///< const accessor

   // actions
    void updateGWL();
    double removedVolume() const { return mRemovedGrowth; } ///< sum of volume with was remvoved because of death/management (m3)
    double LAIfactor() const { return mLAIfactor; } ///< relative fraction of LAI of this species (0..1)
    void setLAIfactor(const double newLAIfraction) { mLAIfactor=newLAIfraction; if (mLAIfactor<0 || mLAIfactor>1.00001) qDebug() << "invalid LAIfactor"<<mLAIfactor; }
    // properties
    double leafArea() const; ///< total leaf area of the species on the RU (m2).
    // action
    void calculate(const bool fromEstablishment=false); ///< calculate response for species, calculate actual 3PG production
    void calclulateEstablishment(); ///< perform establishment calculations
    void calclulateSaplingGrowth(); ///< growth of saplings

private:
    double mLAIfactor; ///< relative amount of this species' LAI on this resource unit (0..1). Is calculated once a year.
    double mRemovedGrowth; ///< m3 volume of trees removed/managed (to calculate GWL)
    StandStatistics mStatistics; ///< statistics of a species on this resource unit
    StandStatistics mStatisticsDead; ///< statistics of died trees (this year) of a species on this resource unit
    StandStatistics mStatisticsMgmt; ///< statistics of removed trees (this year) of a species on this resource unit
    Production3PG m3PG; ///< NPP prodution unit of this species
    SpeciesResponse mResponse; ///< calculation and storage of species specific respones on this resource unit
    Establishment mEstablishment; ///< establishment for seedlings and sapling growth
    Sapling mSapling; ///< saplings storage/growth
    Species *mSpecies; ///< speices
    ResourceUnit *mRU; ///< resource unit
    int mLastYear;
};

#endif // RESSOURCEUNITSPECIES_H
