#ifndef GRID_H
#define GRID_H

#include <QPoint>
#include <QPointF>


#include <stdexcept>

/** Grid class (template).

  */
template <class T>
class Grid {
public:

    Grid();
    Grid(int cellsize, int sizex, int sizey) { mData=0; setup(cellsize, sizex, sizey); }
    ~Grid() { if (mData) delete[] mData; }

    bool setup(const float cellsize, const int sizex, const int sizey);
    void initialize(const T& value) {for( T *p = begin();p!=end(); ++p) *p=value;}

    const int sizeX() const { return mSizeX; }
    const int sizeY() const { return mSizeY; }
    const float metricSizeX() const { return mSizeX*mCellsize; }
    const float metricSizeY() const { return mSizeY*mCellsize; }
    const float cellsize() const { return mCellsize; }
    // query
    T& valueAtIndex(const QPoint& pos); /// value at position defined by indices (x,y)
    T& valueAt(const QPointF& posf); /// value at position defined by metric coordinates
    QPoint indexAt(const QPointF& pos) { return QPoint(int(pos.x() / mCellsize),  int(pos.y()/mCellsize)); } /// get index of value at position pos (metric)
    bool isIndexValid(const QPoint& pos) { return (pos.x()>=0 && pos.x()<mSizeX && pos.y()>=0 && pos.y()<mSizeY); } /// get index of value at position pos (index)
    void validate(QPoint &pos) { pos.setX( qMax(qMin(pos.x(), mSizeX-1), 0) );  pos.setY( qMax(qMin(pos.y(), mSizeY-1), 0) );} /// ensure that "pos" is a valid key. if out of range, pos is set to minimum/maximum values.
    QPointF getCellCoordinates(const QPoint &pos) { return QPointF( (pos.x()+0.5)*mCellsize, (pos.y()+0.5)*mCellsize );} /// get metric coordinates of the cells center
    inline  T* begin() { return mData; } /// get "iterator" pointer
    inline  T* end() { return &(mData[mCount]); } /// get iterator end-pointer


private:
    T* mData;
    float mCellsize; /// size of a cell in meter
    int mSizeX; /// count of cells in x-direction
    int mSizeY; /// count of cells in y-direction
    int mCount;
};

typedef Grid<float> FloatGrid;

template <class T>
Grid<T>::Grid()
{
    mData=0; mCellsize=0.f;
}

template <class T>
bool Grid<T>::setup(const float cellsize, const int sizex, const int sizey)
{
    mSizeX=sizex; mSizeY=sizey; mCellsize=(float)cellsize;
    mCount = mSizeX*mSizeY;
    if (mData)
         delete[] mData;
   if (mCount>0)
    mData = new T[mCount];
   return true;
}
template <class T>
T&  Grid<T>::valueAtIndex(const QPoint& pos)
{
    if (isIndexValid(pos)) {
        return mData[pos.x()*mSizeX + pos.y()];
    }
    throw std::logic_error("TGrid: invalid Index!");
}

template <class T>
T&  Grid<T>::valueAt(const QPointF& posf)
{
    return valueAtIndex( indexAt(posf) );
}

#endif // GRID_H
