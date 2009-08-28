#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "grid.h"

class ImageStamp;
class Species;
class Stamp;

class Tree
{
public:
    Tree();
    void setId(const int id) { m_id=id;}
    const int id() const { return m_id; }
    void setPosition(const QPointF pos) { m_Position=pos; }
    const QPointF position() const { return m_Position; }

    void setDbh(const float dbh) { m_Dbh=dbh; }
    const float dbh() const { return m_Dbh; }

    void setHeight(const float height) { m_Height=height; }
    const float height() const { return m_Height; }

    const float impact() const { return mImpact; }
    const float impactRadius() const { return mImpactRadius; }

    const Species* species() const { return m_species; }
    void setSpecies(Species *ts) { m_species=ts; }

    void setup();

    void enableDebugging() { m_debugid = m_id; }


    // grid based stamp functions
    static void setGrid(FloatGrid* gridToStamp, FloatGrid *dominanceGrid) { m_grid = gridToStamp; m_dominanceGrid = dominanceGrid; }
    void applyStamp();
    double readStamp();
    double readStampMul();
    void heightGrid();

    // statistics
    static void resetStatistics();
    static const int statPrints() { return m_statPrint; }
    static const int statAboveZ() { return m_statAboveZ; } ///< # of trees that are above Z*, i.e. the top is above the dominant height grid

    static float lafactor;

private:
    bool isDebugging() { return m_id == m_debugid; }
    int m_id;
    float m_Dbh;
    float m_Height;
    QPointF m_Position;
    float mOwnImpact;
    float mImpactArea;
    float mImpactRadius;
    float mImpact;
    // Stamp
    const Stamp *m_stamp;
    Species *m_species;
    static FloatGrid *m_grid;
    static FloatGrid *m_dominanceGrid;
    // debugging
    static int m_debugid;

    // statistics
    static int m_statPrint;
    static int m_statAboveZ;
    static int m_nextId;
};

#endif // TREE_H
