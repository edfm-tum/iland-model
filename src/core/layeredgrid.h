/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef LAYEREDGRID_H
#define LAYEREDGRID_H

#include "grid.h"

/** LayeredGrid
    @ingroup tools
  */

class LayeredGridBase
{
public:
    // access to properties
    virtual int sizeX() const=0;
    virtual int sizeY() const=0;
    virtual QRectF metricRect() const=0;
    virtual QRectF cellRect(const QPoint &p) const=0;
    // available variables
    virtual const QStringList names() const=0;
    // statistics
    /// retrieve min and max of variable 'index'
    virtual void range(double &rMin, double &rMax, const int index) const=0;

    // data access functions
    virtual double value(const float x, const float y, const int index) const = 0;
    virtual double value(const QPointF &world_coord, const int index) const = 0;
    virtual double value(const int ix, const int iy, const int index) const = 0;
    virtual double value(const int grid_index, const int index) const = 0;
};

template <class T>
class LayeredGrid: public LayeredGridBase
{
public:
    LayeredGrid(const Grid<T>& grid) { mGrid = &grid; }
    LayeredGrid() { mGrid = 0; }
    QRectF cellRect(const QPoint &p) const { return mGrid->cellRect(p); }
    QRectF metricRect() const { return mGrid->metricRect(); }
    int sizeX() const { return mGrid->sizeX(); }
    int sizeY() const { return mGrid->sizeY();}

    virtual double value(const T& data, const int index) const = 0;
    double value(const T* ptr, const int index) const { return value(mGrid->constValueAtIndex(mGrid->indexOf(ptr)), index);  }
    double value(const int grid_index, const int index) const { return value(mGrid->constValueAtIndex(grid_index), index); }
    double value(const float x, const float y, const int index) const { return value(mGrid->constValueAt(x,y), index); }
    double value(const QPointF &world_coord, const int index) const { return value(mGrid->constValueAt(world_coord), index); }
    double value(const int ix, const int iy, const int index) const { return value(mGrid->constValueAtIndex(ix, iy), index); }
    void range(double &rMin, double &rMax, const int index) const { rMin=9999999999.; rMax=-99999999999.;
                                                              for (int i=0;i<mGrid->count(); ++i) {
                                                                  rMin=qMin(rMin, value(i, index));
                                                                  rMax=qMax(rMax, value(i,index));}}

protected:
    const Grid<T> *mGrid;
};

#endif // LAYEREDGRID_H
