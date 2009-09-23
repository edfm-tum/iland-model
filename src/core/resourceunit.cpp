/** @class ResourceUnit
  ResourceUnit is the spatial unit that encapsulates a forest stand and links to several environmental components
  (Climate, Soil, Water, ...).

  */
#include <QtCore>
#include "global.h"

#include "resourceunit.h"
#include "speciesset.h"
#include "species.h"
#include "production3pg.h"


ResourceUnit::ResourceUnit(const int index)
{
    mSpeciesSet = 0;
    mIndex = index;
    mTrees.reserve(100); // start with space for 100 trees.
}

/// set species and setup the species-per-RU-data
void ResourceUnit::setSpeciesSet(SpeciesSet *set)
{
    mSpeciesSet = set;
    mRUSpecies.clear();
    for (int i=0;i<set->count();i++) {
        Species *s = const_cast<Species*>(mSpeciesSet->species(i));
        if (!s)
            throw IException("ResourceUnit::setSpeciesSet: invalid index!");
        ResourceUnitSpecies rus(s, this);
        mRUSpecies.append(rus);
    }
}

ResourceUnitSpecies &ResourceUnit::ressourceUnitSpecies(const Species *species)
{
    return mRUSpecies[species->index()];
}

Tree &ResourceUnit::newTree()
{
    // start simple: just append to the vector...
    mTrees.append(Tree());
    return mTrees.back();
}

/// remove dead trees from tree list
/// reduce size of vector if lots of space is free
/// tests showed that this way of cleanup is very fast,
/// because no memory allocations are performed (simple memmove())
/// when trees are moved.
void ResourceUnit::cleanTreeList()
{
    QVector<Tree>::iterator last=mTrees.end()-1;
    QVector<Tree>::iterator current = mTrees.begin();
    while (last>=current && (*last).isDead())
        --last;

    while (current<last) {
        if ((*current).isDead()) {
            *current = *last; // copy data!
            --last; //
            while (last>=current && (*last).isDead())
                --last;
        }
        ++current;
    }
    ++last; // last points now to the first dead tree

    // free ressources
    mTrees.erase(last, mTrees.end());
    if (mTrees.capacity()>100) {
        if (mTrees.count() / double(mTrees.capacity()) < 0.2) {
            int target_size = mTrees.count()*2;
            qDebug() << "reduce size from "<<mTrees.capacity() << "to" << target_size;
            mTrees.reserve(qMax(target_size, 100));
        }
    }
}

void ResourceUnit::newYear()
{
    mAggregatedWLA = 0.f;
    mAggregatedLA = 0.f;
    mPixelCount = mStockedPixelCount = 0;
    // clear statistics global and per species...
}

/** production() is the "stand-level" part of the biomass production (3PG).
    - The amount of radiation intercepted by the stand is calculated
    - The 3PG production for each species and ressource unit is invoked  */
void ResourceUnit::production()
{
    mStatistics.clear();
    if (mAggregatedWLA==0 || mPixelCount==0) {
        // nothing to do...
        return;
    }

    // the pixel counters are filled during the height-grid-calculations
    mStockedArea = 100. * mStockedPixelCount;

    // calculate the leaf area index (LAI)
    double LAI = mAggregatedLA / mStockedArea;
    // calculate the intercepted radiation fraction using the law of Beer Lambert
    const double k = 0.6;
    double interception_fraction = 1. - exp(-k * LAI);
    // calculate the amount of radiation available on this ressource unit
    mRadiation_m2 = 3140; // incoming radiation sum of year in MJ/m2*year

    // Formula for distribution: g = (SA*pPAR - sum(LRI*LA))/sum(LA)
    mLRIcorrection = (mStockedArea*interception_fraction - mAggregatedWLA) / mAggregatedLA;
    DBGMODE(qDebug() << QString("production: LAI: %1 avg. WLA: %4 intercepted-fraction: %2 g: %3 stocked area: %4")
            .arg(LAI).arg(interception_fraction).arg(mLRIcorrection).arg(mStockedArea); );


    // invoke species specific calculation (3PG)
    QVector<ResourceUnitSpecies>::iterator i;
    QVector<ResourceUnitSpecies>::iterator iend = mRUSpecies.end();


    //double raw_gpp_per_rad;

    for (i=mRUSpecies.begin(); i!=iend; ++i) {
        (*i).prod3PG().calculate();
        (*i).statistics().clear();
//        qDebug() << "species" << (*i).species()->id() << "raw_gpp_per_rad" << raw_gpp_per_rad;
    }
}

void ResourceUnit::yearEnd()
{
    // calculate statistics for all tree species of the ressource unit
    int c = mRUSpecies.count();
    for (int i=0;i<c; i++) {
        mRUSpecies[i].statistics().calculate();
        mStatistics.add(mRUSpecies[i].statistics());
    }
    mStatistics.calculate(); // aggreagte on stand level
}

