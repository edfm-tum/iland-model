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
    void setPosition(const QPointF pos) { mPosition=pos; }
    const QPointF position() const { return mPosition; }

    void setDbh(const float dbh) { mDbh=dbh; }
    const float dbh() const { return mDbh; }

    void setHeight(const float height) { mHeight=height; }
    const float height() const { return mHeight; }

    const float impact() const { return mImpact; }
    const float impactRadius() const { return mImpactRadius; }

    void setup();

    void stampOnGrid(ImageStamp& stamp, FloatGrid& grid);
    float retrieveValue(ImageStamp& stamp, FloatGrid& grid);

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
    Stamp *m_stamp;
    TreeSpecies *m_species;
};

#endif // TREE_H
