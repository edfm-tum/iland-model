#ifndef STAMPCONTAINER_H
#define STAMPCONTAINER_H

#include "stamp.h"
#include "grid.h"

/** Collection of @class Stamp for one tree species.
  Per species several stamps are stored (different BHD, different HD relations). This class
  encapsulates storage and access to these stamps. The design goal is to deliver high
  access speeds for the "stamp()" method.
  Use getStamp(bhd, hd) or getStamp(bhd, height) to access. */
class StampContainer
{
public:
    StampContainer();
    ~StampContainer();
    void useLookup(const bool use) { m_useLookup = use; }
    /// addStamp() add a pre-allocated stamp @param stamp to internal collection. Caller must allocate stamp on the heap,
    /// freeing is done by this class.
    void addStamp(Stamp* stamp, const float dbh, const float hd_value);
    void addReaderStamp(Stamp *stamp, const float crown_radius_m);
    const Stamp* stamp(const float bhd_cm, const float height_m) const;
    const Stamp* readerStamp(const float crown_radius_m) const; ///< retrieve reader-stamp. @param radius of crown in m. @return the appropriate stamp or NULL if not found.
    const int count() const { return m_stamps.count(); }
    /// save the content of the StampContainer to the output stream (binary encoding)
    void save(QDataStream &out);
    /// load the content of the StampContainer to the output stream (binary encoding)
    void load(QDataStream &in);

    /** factory creation function for stamps of different size.
        newStamp() creates new Stamp-Objects on the heap with a given type (see @enum Stamp::StampType).*/
    static Stamp* newStamp(const Stamp::StampType type);
    /** this functions attaches the appropriate reader (dep. on crown radius) to each stamp of the container.
        The reader-stamp is returned by a call to the reader()-function of the Stamp itself.
        @param Container holding the reader stamps.*/
    void attachReaderStamps(const StampContainer &source);
    void invert(); ///< invert stamps (value = 1. - value) (for multiplicative overlay)
    // description
    const QString &description() { return m_desc; }
    void setDescription(const QString s) { m_desc = s; }
    QString dump();

private:
    void finalizeSetup(); ///< complete lookup-grid by filling up zero values

    static const int cBHDclassWidth;
    static const int cHDclassWidth;
    static const int cBHDclassLow; ///< bhd classes start with 4: class 0 = 4..8, class1 = 8..12
    static const int cHDclassLow; ///< hd classes offset is 40: class 0 = 40-50, class 1 = 50-60
    static const int cBHDclassCount; ///< class count, 50: highest class = 50*4 +- 2 = 198 - 202
    static const int cHDclassCount; ///< class count. highest class: 140-150
    struct StampItem {
        Stamp* stamp;
        float dbh;
        float hd;
        float crown_radius;
    };
    inline void getKey(const float dbh, const float hd_value, int &dbh_class, int &hd_class) const;
    void addStamp(Stamp* stamp, const int cls_dbh, const int cls_hd, const float crown_radius_m, const float dbh, const float hd_value);
    int m_maxBhd;
    bool m_useLookup; // use lookup table?
    QList<StampItem> m_stamps;
    Grid<Stamp*> m_lookup;
    QString m_desc;
};

#endif // STAMPCONTAINER_H
