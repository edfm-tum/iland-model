#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "grid.h"
// forwards
class Species;
class Stamp;
class RessourceUnit;
struct HeightGridValue;

class Tree
{
public:
    // lifecycle
    Tree();
    void setup();

    // access to properties
    int id() const { return mId; }
    /// @property position The tree does not store the floating point coordinates but only the index of pixel on the LIF grid
    const QPointF position() const { Q_ASSERT(mGrid!=0); return mGrid->cellCenterPoint(mPositionIndex); }
    float dbh() const { return mDbh; }
    float height() const { return mHeight; }
    const Species* species() const { Q_ASSERT(mRU!=0); return mSpecies; } ///< pointer to the tree species of the tree.
    const RessourceUnit *ru() const { Q_ASSERT(mRU!=0); return mRU; } ///< pointer to the ressource unit the tree belongs to.

    float lightRessourceIndex() const { return mLRI; } ///< LRI of the tree (update during readStamp())
    double volume() const; ///< volume (m3) of stem volume based on geometry and density calculated on the fly.
    bool isDead() const { return flag(Tree::TreeDead); } ///< returns true if the tree is already dead.
    // actions
    void die() { setFlag(Tree::TreeDead, true); } ///< kills the tree.
    void enableDebugging(const bool enable=true) {setFlag(Tree::TreeDebugging, enable); }

    // setters for initialization
    void setNewId() { mId = m_nextId++; } ///< force a new id for this object (after copying trees)
    void setPosition(const QPointF pos) { Q_ASSERT(mGrid!=0); mPositionIndex = mGrid->indexAt(pos); }
    void setDbh(const float dbh) { mDbh=dbh; }
    void setHeight(const float height) { mHeight=height; }
    void setSpecies(Species *ts) { mSpecies=ts; }
    void setRU(RessourceUnit *ru) { mRU = ru; }

    // grid based light-concurrency functions
    void applyLIP(); ///< apply LightInfluencePattern onto the global grid
    void readLIF(); ///< calculate the lightRessourceIndex with multiplicative approach
    void heightGrid(); ///< calculate the height grid

    void applyLIP_torus(); ///< apply LightInfluencePattern on a closed 1ha area
    void readLIF_torus(); ///< calculate LRI from a closed 1ha area
    void heightGrid_torus(); ///< calculate the height grid

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
    void partitioning(double npp); ///< split NPP into various plant pools.
    double relative_height_growth(); ///< estimate height growth based on light status.
    void grow_diameter(const double &net_stem_npp); ///< actual growth of the tree's stem.
    // state variables
    int mId;
    float mDbh; ///< diameter at breast height [cm]
    float mHeight; ///< tree height [m]
    QPoint mPositionIndex; ///< index of the trees position on the basic LIF grid
    // biomass compartements
    float mLeafArea; ///< m2 leaf area
    float mOpacity; ///< multiplier on LIP weights, depending on leaf area status (opacity of the crown)
    float mFoliageMass; // kg
    float mWoodyMass; // kg
    float mRootMass; // kg
    // production relevant
    float mNPPReserve; // kg
    float mDbhDelta; ///< diameter growth [cm]
    float mLRI; ///< resulting lightRessourceIndex
    // Stamp, Species, Ressource Unit

    const Stamp *mStamp;
    Species *mSpecies;
    RessourceUnit *mRU;

    // various flags
    int mFlags;
    enum Flags { TreeDead=1, TreeDebugging=2 };
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
};


#endif // TREE_H
