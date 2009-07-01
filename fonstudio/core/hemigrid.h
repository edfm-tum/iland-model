//---------------------------------------------------------------------------

#ifndef HemiGrid_H
#define HemiGrid_H

/** HemiGrid represents a grid covering the hemisphehre as well as some operations.
    The sky is modeled as equally sized tiles.
    Coordinate system:
    azimuth: angle between -pi .. +pi, where 0=south direction
    elevation: 0: horizon - pi/2 (90°)
  */
class HemiGrid
{
 public:
    HemiGrid(): mMatrix(0), mMatrixCountAzimuth(-1),
                     mMatrixCountElevation(-1) {}
    ~HemiGrid() { if (mMatrix) delete[] mMatrix; }

    void setup(double cellsize_degree);
    void clear(double SetWith = 0.);
    //void DumpGrid(TStrings* List);

    double& rGetByIndex(const int iAzimuth, const int iElevation);
    double& rGet(const double Azimuth, const double Elevation);

    int indexAzimuth(double Azimuth)  { int x=int( (Azimuth + M_PI) / (2.*M_PI) * mMatrixCountAzimuth ); if (x==mMatrixCountAzimuth) --x; return x;}
    int indexElevation(double Elevation) { int x=int( Elevation / (M_PI / 2.) * mMatrixCountElevation ); if (x==mMatrixCountElevation) --x; return x;}

    int matrixCountAzimuth() const { return mMatrixCountAzimuth; }
    int matrixCountElevation() const  { return mMatrixCountElevation; }
    void matrixMinMax(double &rMatrixMin, double &rMatrixMax);

    static void projectLine(const double &x, const double &y, const double &deltah, const double &r,
        double &elevation, double &azimuth1, double &azimuth2);

    enum ModifyMode { Add, Multiply, SetTo };
    void modify(const HemiGrid &Source, const ModifyMode mode);

    void projectCylinder(const double &deltax, const double &deltay,
                         const double &offsetz, const double &height, const double &r,
                         const ModifyMode mode, const double &value);
    void modifyAngleRect( const double &elow, const double &alow1, const double &alow2,
                    const double &ehigh, const double &ahigh1, const double &ahigh2,
                    const ModifyMode mode, const double &value);
    double getSum(const HemiGrid *Weighter=NULL) const;

 private:
    double* mMatrix;
    int mMatrixCountAzimuth;
    int mMatrixCountElevation;
    double mMatrixCellSize;
};

//---------------------------------------------------------------------------
#endif
