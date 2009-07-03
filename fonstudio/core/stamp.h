#ifndef STAMP_H
#define STAMP_H

/** Stamp is the basic class for the FON field of a individual tree.

*/
class Stamp
{
public:
    Stamp();
    ~Stamp();
    Stamp(const int sizex, const int sizey) { setup(sizex, sizey); }
    /// get a full access pointer to internal data
    float *data() { return m_data; }
    /// get pointer to the element after the last element (iterator style)
    const float *end() const { return &m_data[m_sizex*m_sizey]; }
    /// get pointer to data item with indices x and y
    float *data(const int x, const int y) { return m_data + index(x,y); }
    /// get index (e.g. for data()[index]) for indices x and y
    int index(const int x, const int y) const { return y*m_sizex + x; }
private:
    void setup(const int sx, const int sy);
    float *m_data;
    int m_sizex;
    int m_sizey;
};

#endif // STAMP_H
