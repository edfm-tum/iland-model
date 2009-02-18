#include <math.h>

#include "tree.h"
#include "../core/grid.h"
#include "stamp.h"

LogicExpression Tree::rScale=LogicExpression();
LogicExpression Tree::hScale=LogicExpression();
Tree::Tree()
{
}
float dist_and_direction(const QPointF &PStart, const QPointF &PEnd, float &rAngle)
{
    float dx = PEnd.x() - PStart.x();
    float dy = PEnd.y() - PStart.y();
    float d = sqrt(dx*dx + dy*dy);
    // direction:
    rAngle = atan2(dx, dy);
    return d;
}

void Tree::stampOnGrid(Stamp& stamp, FloatGrid& grid)
{

    // use formulas to derive scaling values...
    rScale.setVar("height", mHeight);
    rScale.setVar("dbh", mDbh);
    hScale.setVar("height", mHeight);
    hScale.setVar("dbh", mDbh);

    double r = rScale.execute();
    double h = hScale.execute();
    stamp.setScale(r, h); // stamp uses scaling values to calculate impact

    float cell_value, r_cell, phi_cell;
    QPoint ul = grid.indexAt(QPointF(mPosition.x()-r, mPosition.y()-r) );
    QPoint lr =  grid.indexAt( QPointF(mPosition.x()+r, mPosition.y()+r) );
    grid.validate(ul); grid.validate(lr);
    QPoint cell;
    QPointF cellcoord;
    int ix, iy;
    mOwnImpact=0.f;
    mImpactArea=0.f;
    for (ix=ul.x(); ix<lr.x(); ix++)
        for (iy=ul.y(); iy<lr.y(); iy++) {
        cell.setX(ix); cell.setY(iy);
        cellcoord = grid.getCellCoordinates(cell);
        r_cell = dist_and_direction(mPosition, cellcoord, phi_cell);
        if (r_cell > r)
            continue;
        // get value from stamp at this location (given by radius and angle)
        cell_value = stamp.get(r_cell, phi_cell);
        // add value to cell
        mOwnImpact+=(1. - cell_value);
        mImpactArea++;
        grid.valueAtIndex(cell)*=cell_value;
    }
}

float Tree::retrieveValue(Stamp& stamp, FloatGrid& grid)
{

    rScale.setVar("height", mHeight);
    rScale.setVar("dbh", mDbh);
    hScale.setVar("height", mHeight);
    hScale.setVar("dbh", mDbh);
    double r = rScale.execute();
    double h = hScale.execute();
    stamp.setScale(r, h); // stamp uses scaling values to calculate impact

    float cell_value, r_cell, phi_cell;
    QPoint ul = grid.indexAt(QPointF(mPosition.x()-r, mPosition.y()-r) );
    QPoint lr =  grid.indexAt( QPointF(mPosition.x()+r, mPosition.y()+r) );
    grid.validate(ul); grid.validate(lr);
    QPoint cell;
    QPointF cellcoord;
    int ix, iy;
    float value=0.f;

    int counting_cells=0;
    for (ix=ul.x(); ix<lr.x(); ix++)
        for (iy=ul.y(); iy<lr.y(); iy++) {

        cell.setX(ix); cell.setY(iy);
        cellcoord = grid.getCellCoordinates(cell);
        r_cell = dist_and_direction(mPosition, cellcoord, phi_cell);
        if (r_cell>r)
            continue;
        counting_cells++;
        // get value from stamp at this location (given by radius and angle)
        //cell_value = stamp.get(r_cell, phi_cell);
        // sum up values of cell
        //value += cell_value;
        value += grid.valueAtIndex(QPoint(ix,iy)); // - cell_value;
    }
    mImpact = (value + mOwnImpact)/ float(counting_cells);
    return mImpact;
}

