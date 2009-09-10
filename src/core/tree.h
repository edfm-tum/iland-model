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
    void setNewId() { mId = m_nextId++; }
    void setPosition(const QPointF pos) { mPosition=pos; }
    const QPointF position() const { return mPosition; }
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

    // actions
    void enableDebugging(const bool enable=true) { mDebugging = enable; }
    // grid based light-concurrency functions
    void applyStamp(); ///< apply LightInfluencePattern onto the global grid
    double readStamp();
    void readStampMul(); ///< calculate the lightRessourceIndex with multiplicative approach
    void heightGrid(); ///< calculate the height grid

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
    QPointF mPosition;
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

    // debugging
    bool mDebugging;

    // special functions
    bool isDebugging() { return mDebugging; }


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
