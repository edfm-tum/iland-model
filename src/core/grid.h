#ifndef GRID_H
#define GRID_H

#include <QtCore>


#include <stdexcept>
#include <limits>
#include <cstring>

#include "global.h"

/** Grid class (template).

Orientation
The grid is oriented as typically coordinates on the northern hemisphere: higher y-values -> north, higher x-values-> west.
The projection is reversed for drawing on screen (Viewport).
          N
  (0/2) (1/2) (2/2)
E (0/1) (1/1) (2/1)  W
  (0/0) (1/0) (2/0)
          S
*/
template <class T>
class Grid {
public:

    Grid();
    Grid(int cellsize, int sizex, int sizey) { mData=0; setup(cellsize, sizex, sizey); }
    Grid(const QRectF rect_metric, const float cellsize) { mData=0; setup(rect_metric,cellsize); }
    // copy ctor
    Grid(const Grid<T>& toCopy);
    ~Grid() { clear(); }
    void clear() { if (mData) delete[] mData; mData=0; }

    bool setup(const float cellsize, const int sizex, const int sizey);
    bool setup(const QRectF& rect, const double cellsize);
    bool setup(Grid<T>& source) {     mData = 0;  mRect = source.mRect; return setup(source.cellsize(), source.sizeX(), source.sizeY()); }
    void initialize(const T& value) {for( T *p = begin();p!=end(); ++p) *p=value; }
    void wipe(); ///< write 0-bytes with memcpy to the whole area
    void wipe(const T value); ///< overwrite the whole area with "value" size of T must be the size of "int" ERRORNOUS!!!

    int sizeX() const { return mSizeX; }
    int sizeY() const { return mSizeY; }
    float metricSizeX() const { return mSizeX*mCellsize; }
    float metricSizeY() const { return mSizeY*mCellsize; }
    QRectF metricRect() const { return mRect; }
    float cellsize() const { return mCellsize; }
    int count() const { return mCount; } ///< returns the number of elements of the grid
    bool isEmpty() const { return mData==NULL; } ///< returns false if the grid was not setup
    // operations
    // query
    /// access (const) with index variables. use int.
    inline const T& operator()(const int ix, const int iy) const { return constValueAtIndex(ix, iy); }
    /// access (const) using metric variables. use float.
    inline const T& operator()(const float x, const float y) const { return constValueAt(x, y); }
    inline const T& operator[] (const QPointF &p) const { return constValueAt(p); }

    inline T& valueAtIndex(const QPoint& pos); ///< value at position defined by indices (x,y)
    T& valueAtIndex(const int ix, const int iy) { return valueAtIndex(QPoint(ix,iy)); } ///< const value at position defined by indices (x,y)
    T& valueAtIndex(const int index) {return mData[index]; } ///< get a ref ot value at (one-dimensional) index 'index'.

    const T& constValueAtIndex(const QPoint& pos) const; ///< value at position defined by a (integer) QPoint
    const T& constValueAtIndex(const int ix, const int iy) const { return constValueAtIndex(QPoint(ix,iy)); }

    T& valueAt(const QPointF& posf); ///< value at position defined by metric coordinates (QPointF)
    const T& constValueAt(const QPointF& posf) const; ///< value at position defined by metric coordinates (QPointF)

    T& valueAt(const float x, const float y); ///< value at position defined by metric coordinates (x,y)
    const T& constValueAt(const float x, const float y) const; ///< value at position defined by metric coordinates (x,y)

    bool coordValid(const float x, const float y) const { return x>=mRect.left() && x<mRect.right()  && y>=mRect.top() && y<mRect.bottom(); }
    bool coordValid(const QPointF &pos) const { return coordValid(pos.x(), pos.y()); }

