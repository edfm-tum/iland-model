#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "core/grid.h"
#include "tools/expression.h"

class ImageStamp;


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

    void stampOnGrid(ImageStamp& stamp, FloatGrid& grid);
    float retrieveValue(ImageStamp& stamp, FloatGrid& grid);

    static Expression rScale;
    static Expression hScale;
    // for visuals:
    QRect pxRect;
private:
    float mDbh;
    float mHeight;
    QPointF mPosition;
    float mOwnImpact;
    float mImpactArea;
    float mImpactRadius;
    float mImpact;
};

#endif // TREE_H
