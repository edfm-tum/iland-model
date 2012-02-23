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

#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "grid.h"
// forwards
class Species;
class Stamp;
class ResourceUnit;
struct HeightGridValue;
struct TreeGrowthData;
class TreeOut;

class Tree
{
public:
    // lifecycle
    Tree();
    void setup();

    // access to properties
    int id() const { return mId; }
    int age() const { return mAge; }
    /// @property position The tree does not store the floating point coordinates but only the index of pixel on the LIF grid
    const QPointF position() const { Q_ASSERT(mGrid!=0); return mGrid->cellCenterPoint(mPositionIndex); }
    const Species* species() const { Q_ASSERT(mRU!=0); return mSpecies; } ///< pointer to the tree species of the tree.
    const ResourceUnit *ru() const { Q_ASSERT(mRU!=0); return mRU; } ///< pointer to the ressource unit the tree belongs to.

    // properties
    float dbh() const { return mDbh; } ///< dimater at breast height in cm
    float height() const { return mHeight; } ///< tree height in m
    float lightResourceIndex() const { return mLRI; } ///< LRI of the tree (updated during readStamp())
    float leafArea() const { return mLeafArea; } ///< leaf area (m2) of the tree
    double volume() const; ///< volume (m3) of stem volume based on geometry and density calculated on the fly.
    double basalArea() const; ///< basal area of the tree at breast height in m2
    bool isDead() const { return flag(Tree::TreeDead); } ///< returns true if the tree is already dead.
    float crownRadius() const; ///< fetch crown radius (m) from the attached stamp
    // actions
    void die(TreeGrowthData *d=0); ///< kills the tree.
    void remove(); ///< remove the tree (management)
    void enableDebugging(const bool enable=true) {setFlag(Tree::TreeDebugging, enable); }

    // setters for initialization
    void setNewId() { mId = m_nextId++; } ///< force a new id for this object (after copying trees)
    void setId(const int id) { mId = id; } ///< set a spcific ID (if provided in stand init file).
    void setPosition(const QPointF pos) { Q_ASSERT(mGrid!=0); mPositionIndex = mGrid->indexAt(pos); }
    void setPosition(const QPoint posIndex) { mPositionIndex = posIndex; }
    void setDbh(const float dbh) { mDbh=dbh; }
    void setHeight(const float height) { mHeight=height; }
    void setSpecies(Species *ts) { mSpecies=ts; }
    void setRU(ResourceUnit *ru) { mRU = ru; }
    void setAge(const int age, const float treeheight);

    // grid based light-concurrency functions
    void applyLIP(); ///< apply LightInfluencePattern onto the global grid
    void readLIF(); ///< calculate the lightResourceIndex with multiplicative approach
    void heightGrid(); ///< calculate the height grid

    void applyLIP_torus(); ///< apply LightInfluencePattern on a closed 1ha area
    void readLIF_torus(); ///< calculate LRI from a closed 1ha area
    void heightGrid_torus(); ///< calculate the height grid

    void calcLightResponse(); ///< calculate light response
    // growth, etc.
    void grow(); ///< main growth function to update the tree state.

    // static functions
    static void setGrid(FloatGrid* gridToStamp, Grid<HeightGridValue> *dominanceGrid);
    // statistics
    static void resetStatistics();
    static int statPrints() { return m_statPrint; }
    static int statCreated() { return m_statCreated; }

    QString dump();
    void dumpList(QList<QVariant> &rTargetList);

private:
    // helping functions
    void partitioning(TreeGrowthData &d); ///< split NPP into various plant pools.
    double relative_height_growth(); ///< estimate height growth based on light status.
    void grow_diameter(TreeGrowthData &d); ///< actual growth of the tree's stem.
    void mortality(TreeGrowthData &d); ///< main function that checks whether trees is to die

    // state variables
    int mId; ///< unique ID of tree
    int mAge; ///< age of tree in years
    float mDbh; ///< diameter at breast height [cm]
    float mHeight; ///< tree height [m]
    QPoint mPositionIndex; ///< index of the trees position on the basic LIF grid
    // biomass compartements
    float mLeafArea; ///< m2 leaf area
    float mOpacity; ///< multiplier on LIP weights, depending on leaf area status (opacity of the crown)
    float mFoliageMass; // kg
    float mWoodyMass; // kg
    float mFineRootMass; // kg
    float mCoarseRootMass; // kg
    // production relevant
    float mNPPReserve; // kg
    float mLRI; ///< resulting lightResourceIndex
    float mLightResponse; ///< light response used for distribution of biomass on RU level
    // auxiliary
    float mDbhDelta; ///< diameter growth [cm]
    float mStressIndex; ///< stress index (used for mortality)

    // Stamp, Species, Resource Unit
    const Stamp *mStamp;
    Species *mSpecies;
    ResourceUnit *mRU;

    // various flags
    int mFlags;
    enum Flags { TreeDead=1, TreeDebugging=2 }; ///< (binary coded) tree flags
    void setFlag(const Tree::Flags flag, const bool value) { if (value) mFlags |= flag; else mFlags &= (flag ^ 0xffffff );}
    bool flag(const Tree::Flags flag) const { return mFlags & flag; }

    // special functions
    bool isDebugging() { return flag(Tree::TreeDebugging); }

    // static data
    static FloatGrid *mGrid;
    static Grid<HeightGridValue> *mHeightGrid;

    // statistics
    static int m_statPrint;
    static int m_statAboveZ;
    static int m_statCreated;
    static int m_nextId;

    // friends
    friend class TreeWrapper;
    friend class StandStatistics;
    friend class TreeOut;
};

/// internal data structure which is passed between function and to statistics
struct TreeGrowthData
{
    double NPP; ///< total NPP (kg)
    double NPP_above; ///< NPP aboveground (kg) (NPP - fraction roots), no consideration of tree senescence
    double NPP_stem;  ///< NPP used for growth of stem (dbh,h)
    double stress_index; ///< stress index used for mortality calculation
    TreeGrowthData(): NPP(0.), NPP_above(0.), NPP_stem(0.) {}
};
#endif // TREE_H
