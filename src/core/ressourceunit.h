#ifndef RESSOURCEUNIT_H
#define RESSOURCEUNIT_H

#include "tree.h"
#include "ressourceunitspecies.h"

class SpeciesSet;

class RessourceUnit
{
public:
    RessourceUnit(const int index);
    // access
    const int index() const { return mIndex; }
    SpeciesSet *speciesSet() const { return  mSpeciesSet; } ///< get SpeciesSet this RU links to.
    /// get RU-Species-container of @p species from the RU
    RessourceUnitSpecies &ressourceUnitSpecies(const Species *species);
    const QRectF &boundingBox() const { return mBoundingBox; }
    QVector<Tree> &trees() { return mTrees; } ///< reference to the tree list.
    const QVector<Tree> &constTrees() const { return mTrees; } ///< reference to the tree list.

    // actions
    /// returns a modifiable reference to a free space inside the tree-vector. should be used for tree-init.
    Tree &newTree();
    /// addWLA() is called by each tree to aggregate the total weighted leaf area on a unit
    void addWLA(const float WLA, const float LA) { mAggregatedWLA += WLA; mAggregatedLA += LA; }
    double interceptedRadiation(const float WLA) const { return mIntercepted_per_WLA * WLA;  } ///< returns the total! intercepted radiation.

    // model flow
    void newYear(); ///< reset values for a new simulation year
    void production(); ///< called after the LIP/LIF calc, before growth of individual trees

    // setup/maintenance
    void setSpeciesSet(SpeciesSet *set);
    void setBoundingBox(const QRectF &bb) { mBoundingBox = bb; }
private:
    int mIndex; // internal index
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    QVector<RessourceUnitSpecies> mRUSpecies; ///< data for this ressource unit per species
    QVector<Tree> mTrees; ///< storage container for tree individuals
    QRectF mBoundingBox; ///< bounding box (metric) of the RU
    float mAggregatedLA; ///< sum of leafArea
    float mAggregatedWLA; ///< sum of lightResponseIndex * LeafArea for all trees
    double mInterceptedRadiation; ///< total amount of radiation intercepted per year by this RU (full area) Unit???
    double mIntercepted_per_WLA; ///< intercepted / aggregated_wla

};

#endif // RESSOURCEUNIT_H
