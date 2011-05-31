#ifndef LAYEREDGRID_H
#define LAYEREDGRID_H

#include "grid.h"

/** LayeredGrid

  */

class LayeredGridBase
{
public:
    // access to properties
    int sizeX() const;
    int sizeY() const;
    QRectF metricRect() const;
    // available variables
    virtual const QStringList names() const=0;
    // statistics
    /// retrieve min and max of variable 'index'
    void range(double &rMin, double &rMax, const int index);

    // data access functions
    double value(const float x, const float y, const int index) const;
    double value(const int grid_index, const int index) const;
};

template <class T>
class LayeredGrid: public LayeredGridBase
{
public:
    LayeredGrid(const Grid<T>& grid) { mGrid = &grid; }
    LayeredGrid() { mGrid = 0; }
    double value(const T& data, const int index) const;
    double value(const T* ptr, const int index) const { return value(mGrid->constValueAtIndex(mGrid->indexOf(ptr)), index);  }
    double value(const int grid_index, const int index) const { return value(mGrid->constValueAtIndex(grid_index), index); }
    double value(const float x, const float y, const int index) const { return value(mGrid->constValueAt(x,y), index); }
    void range(double &rMin, double &rMax, const int index) { rMin=9999999999.; rMax=-99999999999.;
                                                              for (int i=0;i<mGrid->count(); ++i) {
                                                                  rMin=qMin(rMin, value(i, index));
                                                                  rMax=qMax(rMax, value(i,index));}}

protected:
    const Grid<T> *mGrid;
};

#endif // LAYEREDGRID_H
