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
    //qDebug() << "grid after init" << gridToString(m_lookup);
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
    int cls_bhd = int(bhd - cBHDclassLow) / cBHDclassWidth;
    int cls_hd = int(hd_value - cHDclassLow) / cHDclassWidth;

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
        int cls_bhd = int(bhd - cBHDclassLow) / cBHDclassWidth;
        int cls_hd = int(hd_value - cHDclassLow) / cHDclassWidth;
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

int StampContainer::addReaderStamp(Stamp *stamp, const float crown_radius_m)
{
    double rest = crown_radius_m - floor(crown_radius_m) + 0.001;
    int cls_hd = int( rest * cHDclassCount ); // 0 .. 9.99999999
    if (cls_hd>=cHDclassCount)
        cls_hd=cHDclassCount-1;
    int cls_bhd = int(crown_radius_m);
    float bhd = cBHDclassWidth * cls_bhd + cBHDclassLow;
    float hd = cHDclassWidth * cls_hd + cHDclassLow;
    qDebug() << "reader stamp radius" << crown_radius_m << "mapped to" << bhd << "bhd and hd=" << hd << "classes bhd hd:" << cls_bhd << cls_hd;
    return addStamp(stamp, bhd, hd);

 }


/** retrieve a read-out-stamp. Readers depend solely on a crown radius.
Internally, readers are stored in the same lookup-table, but using a encoding/decoding trick.*/
const Stamp* StampContainer::readerStamp(const float crown_radius_m) const
{
    // Readers: from 0..10m in 50 steps???
    int cls_hd = int( (fmod(crown_radius_m, 1.)+0.001) * cHDclassCount ); // 0 .. 9.99999999
    if (cls_hd>=cHDclassCount)
        cls_hd=cHDclassCount-1;
    int cls_bhd = int(crown_radius_m);
    const Stamp* stamp = m_lookup(cls_bhd, cls_hd);
    return stamp;
}

/** fast access for an individual stamp using a lookup table.
    the dimensions of the lookup table are defined by class-constants.
    If stamp is not found there, the more complete list of stamps is searched. */
const Stamp* StampContainer::stamp(const float bhd_cm, const float height_m) const
{

    float hd_value = 100.f * height_m / bhd_cm;
    int cls_bhd = int(bhd_cm - cBHDclassLow) / cBHDclassWidth;
    int cls_hd = int(hd_value - cHDclassLow) / cHDclassWidth;


    // check loopup table
    if (cls_bhd<cBHDclassCount && cls_bhd>=0 && cls_hd < cHDclassCount && cls_bhd>=0) {
        const Stamp* stamp = m_lookup(cls_bhd, cls_hd);
        if (stamp)
            return stamp;
        return m_stamps.first().stamp; // default value: the first stamp in the list....
    }

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
    float bhd, hdvalue, readsum;
    in >> count; // read count of stamps
    qDebug() << count << "stamps to read";
    for (int i=0;i<count;i++) {
        in >> type; // read type
        in >> bhd;
        in >> hdvalue;
        in >> readsum;
        qDebug() << "stamp bhd hdvalue type" << bhd << hdvalue << type;
        Stamp *stamp = newStamp( Stamp::StampType(type) );
        stamp->load(in);
        stamp->setReadSum(readsum);
        addStamp(stamp, bhd, hdvalue);
    }
}
/** Saves all stamps of the container to a binary stream.
  Format: * count of stamps (int32)
      for each stamp:
      - type (enum Stamp::StampType, 4, 8, 12, 16, ...)
      - bhd of the stamp (float)
      - hd-value of the tree (float)
      - the sum of values in the center of the stamp
      - individual data values (Stamp::save() / Stamp::load())
      -- offset (int) no. of pixels away from center
      -- list of data items (type*type items)
      see also stamp creation (FonStudio application, MainWindow.cpp).
*/
void StampContainer::save(QDataStream &out)
{
    qint32 type;
    qint32 size = m_stamps.count();
    out << size;
    foreach(StampItem si, m_stamps) {
        type = si.stamp->dataSize();
        out << type;
        out << si.bhd;
        out << si.hd;
        out << si.stamp->readSum();
        si.stamp->save(out);
    }

}

QString StampContainer::dump()
{
    QString res;
    QString line;
    int x,y;
    int maxidx;
    foreach (StampItem si, m_stamps) {
        line = QString("%5 -> size: %1 offset: %2 dbh: %3 hd-ratio: %4\r\n")
               .arg(sqrt(si.stamp->count())).arg(si.stamp->offset())
               .arg(si.bhd).arg(si.hd).arg((int)si.stamp, 0, 16);
        // add data....
        maxidx = 2*si.stamp->offset() + 1;
        for (y=0;y<maxidx;++y)  {
            for (x=0;x<maxidx;++x)  {
                line+= QString::number(*si.stamp->data(x,y)) + " ";
            }
            line+="\r\n";
        }
        line+="==============================================\r\n";
        res+=line;
    }
    res+= "Dump of lookup map\r\n=====================\r\n";
    for (Stamp **s = m_lookup.begin(); s!=m_lookup.end(); ++s) {
        if (*s)
         res += QString("P: x/y: %1/%2 addr %3\r\n").arg( m_lookup.indexOf(s).x()).arg(m_lookup.indexOf(s).y()).arg((int)*s, 0, 16);
    }
    res+="\r\n" + gridToString(m_lookup);
    return res;
}
