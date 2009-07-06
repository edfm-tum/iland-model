#ifndef STAMP_H
#define STAMP_H

#include <QtCore>
/** Stamp is the basic class for the FON field of a individual tree.

*/
class Stamp
{
public:
    /// @enum StampType defines different grid sizes for stamps (4x4 floats, ... 48x48 floats).
    /// the numeric value indicates also the size of the grid.
    enum StampType { est4x4=4, est8x8=8, est12x12=12, est16x16=16, est24x24=24, est32x32=32, est48x48=48 };
    Stamp();
    ~Stamp();
    Stamp(const int size) { setup(size); }
    const int count() const { return m_size; }
    /// get a full access pointer to internal data
    float *data() { return m_data; }
    /// get pointer to the element after the last element (iterator style)
    const float *end() const { return &m_data[m_size*m_size]; }
    /// get pointer to data item with indices x and y
    float *data(const int x, const int y) { return m_data + index(x,y); }
    /// get index (e.g. for data()[index]) for indices x and y
    int index(const int x, const int y) const { return y*m_size + x; }
    // loading/saving
    void loadFromFile(const QString &fileName);
    void load(QDataStream &in); ///< load from stream (predefined binary structure)
    void save(QDataStream &out); ///< save to stream (predefined binary structure)
private:
    void setup(const int size);
    float *m_data;
    int m_size;
    int m_offset;
};

#endif // STAMP_H
