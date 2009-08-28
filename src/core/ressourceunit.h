#ifndef RESSOURCEUNIT_H
#define RESSOURCEUNIT_H

#include "tree.h"

class SpeciesSet;

class RessourceUnit
{
public:
    RessourceUnit();
    // access
    SpeciesSet *speciesSet() { return  mSpeciesSet; }
    // setup/maintenance
    void setSpeciesSet(SpeciesSet *set) { mSpeciesSet = set; }
private:
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    QVector<Tree> mTrees; ///< storage container for tree individuals

};

#endif // RESSOURCEUNIT_H
