#include "geotiff.h"

#include <iostream>
#include <sstream>


#include "grid.h"

#include "../3rdparty/FreeImage/FreeImage.h"

FIBITMAP *GeoTIFF::mProjectionBitmap = nullptr;

GeoTIFF::GeoTIFF()
{
    dib = nullptr;
    mOx = mOy = mCellsize = 0.;
    mNcol = mNrow = 0;
    mNoDataValue = 0.;
}

GeoTIFF::~GeoTIFF()
{
    if (dib) {
        FreeImage_Unload(dib);
        dib = nullptr;
    }
}

void GeoTIFF::clearProjection()
{
    if (mProjectionBitmap) {
        FreeImage_Unload(mProjectionBitmap);
        mProjectionBitmap = nullptr;
    }
}

int GeoTIFF::loadImage(const QString &fileName)
{
    qDebug() << "Loading TIF file '{}'" << fileName;
    dib = FreeImage_Load(FIF_TIFF, fileName.toStdString().c_str());
    if (!mProjectionBitmap) {

        mProjectionBitmap = FreeImage_Allocate(10,10,24);
        FreeImage_CloneMetadata(mProjectionBitmap, dib);
        qDebug() << "GeoTIFF: meta data (incl. projection) for writing TIFs is copied from" << fileName;
    }


    unsigned int count;
    if ((count = FreeImage_GetMetadataCount(FIMD_GEOTIFF, dib))>0) {
        FITAG *tagMake = nullptr;
        FreeImage_GetMetadata(FIMD_GEOTIFF, dib, "GeoTiePoints", &tagMake);
        if (!tagMake)
            throw IException(QString("GeoTIF '%1' does not contain required tags (tie points).").arg(fileName));

        if (FreeImage_GetTagType(tagMake) != FIDT_DOUBLE)
            throw IException(QString("GeoTIF '%1' invalid datatype (tie points).").arg(fileName));

        size_t tag_count = FreeImage_GetTagCount(tagMake);
        const double *values = static_cast<const double*>(FreeImage_GetTagValue(tagMake));
        if (!values)
            throw IException(QString("GeoTIF '%1' does not contain required tags (tie points).").arg(fileName));

        if (logLevelDebug())
            for (size_t i=0; i<tag_count;++i) {
                qDebug() << "TIFF: TiePoints value #" << i << ":" << values[i];
        }
        mOx = values[0];
        mOy = values[1];
        if (mOx == 0. && mOy == 0.) {
            mOx = values[3];
            mOy = values[4];
        }


        FreeImage_GetMetadata(FIMD_GEOTIFF, dib, "GeoPixelScale", &tagMake);
        if (!tagMake)
            return -1;
        if (FreeImage_GetTagType(tagMake) != FIDT_DOUBLE)
            throw IException(QString("GeoTIF '%1' does not contain required tags (pixel scale).").arg(fileName));

        tag_count = FreeImage_GetTagCount(tagMake);
        values = static_cast<const double*>(FreeImage_GetTagValue(tagMake));
        if (!values)
            throw IException(QString("GeoTIF '%1' does not contain required tags (pixel scale).").arg(fileName));

        mCellsize = values[0];
        if (fabs(mCellsize-values[1])>0.001) {
            throw IException(QString("GeoTIF '%1' pixel scale in x and y do not match (x: %2, y: %3).").arg(fileName).arg(mCellsize).arg(values[1]));

        }
        mNcol = FreeImage_GetWidth(dib);
        mNrow = FreeImage_GetHeight(dib);


        switch (FreeImage_GetImageType(dib)) {
        case FIT_INT16: mNoDataValue = std::numeric_limits<short int>::lowest(); mDType = DTSINT16; break;
        case FIT_INT32: mNoDataValue = std::numeric_limits<int>::lowest(); mDType = DTSINT32; break;
        case FIT_FLOAT: mNoDataValue = std::numeric_limits<float>::lowest(); mDType = DTFLOAT; break;
        case FIT_DOUBLE: mNoDataValue = std::numeric_limits<double>::lowest(); mDType = DTDOUBLE; break;
        default:
            throw IException(QString("GeoTiff: The TIF file '%1' has an invalid datatype. \n" \
                                     "Currently valid are: int16 (INT2S), int32 (INT4S), float (FLT4S), double (FLT8S).").arg(fileName));
        }

        QString info_str = QString("Loaded TIF '%1', x/y: %2/%3, cellsize: %4, width: %5, height: %6, datatype %7, %8 bits per cell")
                               .arg(fileName).arg(mOx).arg(mOy)
                               .arg(mCellsize).arg(mNcol).arg(mNrow)
                               .arg(FreeImage_GetImageType(dib))
                               .arg(FreeImage_GetBPP(dib));
        qInfo() << info_str;
        return 0;

    } else {
        throw IException(QString("GeoTIF '%1' does not contain meta data.").arg(fileName));
    }

}

