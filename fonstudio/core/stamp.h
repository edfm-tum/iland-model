#ifndef STAMP_H
#define STAMP_H

#include <QtCore>
/** Stamp is the basic class for the FON field of a individual tree.

*/
class Stamp
{
public:
    Stamp();
    ~Stamp();
    Stamp(const int size) { setup(size); }
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
private:
    void setup(const int size);
    float *m_data;
    int m_size;
};

#endif // STAMP_H
