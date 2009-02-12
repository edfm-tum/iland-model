#ifndef TREE_H
#define TREE_H
#include <QPointF>

#include "../core/grid.h"
#include "logicexpression.h"

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

    void stampOnGrid(Stamp& stamp, FloatGrid& grid);
    float retrieveValue(Stamp& stamp, FloatGrid& grid);

    static LogicExpression rScale;
    static LogicExpression hScale;
private:
    float mDbh;
    float mHeight;
    QPointF mPosition;
    float mOwnImpact;
    float mImpact;
};

#endif // TREE_H