void GeoTIFF::copyToIntGrid(Grid<int> *grid)
{
    if (!dib)
        throw std::logic_error("Copy TIF to grid: tif not loaded!");
    auto dtype = FreeImage_GetImageType(dib);
    if (dtype != FIT_INT32 && dtype != FIT_UINT16 && dtype != FIT_INT16) {
        throw IException(QString("Copy TIF to grid: wrong data type, INT32, UINT16 or INT16 expected, got type {}").arg(FreeImage_GetImageType(dib)));
    }
    // the null value of grids (at least for INT) is weird; it is not the smallest possible value (−2,147,483,648), but instead −2,147,483,647.
    throw IException("Internal: fix handling of null values!");
    //int null_value = grid->nullValue();
    //int value_null = std::numeric_limits<LONG>::min()+2;
    int null_value = -1;
    int value_null = -1;

    if (dtype == FIT_INT32) {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            LONG *bits = (LONG*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x] < value_null ? null_value : bits[x];
            }
        }
    }

    if (dtype == FIT_UINT16) {
        value_null = std::numeric_limits<WORD>::max();
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            WORD *bits = (WORD*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x] == value_null ? null_value : bits[x];
            }
        }
    }

    if (dtype == FIT_INT16) {
        value_null = std::numeric_limits<short>::min();
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            short *bits = (short*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x] == value_null ? null_value : bits[x];
            }
        }
    }


}

void GeoTIFF::copyToDoubleGrid(Grid<double> *grid)
{
    if (!dib)
        throw IException("Copy TIF to grid: tif not loaded!");

    if (FreeImage_GetImageType(dib) != FIT_DOUBLE
        && FreeImage_GetImageType(dib) != FIT_FLOAT
        && FreeImage_GetImageType(dib) != FIT_INT16
        && FreeImage_GetImageType(dib) != FIT_INT32) {
        throw IException("Copy TIF to grid: wrong data type, double, float, int16, int32 expected!");
    }
    switch (FreeImage_GetImageType(dib)) {
    case FIT_DOUBLE: {

        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            double *bits = (double*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x] == noDataDouble() ? noDataDouble() : bits[x];
            }
        }
        return;
    }
    case FIT_FLOAT: {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            float *bits = (float*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x] == noDataFloat() ? noDataDouble() : bits[x];
            }
        }
        return;
    }
    case FIT_INT16:  {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            short int *bits = (short int *)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x] == noDataShort() ? noDataDouble() : bits[x];
            }
        }
        return;
    }
    case FIT_INT32:  {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            int *bits = (int *)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x] == noDataInt() ? noDataDouble() : bits[x];
            }
        }
        return;
    }

    default:
        throw IException("Geotiff::copyToDoubleGrid: invalid data type.");
    }


}

