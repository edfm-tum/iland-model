#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "grid.h"

class Species;
class Stamp;
class RessourceUnit;

class Tree
{
public:
    Tree();
    const int id() const { return mId; }
    void setNewId() { mId = m_nextId++; }
    void setPosition(const QPointF pos) { mPosition=pos; }
    const QPointF position() const { return mPosition; }

    void setDbh(const float dbh) { mDbh=dbh; }
    const float dbh() const { return mDbh; }

    void setHeight(const float height) { mHeight=height; }
    const float height() const { return mHeight; }

    const float lightRessourceIndex() const { return mLRI; }

    const Species* species() const { return mSpecies; }
    void setSpecies(Species *ts) { mSpecies=ts; }
    const RessourceUnit *ru() const { return mRU; }
    void setRU(RessourceUnit *ru) { mRU = ru; }

    void setup();

    void enableDebugging(const bool enable=true) { mDebugging = enable; }


    // grid based light-concurrency functions
    void applyStamp(); ///< apply LightInfluencePattern onto the global grid
    double readStamp();
    void readStampMul(); ///< calculate the lightRessourceIndex
    void heightGrid(); ///< calculate the height grid

    // growth, etc.
    void grow();
    void grow_diameter(const double &net_stem_npp);
    double relative_height_growth();

    // static functions
    static void setGrid(FloatGrid* gridToStamp, FloatGrid *dominanceGrid) { mGrid = gridToStamp; mHeightGrid = dominanceGrid; }
    // statistics
    static void resetStatistics();
    static const int statPrints() { return m_statPrint; }
    static const int statCreated() { return m_statCreated; }

    static float lafactor;

    QString dump();

private:
    // helping functions
    void partitioning(double npp);
    // state variables
    int mId;
    float mDbh; ///< diameter at breast height [cm]
    float mHeight; ///< tree height [m]
    QPointF mPosition;
    // biomass compartements
    float mLeafArea; ///< m2 leaf area??

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

    // special functions
    bool isDebugging() { return mDebugging; }

    void dumpList(DebugList &rTargetList);
    // static data
    static FloatGrid *mGrid;
    static FloatGrid *mHeightGrid;
    // debugging
    bool mDebugging;

    // statistics
    static int m_statPrint;
    static int m_statAboveZ;
    static int m_statCreated;
    static int m_nextId;
};


#endif // TREE_H
