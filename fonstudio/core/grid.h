#ifndef GRID_H
#define GRID_H

#include <QtCore>


#include <stdexcept>

/** Grid class (template).

  */
template <class T>
class Grid {
public:

    Grid();
    Grid(int cellsize, int sizex, int sizey) { mData=0; setup(cellsize, sizex, sizey); }
    Grid(const QRectF rect_metric, const float cellsize) { mData=0; setup(rect_metric,cellsize); }
    // copy ctor
    Grid(const Grid<T>& toCopy);
    ~Grid() { if (mData) delete[] mData; }

    bool setup(const float cellsize, const int sizex, const int sizey);
    bool setup(const QRectF& rect, const double cellsize);
    void initialize(const T& value) {for( T *p = begin();p!=end(); ++p) *p=value; qDebug()<<"Grid initialize"<<end()-begin()<<"items.";}

    const int sizeX() const { return mSizeX; }
    const int sizeY() const { return mSizeY; }
    const float metricSizeX() const { return mSizeX*mCellsize; }
    const float metricSizeY() const { return mSizeY*mCellsize; }
    QRectF metricRect() const { return mRect; }
    const float cellsize() const { return mCellsize; }
    const int count() const { return mCount; }
    const bool isEmpty() const { return mData==NULL; }
    // operations
    // query
    /// access (const) with index variables. use int.
    inline const T& operator()(const int ix, const int iy) const { return constValueAtIndex(ix, iy); }
    /// access (const) using metric variables. use float.
    inline const T& operator()(const float x, const float y) const { return constValueAt(x, y); }
    inline const T& operator[] (const QPointF &p) const { return constValueAt(p); }

    T& valueAtIndex(const QPoint& pos); ///< value at position defined by indices (x,y)
    T& valueAtIndex(const int ix, const int iy) { return valueAtIndex(QPoint(ix,iy)); } ///< const value at position defined by indices (x,y)

    const T& constValueAtIndex(const QPoint& pos) const; ///< value at position defined by a (integer) QPoint
    const T& constValueAtIndex(const int ix, const int iy) const { return constValueAtIndex(QPoint(ix,iy)); }

    T& valueAt(const QPointF& posf); ///< value at position defined by metric coordinates (QPointF)
    const T& constValueAt(const QPointF& posf) const; ///< value at position defined by metric coordinates (QPointF)

    T& valueAt(const float x, const float y); ///< value at position defined by metric coordinates (x,y)
    const T& constValueAt(const float x, const float y) const; ///< value at position defined by metric coordinates (x,y)

    bool coordValid(const float x, const float y) const { return mRect.contains(x,y); }
    bool coordValid(const QPointF &pos) const { return coordValid(pos.x(), pos.y()); }
    QPoint indexAt(const QPointF& pos) const { return QPoint(int((pos.x()-mRect.left()) / mCellsize),  int((pos.y()-mRect.top())/mCellsize)); } ///< get index of value at position pos (metric)
    bool isIndexValid(const QPoint& pos) const { return (pos.x()>=0 && pos.x()<mSizeX && pos.y()>=0 && pos.y()<mSizeY); } ///< get index of value at position pos (index)
    void validate(QPoint &pos) const{ pos.setX( qMax(qMin(pos.x(), mSizeX-1), 0) );  pos.setY( qMax(qMin(pos.y(), mSizeY-1), 0) );} ///< ensure that "pos" is a valid key. if out of range, pos is set to minimum/maximum values.
    QPointF cellCoordinates(const QPoint &pos) { return QPointF( (pos.x()+0.5)*mCellsize+mRect.left(), (pos.y()+0.5)*mCellsize + mRect.top());} ///< get metric coordinates of the cells center
    QRectF cellRect(const QPoint &pos) { QRectF r( QPointF(mRect.left() + mCellsize*pos.x(), mRect.top() + pos.y()*mCellsize),
                                                   QSizeF(mCellsize, mCellsize)); return r; } ///< return coordinates of rect given by @param pos.
    inline  T* begin() const { return mData; } ///< get "iterator" pointer
    inline  T* end() const { return mEnd; } ///< get iterator end-pointer
    QPoint indexOf(T* element) const; ///< retrieve index (x/y) of the pointer element. returns -1/-1 if element is not valid.
    // special queries
    T max() const; ///< retrieve the maximum value of a grid
    T sum() const; ///< retrieve the sum of the grid
    T avg() const; ///< retrieve the average value of a grid
    /// creates a grid with lower resolution and averaged cell values.
    /// @param factor factor by which grid size is reduced (e.g. 3 -> 3x3=9 pixels are averaged to 1 result pixel)
    /// @param offsetx, offsety: start averaging with an offset from 0/0 (e.g.: x=1, y=2, factor=3: -> 1/2-3/4 -> 0/0)
    /// @return Grid with size sizeX()/factor x sizeY()/factor
    Grid<T> averaged(const int factor, const int offsetx=0, const int offsety=0) const;
    /// normalized returns a normalized grid, in a way that the sum()  = @param targetvalue.
    /// if the grid is empty or the sum is 0, no modifications are performed.
    Grid<T> normalized(const T targetvalue) const;
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
    if (isIndexValid(pos)) {
        return mData[pos.x()*mSizeY + pos.y()];
    }
    qCritical("Grid::valueAtIndex. invalid: %d/%d", pos.x(), pos.y());
    return mData[0];
}

template <class T>
const T&  Grid<T>::constValueAtIndex(const QPoint& pos) const
{
    if (isIndexValid(pos)) {
        return mData[pos.x()*mSizeY + pos.y()];
    }
    qCritical("Grid::constValueAtIndex. invalid: %d/%d", pos.x(), pos.y());
    return mData[0];
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

template <class T>
QPoint Grid<T>::indexOf(T* element) const
{
    QPoint result(-1,-1);
    if (element==NULL || element<mData || element>=end())
        return result;
    int idx = element - mData;
    result.setX( idx % mSizeY);
    result.setY( idx / mSizeY);
    return result;
}

template <class T>
T  Grid<T>::max() const
{
    T maxv = std::numeric_limits<T>::min();
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

////////////////////////////////////////////////////////////7
// global functions
////////////////////////////////////////////////////////////7

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