void GeoTIFF::copyToFloatGrid(Grid<float> *grid)
{
    if (!dib)
        throw IException("Copy TIF to grid: tif not loaded!");

    if (FreeImage_GetImageType(dib) != FIT_DOUBLE
        && FreeImage_GetImageType(dib) != FIT_FLOAT
        && FreeImage_GetImageType(dib) != FIT_INT16
        && FreeImage_GetImageType(dib) != FIT_INT32) {
        throw IException("Copy TIF to grid: wrong data type, double, float, int16, int32 expected!");
    }

    switch (FreeImage_GetImageType(dib)) {
    case FIT_DOUBLE: {

        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            double *bits = (double*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }
        return;
    }
    case FIT_FLOAT: {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            float *bits = (float*)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }
        return;
    }
    case FIT_INT16:  {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            short int *bits = (short int *)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }
        return;
    }
    case FIT_INT32:  {
        for(size_t y = 0; y < FreeImage_GetHeight(dib); y++) {
            int *bits = (int *)FreeImage_GetScanLine(dib, y);
            for(size_t x = 0; x < FreeImage_GetWidth(dib); x++) {
                grid->valueAtIndex(x,y) = bits[x];
            }
        }
        return;
    }

    default:
        throw IException("Geotiff::copyToFloatGrid: invalid data type.");
    }


}

bool GeoTIFF::saveToFile(const QString &fileName)
{
    if (!dib)
        return false;
    bool success = FreeImage_Save(FIF_TIFF, dib, fileName.toStdString().c_str(), TIFF_DEFAULT);
    FreeImage_Unload(dib);
    dib = nullptr;
    return success;

}

void GeoTIFF::initialize(size_t width, size_t height, TIFDatatype dtype)
{
    if (!mProjectionBitmap)
        throw IException("GeoTif: init write: no projection information is available. You need to load at least one TIF including projection info before writing a TIF.");

    mDType = dtype;
    if (dtype != DTSINT16 && dtype!=DTSINT32 && dtype!=DTFLOAT && dtype!=DTDOUBLE)
        throw IException("GeoTif: init write: invalid data type!");

    switch (mDType) {
    case DTSINT16: mNoDataValue = std::numeric_limits<short int>::lowest(); break;
    case DTSINT32: mNoDataValue = std::numeric_limits<int>::lowest(); break;
    case DTFLOAT: mNoDataValue = std::numeric_limits<float>::lowest(); break;
    case DTDOUBLE: mNoDataValue = std::numeric_limits<double>::lowest(); break;
    }

    FREE_IMAGE_TYPE fit = FREE_IMAGE_TYPE(dtype);
    dib = FreeImage_AllocateT(fit , width, height );
    FreeImage_CloneMetadata(dib, mProjectionBitmap);

    //if (!FreeImage_SetMetadataKeyValue(FIMD_GEOTIFF, dib, "GDAL_NODATA", "-32768"))
    //   throw std::logic_error("GeoTif: set metadata (NODATA) to xyz not successful!");
}

void GeoTIFF::setValue(size_t ix, size_t iy, double value)
{
    if (!dib)
        return;
    if (ix > FreeImage_GetWidth(dib) || iy > FreeImage_GetHeight(dib))
        return;
    switch (mDType) {
    case DTFLOAT: {
        float flt_value = static_cast<float>(value);
        ((float*)FreeImage_GetScanLine(dib, iy))[ix] = flt_value;
        return;
    }
    case DTDOUBLE:
        ((double*)FreeImage_GetScanLine(dib, iy))[ix] = value;
        return;
    case DTSINT16: {
        short int short_value = static_cast<short int>(value);
        ((short int*)FreeImage_GetScanLine(dib, iy))[ix] = short_value;
        return;
    }
    case DTSINT32: {
        int int_value = static_cast<int>(value);
        ((int*)FreeImage_GetScanLine(dib, iy))[ix] = int_value;
        return;
    }
    }

    throw IException(QString("GeoTif:setValue(): invalid type of TIF: %1").arg(mDType));
}

