/** @class RessourceUnit
  RessourceUnit is the spatial unit that encapsulates a forest stand and links to several environmental components
  (Climate, Soil, Water, ...).

  */
#include <QtCore>
#include "global.h"

#include "ressourceunit.h"
#include "speciesset.h"
#include "species.h"


RessourceUnit::RessourceUnit()
{
    mSpeciesSet = 0;
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

void RessourceUnit::beforeGrow()
{
    const double ruArea = 10000;
    double LAI = mAggregatedLA / ruArea;
    qDebug() << "LAI" << LAI;
}

