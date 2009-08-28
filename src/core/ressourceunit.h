#ifndef RESSOURCEUNIT_H
#define RESSOURCEUNIT_H

#include "tree.h"

class SpeciesSet;

class RessourceUnit
{
public:
    RessourceUnit();
private:
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    QVector<Tree> mTrees; ///< storage container for tree individuals

};

#endif // RESSOURCEUNIT_H
