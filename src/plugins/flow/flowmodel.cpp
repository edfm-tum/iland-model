#include "flowmodel.h"

FlowModel::FlowModel() {}

bool FlowModel::setup(const Grid<float> *DEM, Grid<float> &FSI)
{

    mFSI = &FSI;

    mEnergy.setup(FSI.rectangle(), FSI.cellsize());
    mFlux.setup(FSI.rectangle(), FSI.cellsize());
    mDEM.setup(FSI.rectangle(), FSI.cellsize());

    // copy elevation from DEM grid, but make sure the size matches exactly
    for (auto *p = mDEM.begin(); p!=mDEM.end(); ++p) {
        auto pt = mDEM.cellCenterPoint(p);
        if (DEM->coordValid(pt))
            *p = DEM->constValueAt(pt);
        else
            *p = 0.;
    }

    qDebug() << "*** Setup of Flow module  ***";
    qDebug() << "Landscape: " << mFSI->rectangle() << ", cellsize:", mFSI->cellsize();

    return true;
}

bool FlowModel::run()
{
    qDebug() << "Flow Model running...";
    return true;
}