    QPoint indexAt(const QPointF& pos) const { return QPoint(int((pos.x()-mRect.left()) / mCellsize),  int((pos.y()-mRect.top())/mCellsize)); } ///< get index of value at position pos (metric)
    /// get index (x/y) of the (linear) index 'index' (0..count-1)
    QPoint indexOf(const int index) const {return QPoint(index % mSizeX,  index / mSizeX); }
    bool isIndexValid(const QPoint& pos) const { return (pos.x()>=0 && pos.x()<mSizeX && pos.y()>=0 && pos.y()<mSizeY); } ///< return true, if position is within the grid
    bool isIndexValid(const int x, const int y) const {return (x>=0 && x<mSizeX && y>=0 && y<mSizeY); } ///< return true, if index is within the grid
    /// force @param pos to contain valid indices with respect to this grid.
    void validate(QPoint &pos) const{ pos.setX( qMax(qMin(pos.x(), mSizeX-1), 0) );  pos.setY( qMax(qMin(pos.y(), mSizeY-1), 0) );} ///< ensure that "pos" is a valid key. if out of range, pos is set to minimum/maximum values.
    /// get the (metric) centerpoint of cell with index @p pos
    QPointF cellCenterPoint(const QPoint &pos) { return QPointF( (pos.x()+0.5)*mCellsize+mRect.left(), (pos.y()+0.5)*mCellsize + mRect.top());} ///< get metric coordinates of the cells center
    /// get the metric rectangle of the cell with index @pos
    QRectF cellRect(const QPoint &pos) const { QRectF r( QPointF(mRect.left() + mCellsize*pos.x(), mRect.top() + pos.y()*mCellsize),
                                                   QSizeF(mCellsize, mCellsize)); return r; } ///< return coordinates of rect given by @param pos.

    inline  T* begin() const { return mData; } ///< get "iterator" pointer
    inline  T* end() const { return mEnd; } ///< get iterator end-pointer
    inline QPoint indexOf(T* element) const; ///< retrieve index (x/y) of the pointer element. returns -1/-1 if element is not valid.
    // special queries
    T max() const; ///< retrieve the maximum value of a grid
    T sum() const; ///< retrieve the sum of the grid
    T avg() const; ///< retrieve the average value of a grid
    // modifying operations
    void add(const T& summand);
    void multiply(const T& factor);
    /// creates a grid with lower resolution and averaged cell values.
    /// @param factor factor by which grid size is reduced (e.g. 3 -> 3x3=9 pixels are averaged to 1 result pixel)
    /// @param offsetx, offsety: start averaging with an offset from 0/0 (e.g.: x=1, y=2, factor=3: -> 1/2-3/4 -> 0/0)
    /// @return Grid with size sizeX()/factor x sizeY()/factor
    Grid<T> averaged(const int factor, const int offsetx=0, const int offsety=0) const;
    /// normalized returns a normalized grid, in a way that the sum()  = @param targetvalue.
    /// if the grid is empty or the sum is 0, no modifications are performed.
    Grid<T> normalized(const T targetvalue) const;
    T* ptr(int x, int y) { return &(mData[y*mSizeX + x]); } ///< get a pointer to the element denoted by "x" and "y"
    inline double distance(const QPoint &p1, const QPoint &p2); ///< distance (metric) between p1 and p2
    const QPoint randomPosition() const; ///< returns a (valid) random position within the grid
private:

    T* mData;
    T* mEnd; ///< pointer to 1 element behind the last
    QRectF mRect;
    float mCellsize; ///< size of a cell in meter
    int mSizeX; ///< count of cells in x-direction
    int mSizeY; ///< count of cells in y-direction
    int mCount; ///< total number of cells in the grid
};

typedef Grid<float> FloatGrid;

/** @class GridRunner is a helper class to iterate over a rectangular fraction of a grid
*/
template <class T>
class GridRunner {
public:
    GridRunner(Grid<T> &target_grid, const QRectF &rectangle) {setup(target_grid, rectangle);}
    GridRunner(const Grid<T> &target_grid, const QRectF &rectangle) {setup(target_grid, rectangle);}
    T* next(); ///< to to next element, return NULL if finished
private:
    void setup(const Grid<T> &target_grid, const QRectF &rectangle);
    T* mLast;
    T* mCurrent;
    size_t mLineLength;
    size_t mCols;
    size_t mCurrentCol;
};


// copy constructor
template <class T>
Grid<T>::Grid(const Grid<T>& toCopy)
{
    mData = 0;
    mRect = toCopy.mRect;
    setup(toCopy.cellsize(), toCopy.sizeX(), toCopy.sizeY());
    const T* end = toCopy.end();
    T* ptr = begin();
    for (T* i= toCopy.begin(); i!=end; ++i, ++ptr)
       *ptr = *i;
}

