/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef RESOURCEUNITSPECIES_H
#define RESOURCEUNITSPECIES_H
#include "production3pg.h"
#include "standstatistics.h"
#include "speciesresponse.h"
#include "establishment.h"
#include "sapling.h"
#include "grid.h"
#include "snag.h"
class Species;
class ResourceUnit;

class ResourceUnitSpecies
{
public:
    ResourceUnitSpecies() : mLAIfactor(0.), mSpecies(0), mRU(0) {}
    ~ResourceUnitSpecies();
    void setup(Species *species, ResourceUnit *ru);

    // access
    const SpeciesResponse *speciesResponse() const { return &mResponse; }
    const Species *species() const { return mSpecies; } ///< return pointer to species
    const ResourceUnit *ru() const { return mRU; } ///< return pointer to resource unit
    const Production3PG &prod3PG() const { return m3PG; } ///< the 3pg production model of this speies x resourceunit
    const Sapling &sapling() const { return mSapling; } ///< sapling growth submodel
    Establishment &establishment() { return mEstablishment; } ///< establishment submodel
    Sapling &changeSapling() { return mSapling; } ///< sapling growth submodel (non-const access)
    StandStatistics &statistics() { return mStatistics; } ///< statistics of this species on the resourceunit
    StandStatistics &statisticsDead() { return mStatisticsDead; } ///< statistics of died trees
    StandStatistics &statisticsMgmt() { return mStatisticsMgmt; } ///< statistics of removed trees
    const StandStatistics &constStatistics() const { return mStatistics; } ///< const accessor
    const StandStatistics &constStatisticsDead() const { return mStatisticsDead; } ///< const accessor
    const StandStatistics &constStatisticsMgmt() const { return mStatisticsMgmt; } ///< const accessor

   // actions
    void updateGWL();
    double removedVolume() const { return mRemovedGrowth; } ///< sum of volume with was remvoved because of death/management (m3/ha)
    /// relative fraction of LAI of this species (0..1) (if total LAI on resource unit is >= 1, then the sum of all LAIfactors of all species = 1)
    double LAIfactor() const { return mLAIfactor; }
    void setLAIfactor(const double newLAIfraction) { mLAIfactor=newLAIfraction;
                                                     if (mLAIfactor<0 || mLAIfactor>1.00001)
                                                           qDebug() << "invalid LAIfactor"<<mLAIfactor; }
    // properties
    double leafArea() const; ///< total leaf area of the species on the RU (m2).
    // action
    void calculate(const bool fromEstablishment=false); ///< calculate response for species, calculate actual 3PG production
    // establishment, sapling growth
    void calculateEstablishment(); ///< perform establishment calculations
    void calclulateSaplingGrowth(); ///< growth of saplings
    int addSapling(const QPoint &position, const float height=0.05f, const int age=1) { return mSapling.addSapling(position, height, age); } ///< add a saplings on a given position
    void clearSaplings(const QPoint &position) { mSapling.clearSaplings(position);} ///< clear saplings on a given position (after recruitment)
    bool hasSaplingAt(const QPoint &position) const { return mSapling.hasSapling(position); } ///< return true if a sapling of the current speices is present at 'position'
    // visualization/graphical output
    void visualGrid(Grid<float> &grid) const;

private:
    ResourceUnitSpecies(const ResourceUnitSpecies &); // no copy
    ResourceUnitSpecies &operator=(const ResourceUnitSpecies &); // no copy
    double mLAIfactor; ///< relative amount of this species' LAI on this resource unit (0..1). Is calculated once a year.
    double mRemovedGrowth; ///< m3 volume of trees removed/managed (to calculate GWL) (m3/ha)
    StandStatistics mStatistics; ///< statistics of a species on this resource unit
    StandStatistics mStatisticsDead; ///< statistics of died trees (this year) of a species on this resource unit
    StandStatistics mStatisticsMgmt; ///< statistics of removed trees (this year) of a species on this resource unit
    Production3PG m3PG; ///< NPP prodution unit of this species
    SpeciesResponse mResponse; ///< calculation and storage of species specific respones on this resource unit
    Establishment mEstablishment; ///< establishment for seedlings and sapling growth
    Sapling mSapling; ///< saplings storage/growth
    Species *mSpecies; ///< link to speices
    ResourceUnit *mRU; ///< link to resource unit
    int mLastYear;
};

#endif // RESSOURCEUNITSPECIES_H
