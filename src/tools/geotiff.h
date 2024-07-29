#ifndef GEOTIFF_H
#define GEOTIFF_H

#include <string>


struct FIBITMAP;
template <typename T>
class Grid; // forward

/**
 * @brief The GeoTIFF class is a wrapper for handling GeoTIFF format in
 * the FreeImage library.
 * The class supports reading TIFF files (loadImage()) and writing
 * TIFF files (initialize() + setValue() + saveToFile()).
 */


class GeoTIFF
{
public:
    GeoTIFF();
    ~GeoTIFF();
    // constants from FREE_IMAGE_TYPE (FreeImage.h)
    enum TIFDatatype { 	DTSINT16		= 3,	//!  16-bit signed integer
                       DTSINT32		= 5,	//!  32-bit signed integer
                       DTFLOAT		= 6,	//! 32-bit IEEE floating point
                       DTDOUBLE		= 7    //! 64-bit IEEE floating point
    };

    static void clearProjection();

    int loadImage(const QString &fileName);

    void copyToIntGrid(Grid<int> *grid);
    void copyToDoubleGrid(Grid<double> *grid);
    void copyToFloatGrid(Grid<float> *grid);

    // write Grid to file + free memory
    bool saveToFile(const QString &fileName);
    /// create a bitmap with the the size of the full grid, and provide the data type
    void initialize(size_t width, size_t height, TIFDatatype dtype=DTDOUBLE);
    /// set value at ix/iy to *double* value
    void setValue(size_t ix, size_t iy, double value);
    /// set value at ix/iy to *float* value
    void setValue(size_t ix, size_t iy, float value);
    /// set value at ix/iy to int* value
    void setValue(size_t ix, size_t iy, int value);
    /// set value at ix/iy to *short int* value
    void setValue(size_t ix, size_t iy, short int value);


    // getters
    double ox() const { return mOx; }
    double oy() const { return mOy; }
    double cellsize() const { return mCellsize; }
    size_t ncol() const { return mNcol; }
    size_t nrow() const { return mNrow; }
private:
    static FIBITMAP *mProjectionBitmap;
    FIBITMAP *dib;
    TIFDatatype mDType;

    double mOx, mOy;
    double mCellsize;
    size_t mNcol, mNrow;
};

#endif // GEOTIFF_H
