#include "hemigrid.h"
#include <QtCore>
#include <algorithm>

//////////////////////////////////////////////////////
// Setup memory for the Grid.
// @param size of a single pixel in degree
//////////////////////////////////////////////////////
void HemiGrid::setup(double cellsize_degree)
{
    mMatrixCountAzimuth = int(360 / cellsize_degree);
    mMatrixCountElevation = int(90 / cellsize_degree);
    mCellSizeDegree = cellsize_degree;
    // size occupied by one pixel in rad
    mMatrixCellSize = cellsize_degree * M_PI / 180.;
    delete[] mMatrix;
    mMatrix = new double[mMatrixCountAzimuth*mMatrixCountElevation];

    clear();
}

//////////////////////////////////////////////////////
// Clear the Grid.
// @param SetWith value used to fill (default 0.)
//////////////////////////////////////////////////////
void HemiGrid::clear(double SetWith)
{
    if (mMatrix) {
        for (int i=0; i<mMatrixCountAzimuth*mMatrixCountElevation; i++)
            mMatrix[i] = SetWith;
    }
}

void HemiGrid::matrixMinMax(double &rMatrixMin, double &rMatrixMax) const
{
    rMatrixMin = 100000000.;
    rMatrixMax = -1000000000.;
    if (mMatrix) {
        for (int i=0; i<mMatrixCountAzimuth*mMatrixCountElevation; i++) {
            rMatrixMin = std::min(mMatrix[i], rMatrixMin);
            rMatrixMax = std::max(mMatrix[i], rMatrixMax);
        }
    }
}

double HemiGrid::sum(const double elevation) const
{
    int ie = indexElevation(elevation);
    int emax = matrixCountElevation();
    int amax = matrixCountAzimuth();
    double value = 0;
    for (int e=ie; e<emax; e++)
        for (int a=0; a<amax; a++)
            value += rGetByIndex(a, e);
    return value;
}

//////////////////////////////////////////////////////
// Get Gridvalue by integer internal grid indices
// @param iAzimuth index of azimuth (0..count(pixels)-1
// @param iElevation index of elevation (0..count-1)
// @return ref. to grid-value
//////////////////////////////////////////////////////
double& HemiGrid::rGetByIndex(const int iAzimuth, const int iElevation) const
{
    if (iAzimuth < mMatrixCountAzimuth && iElevation < mMatrixCountElevation
        && iAzimuth >= 0 && iElevation >= 0)
        return mMatrix[iAzimuth*mMatrixCountElevation + iElevation];
    else
        throw QString("TSolar::Rad::Get() - invalid indices");
}

//////////////////////////////////////////////////////
// Get Gridvalue
// @param Azimuth azimuth angle (-pi .. +pi, 0=south)
// @param Elevation elevation angle (0=horizon, pi/2: zenith)
// @return ref. to grid-value
//////////////////////////////////////////////////////
double& HemiGrid::rGet(const double Azimuth, const double Elevation) const
{
    int iAzimuth = indexAzimuth(Azimuth);
    int iElevation = indexElevation(Elevation);
    return rGetByIndex(iAzimuth, iElevation);
}

void HemiGrid::modify(const HemiGrid &Source, const ModifyMode mode)
{
    for (int i=0; i<mMatrixCountAzimuth*mMatrixCountElevation; i++) {
        switch (mode) {
            case Add: mMatrix[i] += Source.mMatrix[i]; break;
            case Multiply: mMatrix[i] *= Source.mMatrix[i]; break;
            case SetTo: mMatrix[i] = Source.mMatrix[i]; break;
        }
    }
}
