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
    void setId(const int id) { mId=id;}
    const int id() const { return mId; }
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

    void enableDebugging() { mDebugid = mId; }


    // grid based stamp functions
    static void setGrid(FloatGrid* gridToStamp, FloatGrid *dominanceGrid) { mGrid = gridToStamp; mHeightGrid = dominanceGrid; }
    void applyStamp();
    double readStamp();
    double readStampMul();
    void heightGrid();

    // statistics
    static void resetStatistics();
    static const int statPrints() { return m_statPrint; }
    static const int statCreated() { return m_statCreated; }

    static float lafactor;

private:
    bool isDebugging() { return mId == mDebugid; }
    int mId;
    float mDbh;
    float mHeight;
    QPointF mPosition;
    //float mOwnImpact;
    //float mImpactArea;
    //float mImpactRadius;
    float mLRI;
    // Stamp
    const Stamp *mStamp;
    Species *mSpecies;
    RessourceUnit *mRU;
    static FloatGrid *mGrid;
    static FloatGrid *mHeightGrid;
    // debugging
    static int mDebugid;

    // statistics
    static int m_statPrint;
    static int m_statAboveZ;
    static int m_statCreated;
    static int m_nextId;
};

#endif // TREE_H
