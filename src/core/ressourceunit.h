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
    Tree &newTree(); ///< returns a modifiable reference to a free space inside the tree-vector. should be used for tree-init.
    QVector<Tree> &trees() { return mTrees; } ///< reference to the tree list.
    const QRectF &boundingBox() { return mBoundingBox; }
    // setup/maintenance
    void setSpeciesSet(SpeciesSet *set) { mSpeciesSet = set; }
    void setBoundingBox(const QRectF &bb) { mBoundingBox = bb; }
private:
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    QVector<Tree> mTrees; ///< storage container for tree individuals
    QRectF mBoundingBox; ///< bounding box (metric) of the RU

};

#endif // RESSOURCEUNIT_H
