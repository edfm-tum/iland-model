#ifndef HemiGrid_H
#define HemiGrid_H

#include <math.h>
#include <QString>

/** HemiGrid represents a grid covering the hemisphere as well as some operations.
    The sky is modeled as equally sized tiles.
    Coordinate system:
    azimuth: angle between -pi .. +pi, where 0=south direction
    elevation: 0: horizon - pi/2 (90deg)
  */
class HemiGrid
{
 public:
    HemiGrid(): mMatrix(nullptr), mMatrixCountAzimuth(-1),
                mMatrixCountElevation(-1), mMatrixCellSize(0.), mCellSizeDegree(0.) {}
    ~HemiGrid() { delete[] mMatrix; }
    HemiGrid(const HemiGrid& grid): mMatrix(nullptr), mMatrixCountAzimuth(-1),
                                    mMatrixCountElevation(-1), mMatrixCellSize(0.), mCellSizeDegree(0.)
    {
        setup(grid.mCellSizeDegree);
        modify(grid, HemiGrid::SetTo);
    }

    void setup(double cellsize_degree);
    void clear(double SetWith = 0.);

    double& rGetByIndex(const int iAzimuth, const int iElevation) const;
    double& rGet(const double Azimuth, const double Elevation) const;

    /// get azimuth index of given azimuth angle. angle should be -pi .. pi.
    int indexAzimuth(double Azimuth) const { int x=int( (Azimuth + M_PI) / (2.*M_PI) * mMatrixCountAzimuth ); if (x==mMatrixCountAzimuth) --x; return x;}
    /// get index of elevation (0..pi/2)
    int indexElevation(double Elevation) const { int x=int( Elevation / (M_PI / 2.) * mMatrixCountElevation ); if (x==mMatrixCountElevation) --x; return x;}

    /// get azimuth angle associated with given index. @return from -pi .. pi.
    double azimuth(const int iAzimuth) const { return iAzimuth/double(mMatrixCountAzimuth) * M_PI*2 - M_PI; }
    /// get azimuth angle associated with given index. @return 0 (North) -> pi/2 (West) -> pi (South) -> 3pi/2 (East)
    double azimuthNorth(const int iAzimuth) const { return iAzimuth/double(mMatrixCountAzimuth) * M_PI*2 - M_PI/2.; }
    /// get elevation angle associated with given index. @return from 0..pi/2
    double elevation(const int iElevation) const { return iElevation/double(mMatrixCountElevation) * M_PI_2; }

    int matrixCountAzimuth() const { return mMatrixCountAzimuth; }
    int matrixCountElevation() const  { return mMatrixCountElevation; }
    /// get minimum and maximum value in the grid.
    void matrixMinMax(double &rMatrixMin, double &rMatrixMax) const;
    /// get sum of all cell values with elevation >= "elevation".
    double sum(const double elevation=0) const;

    enum ModifyMode { Add, Multiply, SetTo };
    /** combine two Hemigrids.
        use "Source" and combine it with current grid using the combine mode "mode".
        Available modes are: Add, Multiply, Set (=copy).
        @param Source Hemigrid used as source (is const, and not modified).
        @param mode Combine mode (Add, Multiply, SetTo) */
    void modify(const HemiGrid &Source, const ModifyMode mode);

 private:
    double* mMatrix;
    int mMatrixCountAzimuth;
    int mMatrixCountElevation;
    double mMatrixCellSize;
    double mCellSizeDegree;
};

#endif
