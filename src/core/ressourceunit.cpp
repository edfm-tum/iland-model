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
    // clear statistics global and per species...
}

/** production() is the "stand-level" part of the biomass production (3PG).
    - The amount of radiation intercepted by the stand is calculated
    - The 3PG production for each species and ressource unit is invoked  */
void RessourceUnit::production()
{
    if (mAggregatedWLA==0) {
        // nothing to do...
        return;
    }
    // calculate the leaf area index (LAI)
    const double stockedRUArea = 10000; // m2 of stocked area
    double LAI = mAggregatedLA / stockedRUArea;
    // calculate the intercepted radiation fraction using the law of Beer Lambert
    const double k = 0.6;
    double interception_fraction = 1. - exp(-k * LAI);
    // calculate the amount of radiation available on this ressource unit
    const double I_year_m2 = 3140; // incoming radiation sum of year in MJ/m2*year
    // incoming: I for the RU area (only stocked!) and reduced with beer-lambert
    mInterceptedRadiation = I_year_m2 * stockedRUArea * interception_fraction;
    mIntercepted_per_WLA = mInterceptedRadiation / mAggregatedWLA;

//    qDebug() << QString("production: LAI: %1 avg. WLA: %4 intercepted-fraction: %2 intercept per WLA: %3")
//            .arg(LAI).arg(interception_fraction)
//            .arg(mIntercepted_per_WLA)
//            .arg(mAggregatedWLA/mAggregatedLA);

    // invoke species specific calculation (3PG)
    QVector<RessourceUnitSpecies>::iterator i;
    QVector<RessourceUnitSpecies>::iterator iend = mRUSpecies.end();

    Production3PG p3PG;
    double raw_gpp_per_rad;
    for (i=mRUSpecies.begin(); i!=iend; ++i) {
        raw_gpp_per_rad = p3PG.calculate( *i );
        (*i).setRawGPPperRad(raw_gpp_per_rad);
//        qDebug() << "species" << (*i).species()->id() << "raw_gpp_per_rad" << raw_gpp_per_rad;
    }
}

