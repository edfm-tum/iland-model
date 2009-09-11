#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "grid.h"

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
    void setNewId() { mId = m_nextId++; } ///< force a new id for this object (after copying trees)
    /// @property position The tree does not store the floating point coordinates but only the index of pixel on the LIF grid
    const QPointF position() const { Q_ASSERT(mGrid!=0); return mGrid->cellCenterPoint(mPositionIndex); }
    void setPosition(const QPointF pos) { Q_ASSERT(mGrid!=0); mPositionIndex = mGrid->indexAt(pos); }
    void setDbh(const float dbh) { mDbh=dbh; }
    float dbh() const { return mDbh; }
    void setHeight(const float height) { mHeight=height; }
    float height() const { return mHeight; }
    float lightRessourceIndex() const { return mLRI; }
    const Species* species() const { Q_ASSERT(mRU!=0); return mSpecies; }
    void setSpecies(Species *ts) { mSpecies=ts; }
    const RessourceUnit *ru() const { Q_ASSERT(mRU!=0); return mRU; }
    void setRU(RessourceUnit *ru) { mRU = ru; }
    double volume() const; ///< volume (m3) of stem volume based on geometry and density calculated on the fly.
    bool dead() const { return flag(Tree::TreeDead); }
    // actions
    void die() { setFlag(Tree::TreeDead, true); }
    void enableDebugging(const bool enable=true) {setFlag(Tree::TreeDebugging, enable); }
    // grid based light-concurrency functions
    void applyStamp(); ///< apply LightInfluencePattern onto the global grid
    void readStamp(); ///< calculate the lightRessourceIndex with multiplicative approach
    void heightGrid(); ///< calculate the height grid

    void applyStampTorus(); ///< apply LightInfluencePattern on a closed 1ha area
    void readStampTorus(); ///< calculate LRI from a closed 1ha area
    void heightGridTorus(); ///< calculate the height grid

    // growth, etc.
    void grow();
    void grow_diameter(const double &net_stem_npp);
    double relative_height_growth();

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
    void partitioning(double npp);
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
