/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef RESOURCEUNIT_H
#define RESOURCEUNIT_H

#include "tree.h"
#include "resourceunitspecies.h"
#include "standstatistics.h"
#include <QtCore/QVector>
#include <QtCore/QRectF>
class SpeciesSet;
class Climate;
class WaterCycle;
class Snag;
class Soil;

struct ResourceUnitVariables
{
    double nitrogenAvailable; ///< nitrogen content (kg/m2/year)
};

class ResourceUnit
{
public:
    ResourceUnit(const int index);
    ~ResourceUnit();
    // setup/maintenance
    void setup(); ///< setup operations after the creation of the model space.
    void setSpeciesSet(SpeciesSet *set);
    void setClimate(Climate* climate) { mClimate = climate; }
    void setBoundingBox(const QRectF &bb);
    void setID(const int id) { mID = id; }

    // access to elements
    const Climate *climate() const { return mClimate; } ///< link to the climate on this resource unit
    SpeciesSet *speciesSet() const { return  mSpeciesSet; } ///< get SpeciesSet this RU links to.
    const WaterCycle *waterCycle() const { return mWater; } ///< water model of the unit
    Snag *snag() const { return mSnag; } ///< access the snag object
    Soil *soil() const { return mSoil; } ///< access the soil model

    ResourceUnitSpecies &resourceUnitSpecies(const Species *species); ///< get RU-Species-container of @p species from the RU
    const ResourceUnitSpecies *constResourceUnitSpecies(const Species *species) const; ///< get RU-Species-container of @p species from the RU
    const QList<ResourceUnitSpecies*> &ruSpecies() const { return mRUSpecies; }
    QVector<Tree> &trees() { return mTrees; } ///< reference to the tree list.
    const QVector<Tree> &constTrees() const { return mTrees; } ///< reference to the (const) tree list.
    Tree *tree(const int index) { return &(mTrees[index]);} ///< get pointer to a tree
    const ResourceUnitVariables &resouceUnitVariables() const { return mUnitVariables; } ///< access to variables that are specific to resourceUnit (e.g. nitrogenAvailable)
    const StandStatistics &statistics() const {return mStatistics; }

    // properties
    int index() const { return mIndex; }
    int id() const { return mID; }
    const QRectF &boundingBox() const { return mBoundingBox; }
    const QPoint &cornerPointOffset() const { return mCornerCoord; } ///< coordinates on the LIF grid of the upper left corner of the RU
    double area() const { return mPixelCount*100; } ///< get the resource unit area in m2
    double stockedArea() const { return mStockedArea; } ///< get the stocked area in m2
    double stockableArea() const { return mStockableArea; } ///< total stockable area in m2
    double productiveArea() const { return mEffectiveArea; } ///< TotalArea - Unstocked Area - loss due to BeerLambert (m2)
    double leafAreaIndex() const { return stockableArea()?mAggregatedLA / stockableArea():0.; } ///< Total Leaf Area Index
    double leafArea() const { return mAggregatedLA; } ///< total leaf area of resource unit (m2)
    double interceptedArea(const double LA, const double LightResponse) { return mEffectiveArea_perWLA * LA * LightResponse; }
    const double &LRImodifier() const { return mLRI_modification; }
    double averageAging() const { return mAverageAging; } ///< leaf area weighted average aging

