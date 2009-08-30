/** @class RessourceUnit
  RessourceUnit is the spatial unit that encapsulates a forest stand and links to several environmental components
  (Climate, Soil, Water, ...).

  */
#include <QtCore>
#include "global.h"

#include "ressourceunit.h"

RessourceUnit::RessourceUnit()
{
    mSpeciesSet = 0;
}

Tree &RessourceUnit::newTree()
{
    // start simple: just append to the vector...
    mTrees.append(Tree());
    return mTrees.back();
}
