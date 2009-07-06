#ifndef STAMPCONTAINER_H
#define STAMPCONTAINER_H

#include "stamp.h"
#include "grid.h"

/** Collection of @class Stamp for one tree species.
  Per species several stamps are stored (different BHD, different HD relations). This class
  encapsulates storage and access to these stamps. The design goal is to deliver high
  access speeds for the "getStamp()" method.
  Use getStamp(bhd, hd) or getStamp(bhd, height) to access. */
class StampContainer
{
public:
    StampContainer();
    ~StampContainer();
    void useLookup(const bool use) { m_useLookup = use; }
    /// addStamp() add a pre-allocated stamp @param stamp to internal collection. Caller must allocate stamp on the heap,
    /// freeing is done by this class.
    int addStamp(Stamp* stamp, const float bhd, const float hd_value);
    const Stamp* getStamp(const float bhd_cm, const float height_m);
    /// save the content of the StampContainer to the output stream (binary encoding)
    void save(QDataStream &out);
    /// load and setup from a data stream....
    void load(QDataStream &in);
private:
    static const int cBHDclassWidth;
    static const int cHDclassWidth;
    static const int cBHDclassLow; ///< bhd classes start with 2: class 0 = 2..6, class1 = 6..10
    static const int cHDclassLow; ///< hd classes offset is 40: class 0 = 40-50, class 1 = 50-60
    static const int cBHDclassCount; ///< class count, 50: highest class = 50*4 +- 2 = 198 - 202
    static const int cHDclassCount; ///< class count. highest class: 140-150
    struct StampItem {
        Stamp* stamp;
        float bhd;
        float hd;
    };
    static Stamp* newStamp(const Stamp::StampType type);
    inline int getKey(const float bhd, const float hd_value);
    int m_maxBhd;
    bool m_useLookup; // use lookup table?
    QList<StampItem> m_stamps;
    Grid<Stamp*> m_lookup;
};

#endif // STAMPCONTAINER_H
