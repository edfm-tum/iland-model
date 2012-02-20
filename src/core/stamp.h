#ifndef STAMP_H
#define STAMP_H

#include "grid.h"
#include <QtCore>
/** Stamp is the basic class for the FON field of a individual tree.

*/
class Stamp
{
public:
    /// @enum StampType defines different grid sizes for stamps (4x4 floats, ... 48x48 floats).
    /// the numeric value indicates also the size of the grid.
    enum StampType { est4x4=4, est8x8=8, est12x12=12, est16x16=16, est24x24=24, est32x32=32, est48x48=48, est64x64=64 };
    Stamp();
    ~Stamp();
    Stamp(const int size):m_data(NULL) { setup(size); }
    void setOffset(const int offset) { m_offset = offset; }
    int offset() const { return m_offset; } ///< delta between edge of the stamp and the logical center point (of the tree). e.g. a 5x5 stamp in an 8x8-grid has an offset from 2.
    int count() const { return m_size*m_size; } ///< count of pixels (rectangle)
    int size() const { return m_offset*2+1; } ///< logical size of the stamp
    int dataSize() const { return m_size; } ///< internal size of the stamp; e.g. 4 -> 4x4 stamp with 16 pixels.
    /// get a full access pointer to internal data
    float *data() { return m_data; }
    /// get pointer to the element after the last element (iterator style)
    const float *end() const { return &m_data[m_size*m_size]; }
    /// get pointer to data item with indices x and y
    float *data(const int x, const int y) const { return m_data + index(x,y); }
    void setData(const int x, const int y, const float value) { *data(x,y) = value; }
    /// get index (e.g. for data()[index]) for indices x and y
    int index(const int x, const int y) const { return y*m_size + x; }
    inline float operator()(const int x, const int y) const { return *data(x,y); }
    inline float offsetValue(const int x, const int y, const int offset) const { return *data(x+offset, y+offset); }
    const Stamp *reader() const { return m_reader; }
    void setReader(Stamp *reader) { m_reader = reader; setCrownRadius(reader->crownRadius()); /*calculates also the Area*/ }

    // property crown radius
    float crownRadius() const { return m_crownRadius; }
    float crownArea() const { return m_crownArea; }
    void setCrownRadius(const float r) { m_crownRadius = r; m_crownArea=r*r*M_PI; }
    float distanceToCenter(const int ix, const int iy) const;// { return StampContainer::distanceGrid().constValueAtIndex(abs(ix-m_offset), abs(iy-m_offset)); }
    // loading/saving
    void loadFromFile(const QString &fileName);
    void load(QDataStream &in); ///< load from stream (predefined binary structure)
    void save(QDataStream &out); ///< save to stream (predefined binary structure)
    QString dump() const;
private:
    void setup(const int size);
    float *m_data;
    float m_crownRadius;
    float m_crownArea;
    int m_size;
    int m_offset;
    Stamp *m_reader; ///< pointer to the appropriate reader stamp (if available)
};

// global functions

/// create a stamp from a FloatGrid with any size
/// @param grid source grid. It is assumed the actual stamp data is around the center point and the grid has an uneven size (e.g 13x13 or 25x25)
/// @param width number of pixels that should actually be used. e.g: grid 25x25, width=7 -> data is located from 9/9 to 16/16 (12+-3)
/// @return a stamp created on the heap with the fitting size. The data rect is aligned to 0/0. above example: stamp will be 8x8, with a 7x7-data-block from 0/0 to 6/6.
Stamp *stampFromGrid(const FloatGrid& grid, const int width);

#endif // STAMP_H
