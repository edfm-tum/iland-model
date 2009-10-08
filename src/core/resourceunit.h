#ifndef RESOURCEUNIT_H
#define RESOURCEUNIT_H

#include "tree.h"
#include "resourceunitspecies.h"
#include "standstatistics.h"

class SpeciesSet;
class Climate;

class ResourceUnit
{
public:
    ResourceUnit(const int index);
    // access to elements
    int index() const { return mIndex; }
    Climate *climate() const { return mClimate; } ///< link to the climate on this resource unit
    SpeciesSet *speciesSet() const { return  mSpeciesSet; } ///< get SpeciesSet this RU links to.
    /// get RU-Species-container of @p species from the RU
    ResourceUnitSpecies &resourceUnitSpecies(const Species *species);
    const QVector<ResourceUnitSpecies> ruSpecies() const { return mRUSpecies; }
    const QRectF &boundingBox() const { return mBoundingBox; }
    QVector<Tree> &trees() { return mTrees; } ///< reference to the tree list.
    const QVector<Tree> &constTrees() const { return mTrees; } ///< reference to the tree list.
    // properties
    double area() const { return mPixelCount*100; } ///< get the resuorce unit area in m2
    double stockedArea() const { return mStockedArea; } ///< get the stocked area in m2
    // actions
    /// returns a modifiable reference to a free space inside the tree-vector. should be used for tree-init.
    Tree &newTree();
    /// addWLA() is called by each tree to aggregate the total weighted leaf area on a unit
    void addWLA(const float LA, const float LRI) { mAggregatedWLA += LA*LRI; mAggregatedLA += LA; }
    /// function that distributes effective interception area according to the weight of Light response and LeafArea of the indivudal (@sa production())
    double interceptedArea(const double LA, const double LRI) { return mEffectiveArea_perWLA * LA * LRI; }

    // model flow
    void newYear(); ///< reset values for a new simulation year
    void production(); ///< called after the LIP/LIF calc, before growth of individual trees
    void yearEnd(); ///< called after the growth of individuals

    // stocked area calculation
    void countStockedPixel(bool pixelIsStocked) { mPixelCount++; if (pixelIsStocked) mStockedPixelCount++; }
    // setup/maintenance
    void cleanTreeList();
    void setSpeciesSet(SpeciesSet *set);
    void setClimate(Climate* climate) { mClimate = climate; }
    void setBoundingBox(const QRectF &bb) { mBoundingBox = bb; }
private:
    int mIndex; // internal index
    Climate *mClimate; ///< pointer to the climate object of this RU
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    QVector<ResourceUnitSpecies> mRUSpecies; ///< data for this ressource unit per species
    QVector<Tree> mTrees; ///< storage container for tree individuals
    QRectF mBoundingBox; ///< bounding box (metric) of the RU
    float mAggregatedLA; ///< sum of leafArea
    float mAggregatedWLA; ///< sum of lightResponse * LeafArea for all trees
    double mEffectiveArea_perWLA; ///<

    int mPixelCount; ///< count of (Heightgrid) pixels thare are inside the RU
    int mStockedPixelCount;  ///< count of pixels that are stocked with trees
    double mStockedArea; ///< size of stocked area
    StandStatistics mStatistics; ///< aggregate values on stand value

    friend class RUWrapper;
};

#endif // RESOURCEUNIT_H
