#include "stampcontainer.h"

//constants
const int StampContainer::cBHDclassWidth=4;
const int StampContainer::cHDclassWidth=10;
const int StampContainer::cBHDclassLow = 2; ///< bhd classes start with 2: class 0 = 2..6, class1 = 6..10
const int StampContainer::cHDclassLow = 40; ///< hd classes offset is 40: class 0 = 40-50, class 1 = 50-60
const int StampContainer::cBHDclassCount = 50; ///< class count, 50: highest class = 50*4 +- 2 = 198 - 202
const int StampContainer::cHDclassCount = 10; ///< class count. highest class: 140-150


StampContainer::StampContainer()
{
    //
    m_lookup.setup(1., // cellsize
                   cBHDclassCount, // count x
                   cHDclassCount); // count y
    m_lookup.initialize(NULL);
    m_maxBhd = -1;
    m_useLookup = true;
}

StampContainer::~StampContainer()
{
    // delete stamps.
    while (!m_stamps.isEmpty())
        delete m_stamps.takeLast().stamp;

}

int StampContainer::getKey(const float bhd, const float hd_value)
{
    int cls_bhd = int(bhd / cBHDclassWidth) - cBHDclassLow;
    int cls_hd = int(hd_value / cHDclassWidth) - cHDclassLow;
    return cls_bhd * 1000 + cls_hd;
}

/** add a stamp to the internal storage.
    This function must be called ordered, increasing bhds and hd-values.
    for each line (i.e. same bhd-class) the left and the right margin of hd-values are "filled up":
    e.g. x x x x 3 4 5 x x ---->  3 3 3 3 3 4 5 5 5. */
int  StampContainer::addStamp(Stamp* stamp, const float bhd, const float hd_value)
{
    int key = getKey(bhd, hd_value);
    if (m_useLookup) {
        int cls_bhd = int(bhd / cBHDclassWidth) - cBHDclassLow;
        int cls_hd = int(hd_value / cHDclassWidth) - cHDclassLow;
        if (cls_bhd > m_maxBhd) {
            // start a new band of hd-values....
            // finish last line...
            Stamp *s;
            for (int hd=0;hd<cHDclassCount;++hd) {
                if (m_lookup(cls_bhd, hd))
                    s=m_lookup(cls_bhd, hd); // save last value...
                else
                    m_lookup.valueAt(cls_bhd, hd) = s; // write values
            }

            m_maxBhd = cls_bhd;
            // fill up first line
            for (int hd=0; hd<cls_hd;hd++)
                m_lookup.valueAt(cls_bhd, hd) = stamp;
        }

        m_lookup.valueAtIndex(cls_bhd, cls_hd) = stamp; // save address in look up table
    } // if (useLookup)

    StampItem si;
    si.bhd = bhd;
    si.hd = hd_value;
    si.stamp = stamp;
    m_stamps.append(si); // store entry in list of stamps
    return key;

}

/** fast access for an individual stamp using a lookup table.
    the dimensions of the lookup table are defined by class-constants.
    If stamp is not found there, the more complete list of stamps is searched. */
const Stamp* StampContainer::getStamp(const float bhd_cm, const float height_m)
{

    float hd_value = 100 * height_m / bhd_cm;
    int cls_bhd = int(bhd_cm / cBHDclassWidth) - cBHDclassLow;
    int cls_hd = int(hd_value / cHDclassWidth) - cHDclassLow;
    // check loopup table
    if (cls_bhd<cBHDclassCount && cls_bhd>=0 && cls_hd < cHDclassCount && cls_bhd>=0)
        return m_lookup(cls_bhd, cls_hd);

    // extra work: search in list...
    //if (bhd_cm > m_maxBhd)
    qDebug() << "No stamp defined for bhd " << bhd_cm << "and h="<< height_m;
    return NULL;

}

/// static factory function to create stamps with a given size
/// @param type indicates type of stamp
Stamp* StampContainer::newStamp(const Stamp::StampType type)
{
    Stamp *stamp;
    switch (type) {
       case Stamp::est4x4:  qDebug() << "4x4stamp"; stamp=new Stamp(4); break;
    default:
          stamp = new Stamp(int(type));
    }
    return stamp;
}

void StampContainer::load(QDataStream &in)
{
    qint32 type;
    qint32 count;
    float bhd, hdvalue;
    in >> count; // read count of stamps
    qDebug() << count << "stamps to read";
    for (int i=0;i<count;i++) {
        in >> type; // read type
        in >> bhd;
        in >> hdvalue;
        Stamp *stamp = newStamp( Stamp::StampType(type) );
        stamp->load(in);
        addStamp(stamp, bhd, hdvalue);
    }
}
/** Saves all stamps of the container to a binary stream.
  Format: * count of stamps (int32)
      for each stamp:
      - type (enum Stamp::StampType, 4, 8, 12, 16, ...)
      - bhd of the stamp (float)
      - height of the tree (float)
      - individual data values (Stamp::save() / Stamp::load())
      -- offset (int) no. of pixels away from center
      -- list of data items (type*type items)
      see also stamp creation (FonStudio application, MainWindow.cpp).
*/
void StampContainer::save(QDataStream &out)
{
    qint32 type;
    qint32 size = m_stamps.count();
    out >> size;
    foreach(StampItem si, m_stamps) {
        type = si.stamp->count();
        out >> type;
        out >> si.bhd;
        out >> si.hd;
        si.stamp->save(out);
    }

}
