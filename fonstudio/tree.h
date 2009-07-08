#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "core/grid.h"
#include "tools/expression.h"


class ImageStamp;
class TreeSpecies;
class Stamp;

class Tree
{
public:
    Tree();
    void setPosition(const QPointF pos) { m_Position=pos; }
    const QPointF position() const { return m_Position; }

    void setDbh(const float dbh) { m_Dbh=dbh; }
    const float dbh() const { return m_Dbh; }

    void setHeight(const float height) { m_Height=height; }
    const float height() const { return m_Height; }

    const float impact() const { return mImpact; }
    const float impactRadius() const { return mImpactRadius; }

    const TreeSpecies* species() const { return m_species; }
    void setSpecies(TreeSpecies *ts) { m_species=ts; }

    void setup();

    // image based stamp functions
    void stampOnGrid(ImageStamp& stamp, FloatGrid& grid);
    float retrieveValue(ImageStamp& stamp, FloatGrid& grid);

    // grid based stamp functions
    static void setGrid(FloatGrid* gridToStamp) { m_grid = gridToStamp; }
    void applyStamp();

    static Expression rScale;
    static Expression hScale;
    // for visuals:
    QRect pxRect;
private:
    float m_Dbh;
    float m_Height;
    QPointF m_Position;
    float mOwnImpact;
    float mImpactArea;
    float mImpactRadius;
    float mImpact;
    // Stamp
    const Stamp *m_stamp;
    TreeSpecies *m_species;
    static FloatGrid *m_grid;
};

#endif // TREE_H