// normalize function
template <class T>
Grid<T> Grid<T>::normalized(const T targetvalue) const
{
    Grid<T> target(*this);
    T total = sum();
    T multiplier;
    if (total)
        multiplier = targetvalue / total;
    else
        return target;
    for (T* p=target.begin();p!=target.end();++p)
        *p *= multiplier;
    return target;
}


template <class T>
Grid<T> Grid<T>::averaged(const int factor, const int offsetx, const int offsety) const
{
    Grid<T> target;
    target.setup(cellsize()*factor, sizeX()/factor, sizeY()/factor);
    int x,y;
    T sum=0;
    target.initialize(sum);
    // sum over array of 2x2, 3x3, 4x4, ...
    for (x=offsetx;x<mSizeX;x++)
        for (y=offsety;y<mSizeY;y++) {
            target.valueAtIndex((x-offsetx)/factor, (y-offsety)/factor) += constValueAtIndex(x,y);
        }
    // divide
    double fsquare = factor*factor;
    for (T* p=target.begin();p!=target.end();++p)
        *p /= fsquare;
    return target;
}

template <class T>
T&  Grid<T>::valueAtIndex(const QPoint& pos)
{
    //if (isIndexValid(pos)) {
        return mData[pos.y()*mSizeX + pos.x()];
    //}
    //qCritical("Grid::valueAtIndex. invalid: %d/%d", pos.x(), pos.y());
    //return mData[0];
}

template <class T>
const T&  Grid<T>::constValueAtIndex(const QPoint& pos) const
{
    //if (isIndexValid(pos)) {
        return mData[pos.y()*mSizeX + pos.x()];
    //}
    //qCritical("Grid::constValueAtIndex. invalid: %d/%d", pos.x(), pos.y());
    //return mData[0];
}

template <class T>
T&  Grid<T>::valueAt(const float x, const float y)
{
    return valueAtIndex( indexAt(QPointF(x,y)) );
}

template <class T>
const T&  Grid<T>::constValueAt(const float x, const float y) const
{
    return constValueAtIndex( indexAt(QPointF(x,y)) );
}

template <class T>
T&  Grid<T>::valueAt(const QPointF& posf)
{
    return valueAtIndex( indexAt(posf) );
}

template <class T>
const T&  Grid<T>::constValueAt(const QPointF& posf) const
{
    return constValueAtIndex( indexAt(posf) );
}

template <class T>
Grid<T>::Grid()
{
    mData = 0; mCellsize=0.f;
    mEnd = 0;
}

template <class T>
bool Grid<T>::setup(const float cellsize, const int sizex, const int sizey)
{
    mSizeX=sizex; mSizeY=sizey; mCellsize=cellsize;
    if (mRect.isNull()) // only set rect if not set before
        mRect.setCoords(0., 0., cellsize*sizex, cellsize*sizey);
    mCount = mSizeX*mSizeY;
    if (mData) {
         delete[] mData; mData=NULL;
     }
   if (mCount>0)
        mData = new T[mCount];
   mEnd = &(mData[mCount]);
   return true;
}

template <class T>
bool Grid<T>::setup(const QRectF& rect, const double cellsize)
{
    mRect = rect;
    int dx = int(rect.width()/cellsize);
    if (mRect.left()+cellsize*dx<rect.right())
        dx++;
    int dy = int(rect.height()/cellsize);
    if (mRect.top()+cellsize*dy<rect.bottom())
        dy++;
    return setup(cellsize, dx, dy);
}

/** retrieve from the index from an element reversely from a pointer to that element.
    The internal memory layout is (for dimx=6, dimy=3):
0  1  2  3  4  5
6  7  8  9  10 11
12 13 14 15 16 17
Note: north and south are reversed, thus the item with index 0 is located in the south-western edge of the grid! */
template <class T> inline
QPoint Grid<T>::indexOf(T* element) const
{
//    QPoint result(-1,-1);
    if (element==NULL || element<mData || element>=end())
        return QPoint(-1, -1);
    int idx = element - mData;
    return QPoint(idx % mSizeX,  idx / mSizeX);
//    result.setX( idx % mSizeX);
//    result.setY( idx / mSizeX);
//    return result;
}

template <class T>
T  Grid<T>::max() const
{
    T maxv = -std::numeric_limits<T>::max();
    T* p;
    T* pend = end();
    for (p=begin(); p!=pend;++p)
       maxv = std::max(maxv, *p);
    return maxv;
}