    // actions
    Tree &newTree();  ///< returns a modifiable reference to a free space inside the tree-vector. should be used for tree-init.
    int newTreeIndex(); ///< returns the index of a newly inserted tree
    void cleanTreeList(); ///< remove dead trees from the tree storage.
    void treeDied() { mHasDeadTrees = true; } ///< sets the flag that indicates that the resource unit contains dead trees
    bool hasDiedTrees() const { return mHasDeadTrees; } ///< if true, the resource unit has dead trees and needs maybe some cleanup
    /// addWLA() is called by each tree to aggregate the total weighted leaf area on a unit
    void addWLA(const float LA, const float LRI) { mAggregatedWLA += LA*LRI; mAggregatedLA += LA; }
    void addLR(const float LA, const float LightResponse) { mAggregatedLR += LA*LightResponse; }
    /// function that distributes effective interception area according to the weight of Light response and LeafArea of the indivudal (@sa production())
    void calculateInterceptedArea();
    void addTreeAging(const double leaf_area, const double aging_factor) { mAverageAging += leaf_area*aging_factor; } ///< aggregate the tree aging values (weighted by leaf area)
    void addTreeAgingForAllTrees(); ///< calculate average tree aging for all trees of a RU. Used directly after stand initialization.
    // stocked area calculation
    void countStockedPixel(bool pixelIsStocked) { mPixelCount++; if (pixelIsStocked) mStockedPixelCount++; }
    void createStandStatistics(); ///< helping function to create an initial state for stand statistics
    void recreateStandStatistics(); ///< re-build stand statistics after some change happened to the resource unit
    void setStockableArea(const double area) { mStockableArea = area; } ///< set stockable area (m2)
    // sapling growth: the height map is per resource unit and holds the maximum height of saplings for each LIF-pixel and all species
    // the map itself is a local variable and only filled temporarily.
    void setSaplingHeightMap(float *map_pointer); ///< set (temporal) storage for sapling-height-map
    /// returns maximum sapling height at point given by point-index (LIF-index).
    /// you must call setSaplingHeightMap() with a valid map before.
    float saplingHeightAt(const QPoint &position) const {
            Q_ASSERT(mSaplingHeightMap);
            int pixel_index = cPxPerRU*(position.x()-mCornerCoord.x())+(position.y()-mCornerCoord.y());
            float h =  mSaplingHeightMap[pixel_index];
            return h;
    }
    /// access to the internal sapling height map pointer
    const float *saplingHeightMapPointer() const {return mSaplingHeightMap; }
    /// return maximum sapling height at point 'position' (LIF-index). This call is slower but works witout a prior call
    /// to setSaplingHeightMap().
    float saplingHeightForInit(const QPoint &position) const;
    /// set the height of the sapling map to the maximum of current value and 'height'.
    void setMaxSaplingHeightAt(const QPoint &position, const float height);
    /// clear all saplings of all species on a given position (after recruitment)
    void clearSaplings(const QPoint &position);
    /// kill all saplings within a given rect
    void clearSaplings(const QRectF pixel_rect, const bool remove_from_soil);
    // snag / snag dynamics
    // snag dynamics, soil carbon and nitrogen cycle
    void snagNewYear() { if (snag()) snag()->newYear(); } ///< clean transfer pools
    void calculateCarbonCycle(); ///< calculate snag dynamics at the end of a year
    // model flow
    void newYear(); ///< reset values for a new simulation year
    // LIP/LIF-cylcle -> Model
    void production(); ///< called after the LIP/LIF calc, before growth of individual trees. Production (3PG), Water-cycle
    void beforeGrow(); ///< called before growth of individuals
    // the growth of individuals -> Model
    void afterGrow(); ///< called after the growth of individuals
    void yearEnd(); ///< called at the end of a year (after regeneration??)

private:
    int mIndex; ///< internal index
    int mID; ///< ID provided by external stand grid
    bool mHasDeadTrees; ///< flag that indicates if currently dead trees are in the tree list
    Climate *mClimate; ///< pointer to the climate object of this RU
    SpeciesSet *mSpeciesSet; ///< pointer to the species set for this RU
    WaterCycle *mWater; ///< link to the Soil water calculation engine
    Snag *mSnag; ///< ptr to snag storage / dynamics
    Soil *mSoil; ///< ptr to CN dynamics soil submodel
    QList<ResourceUnitSpecies*> mRUSpecies; ///< data for this ressource unit per species
    QVector<Tree> mTrees; ///< storage container for tree individuals
    QRectF mBoundingBox; ///< bounding box (metric) of the RU
    QPoint mCornerCoord; ///< coordinates on the LIF grid of the upper left corner of the RU
    double mAggregatedLA; ///< sum of leafArea
    double mAggregatedWLA; ///< sum of lightResponse * LeafArea for all trees
    double mAggregatedLR; ///< sum of lightresponse*LA of the current unit
    double mEffectiveArea; ///< total "effective" area per resource unit, i.e. area of RU - non-stocked - beerLambert-loss
    double mEffectiveArea_perWLA; ///<
    double mLRI_modification;
    double mAverageAging; ///< leaf-area weighted average aging f this species on this RU.
    float *mSaplingHeightMap; ///< pointer to array that holds max-height for each 2x2m pixel. Note: this information is not persistent

    int mPixelCount; ///< count of (Heightgrid) pixels thare are inside the RU
    int mStockedPixelCount;  ///< count of pixels that are stocked with trees
    double mStockedArea; ///< size of stocked area
    double mStockableArea; ///< area of stockable area (defined by project setup)
    StandStatistics mStatistics; ///< aggregate values on stand value
    ResourceUnitVariables mUnitVariables;

    friend class RUWrapper;
};


#endif // RESOURCEUNIT_H
