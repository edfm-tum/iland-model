/** @class RessourceUnit
  RessourceUnit is the spatial unit that encapsulates a forest stand and links to several environmental components
  (Climate, Soil, Water, ...).

  */
#include <QtCore>
#include "global.h"

#include "ressourceunit.h"
#include "speciesset.h"
#include "species.h"
#include "production3pg.h"


RessourceUnit::RessourceUnit(const int index)
{
    mSpeciesSet = 0;
    mIndex = index;
}

/// set species and setup the species-per-RU-data
void RessourceUnit::setSpeciesSet(SpeciesSet *set)
{
    mSpeciesSet = set;
    mRUSpecies.clear();
    for (int i=0;i<set->count();i++) {
        Species *s = const_cast<Species*>(mSpeciesSet->species(i));
        if (!s)
            throw IException("RessourceUnit::setSpeciesSet: invalid index!");
        RessourceUnitSpecies rus(s, this);
        mRUSpecies.append(rus);
    }
}

RessourceUnitSpecies &RessourceUnit::ressourceUnitSpecies(const Species *species)
{
    return mRUSpecies[species->index()];
}

Tree &RessourceUnit::newTree()
{
    // start simple: just append to the vector...
    mTrees.append(Tree());
    return mTrees.back();
}


void RessourceUnit::newYear()
{
    mAggregatedWLA = 0.f;
    mAggregatedLA = 0.f;
    mPixelCount = mStockedPixelCount = 0;
    // clear statistics global and per species...
}

/** production() is the "stand-level" part of the biomass production (3PG).
    - The amount of radiation intercepted by the stand is calculated
    - The 3PG production for each species and ressource unit is invoked  */
void RessourceUnit::production()
{
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

    qDebug() << QString("production: LAI: %1 avg. WLA: %4 intercepted-fraction: %2 g: %3 stocked area: %4")
            .arg(LAI).arg(interception_fraction).arg(mLRIcorrection).arg(mStockedArea);


    // invoke species specific calculation (3PG)
    QVector<RessourceUnitSpecies>::iterator i;
    QVector<RessourceUnitSpecies>::iterator iend = mRUSpecies.end();


    //double raw_gpp_per_rad;
    for (i=mRUSpecies.begin(); i!=iend; ++i) {
        (*i).prod3PG().calculate();
//        qDebug() << "species" << (*i).species()->id() << "raw_gpp_per_rad" << raw_gpp_per_rad;
    }
}