template <class T>
T  Grid<T>::sum() const
{
    T* pend = end();
    T total = 0;
    for (T *p=begin(); p!=pend;++p)
       total += *p;
    return total;
}

template <class T>
T  Grid<T>::avg() const
{
    if (count())
        return sum() / T(count());
    else return 0;
}

template <class T>
void Grid<T>::add(const T& summand)
{
    T* pend = end();
    for (T *p=begin(); p!=pend;*p+=summand,++p)
       ;
}

template <class T>
void Grid<T>::multiply(const T& factor)
{
    T* pend = end();
    for (T *p=begin(); p!=pend;*p*=factor,++p)
       ;
}



template <class T>
void  Grid<T>::wipe()
{
    memset(mData, 0, mCount*sizeof(T));
}
template <class T>
void  Grid<T>::wipe(const T value)
{
    /* this does not work properly !!! */
    if (sizeof(T)==sizeof(int)) {
        float temp = value;
        float *pf = &temp;

        memset(mData, *((int*)pf), mCount*sizeof(T));
    } else
        initialize(value);
}

template <class T>
double Grid<T>::distance(const QPoint &p1, const QPoint &p2)
{
    QPointF fp1=cellCenterPoint(p1);
    QPointF fp2=cellCenterPoint(p2);
    double distance = sqrt( (fp1.x()-fp2.x())*(fp1.x()-fp2.x()) + (fp1.y()-fp2.y())*(fp1.y()-fp2.y()));
    return distance;
}

template <class T>
const QPoint Grid<T>::randomPosition() const
{
    return QPoint(irandom(0,mSizeX-1), irandom(0, mSizeY-1));
}

////////////////////////////////////////////////////////////
// grid runner
////////////////////////////////////////////////////////////
template <class T>
void GridRunner<T>::setup(const Grid<T> &target_grid, const QRectF &rectangle)
{
    QPoint upper_left = target_grid.indexAt(rectangle.topLeft());
    QPoint lower_right = target_grid.indexAt(rectangle.bottomRight());
    mCurrent = const_cast<Grid<T> &>(target_grid).ptr(upper_left.x(), upper_left.y());
    mLast = const_cast<Grid<T> &>(target_grid).ptr(lower_right.x()-1, lower_right.y()-1);
    mCols = lower_right.x() - upper_left.x(); //
    mLineLength =  target_grid.sizeX() - mCols;
    mCurrentCol = 0;
}

template <class T>
T* GridRunner<T>::next()
{
    if (mCurrent>mLast)
        return NULL;
    T* t = mCurrent;
    mCurrent++;
    mCurrentCol++;
    if (mCurrentCol >= mCols) {
        mCurrent += mLineLength; // skip to next line
        mCurrentCol = 0;
    }
    return t;
}

////////////////////////////////////////////////////////////
// global functions
////////////////////////////////////////////////////////////

/// dumps a FloatGrid to a String.
/// rows will be y-lines, columns x-values. (see grid.cpp)
QString gridToString(const FloatGrid &grid);

/// creates and return a QImage from Grid-Data.
/// @param black_white true: max_value = white, min_value = black, false: color-mode: uses a HSV-color model from blue (min_value) to red (max_value), default: color mode (false)
/// @param min_value, max_value min/max bounds for color calcuations. values outside bounds are limited to these values. defaults: min=0, max=1
/// @param reverse if true, color ramps are inversed (to: min_value = white (black and white mode) or red (color mode). default = false.
/// @return a QImage with the Grids size of pixels. Pixel coordinates relate to the index values of the grid.
QImage gridToImage(const FloatGrid &grid,
                   bool black_white=false,
                   double min_value=0., double max_value=1.,
                   bool reverse=false);

/** load into 'rGrid' the content of the image pointed at by 'fileName'.
    Pixels are converted to grey-scale and then transformend to a value ranging from 0..1 (black..white).
  */
bool loadGridFromImage(const QString &fileName, FloatGrid &rGrid);

/// template version for non-float grids (see also version for FloatGrid)
template <class T>
        QString gridToString(const Grid<T> &grid)
{
    QString res;
    QTextStream ts(&res);

    for (int y=0;y<grid.sizeY();y++){
        for (int x=0;x<grid.sizeX();x++){
            ts << grid.constValueAtIndex(x,y) << ";";
        }
        ts << "\r\n";
    }

    return res;
}

#endif // GRID_H
