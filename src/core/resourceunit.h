#ifndef RESOURCEUNIT_H
#define RESOURCEUNIT_H

#include "tree.h"
#include "resourceunitspecies.h"
#include "standstatistics.h"

class SpeciesSet;
class Climate;
class WaterCycle;

class ResourceUnit
{
public:
    ResourceUnit(const int index);
    ~ResourceUnit();

    // access to elements
    const Climate *climate() const { return mClimate; } ///< link to the climate on this resource unit
    const WaterCycle *waterCycle() const { return mWater; } ///< water model of the unit
    SpeciesSet *speciesSet() const { return  mSpeciesSet; } ///< get SpeciesSet this RU links to.
    ResourceUnitSpecies &resourceUnitSpecies(const Species *species); ///< get RU-Species-container of @p species from the RU
    const QVector<ResourceUnitSpecies> ruSpecies() const { return mRUSpecies; }
    QVector<Tree> &trees() { return mTrees; } ///< reference to the tree list.
    const QVector<Tree> &constTrees() const { return mTrees; } ///< reference to the tree list.

    // properties
    int index() const { return mIndex; }
    const QRectF &boundingBox() const { return mBoundingBox; }
    double area() const { return mPixelCount*100; } ///< get the resuorce unit area in m2
    double stockedArea() const { return mStockedArea; } ///< get the stocked area in m2
    double productiveArea() const { return mEffectiveArea; } ///< TotalArea - Unstocked Area - loss due to BeerLambert (m2)

    // actions
    /// returns a modifiable reference to a free space inside the tree-vector. should be used for tree-init.
    Tree &newTree();
    /// addWLA() is called by each tree to aggregate the total weighted leaf area on a unit
    void addWLA(const float LA, const float LRI) { mAggregatedWLA += LA*LRI; mAggregatedLA += LA; }
    void addLR(const float LA, const float LightResponse) { mAggregatedLR += LA*LightResponse; }
    /// function that distributes effective interception area according to the weight of Light response and LeafArea of the indivudal (@sa production())
    double interceptedArea(const double LA, const double LightResponse) { return mEffectiveArea_perWLA * LA * LightResponse; }
    void calculateInterceptedArea();
    const double &LRImodifier() const { return mLRI_modification; }

    // model flow
    void newYear(); ///< reset values for a new simulation year
    void production(); ///< called after the LIP/LIF calc, before growth of individual trees
    void yearEnd(); ///< called after the growth of individuals

    // stocked area calculation
    void countStockedPixel(bool pixelIsStocked) { mPixelCount++; if (pixelIsStocked) mStockedPixelCount++; }
    void createStandStatistics();
    // setup/maintenance
    void cleanTreeList(); ///< remove dead trees from the tree storage.
    void setup(); ///< setup operations after the creation of the model space.
    void setSpeciesSet(SpeciesSet *set);
    void setClimate(Climate* climate) { mClimate = climate; }
    void setBoundingBox(const QRectF &bb) { mBoundingBox = bb; }
private:
    int mIndex; // internal index
    Climate *mClimate; ///< pointer to the climate object of this RU
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    WaterCycle *mWater; ///< link to the Soil water calculation engine
    QVector<ResourceUnitSpecies> mRUSpecies; ///< data for this ressource unit per species
    QVector<Tree> mTrees; ///< storage container for tree individuals
    QRectF mBoundingBox; ///< bounding box (metric) of the RU
    double mAggregatedLA; ///< sum of leafArea
    double mAggregatedWLA; ///< sum of lightResponse * LeafArea for all trees
    double mAggregatedLR; ///< sum of lightresponse*LA of the current unit
    double mEffectiveArea; ///< total "effective" area per resource unit, i.e. area of RU - non-stocked - beerLambert-loss
    double mEffectiveArea_perWLA; ///<
    double mLRI_modification;

    int mPixelCount; ///< count of (Heightgrid) pixels thare are inside the RU
    int mStockedPixelCount;  ///< count of pixels that are stocked with trees
    double mStockedArea; ///< size of stocked area
    StandStatistics mStatistics; ///< aggregate values on stand value

    friend class RUWrapper;
};

#endif // RESOURCEUNIT_H
