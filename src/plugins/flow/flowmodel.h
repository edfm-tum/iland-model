#ifndef FLOWMODEL_H
#define FLOWMODEL_H

#include "grid.h"

/**
 * @brief The FlowModel class is a C++ implementation of FlowPy.
 *
 * The class has minimal dependencies to iLand. It is
 * easy to use as the core implmentation of the algorithm for a R or python library.
 */




class FlowModel
{
public:
    FlowModel();
    /// set the digital elevation model, and the
    /// grid holding the ForestStructureIndex
    bool setup(const Grid<float> *DEM, Grid<float> &FSI);

    /// set starting area (point, rect, or polygon)
    void setStartArea(QPointF point);
    void setStartArea(QRectF rectangle);
    void setStartArea(int standId);

    /// run Flow
    bool run();

    // access to data
    Grid<float> &dem() { return mDEM; }
    Grid<float> &flux() { return mFlux; }
    Grid<float> &energy() { return mEnergy; }
    Grid<float> &fsi() { return *mFSI; }

private:
    /// grid (external) with forest structure information
    Grid<float> *mFSI { nullptr };

    /// grid storing elevation
    Grid<float> mDEM;
    /// grid storing maximum energy height for each cell
    Grid<float> mEnergy;
    /// grid storing cumulative flux for each cell
    Grid<float> mFlux;
};

#endif // FLOWMODEL_H
