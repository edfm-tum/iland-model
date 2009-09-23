#ifndef RESOURCEUNIT_H
#define RESOURCEUNIT_H

#include "tree.h"
#include "resourceunitspecies.h"
#include "standstatistics.h"

class SpeciesSet;

class ResourceUnit
{
public:
    ResourceUnit(const int index);
    // access
    int index() const { return mIndex; }
    SpeciesSet *speciesSet() const { return  mSpeciesSet; } ///< get SpeciesSet this RU links to.
    /// get RU-Species-container of @p species from the RU
    ResourceUnitSpecies &ressourceUnitSpecies(const Species *species);
    const QVector<ResourceUnitSpecies> ruSpecies() { return mRUSpecies; }
    const QRectF &boundingBox() const { return mBoundingBox; }
    QVector<Tree> &trees() { return mTrees; } ///< reference to the tree list.
    const QVector<Tree> &constTrees() const { return mTrees; } ///< reference to the tree list.

    // actions
    /// returns a modifiable reference to a free space inside the tree-vector. should be used for tree-init.
    Tree &newTree();
    /// addWLA() is called by each tree to aggregate the total weighted leaf area on a unit
    void addWLA(const float WLA, const float LA) { mAggregatedWLA += WLA; mAggregatedLA += LA; }
    double interceptedRadiation(const double LA, const double LRI) { return mRadiation_m2 * LA * (LRI + mLRIcorrection); }

    // model flow
    void newYear(); ///< reset values for a new simulation year
    void production(); ///< called after the LIP/LIF calc, before growth of individual trees
    void yearEnd(); ///< called after the growth of individuals

    // stocked area calculation
    void countStockedPixel(bool pixelIsStocked) { mPixelCount++; if (pixelIsStocked) mStockedPixelCount++; }
    // setup/maintenance
    void cleanTreeList();
    void setSpeciesSet(SpeciesSet *set);
    void setBoundingBox(const QRectF &bb) { mBoundingBox = bb; }
private:
    int mIndex; // internal index
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    QVector<ResourceUnitSpecies> mRUSpecies; ///< data for this ressource unit per species
    QVector<Tree> mTrees; ///< storage container for tree individuals
    QRectF mBoundingBox; ///< bounding box (metric) of the RU
    float mAggregatedLA; ///< sum of leafArea
    float mAggregatedWLA; ///< sum of lightResponseIndex * LeafArea for all trees
    double mLRIcorrection; ///< correction factor to scale LRIs to WLA
    double mRadiation_m2; ///< total incoming radiation per m2 and yaer
    int mPixelCount; ///< count of (Heightgrid) pixels thare are inside the RU
    int mStockedPixelCount;  ///< count of pixels that are stocked with trees
    float mStockedArea; ///< size of stocked area
    StandStatistics mStatistics; ///< aggregate values on stand value

    friend class RUWrapper;
};

#endif // RESSOURCEUNIT_H
