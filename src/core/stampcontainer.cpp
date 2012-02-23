/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include "stampcontainer.h"
#include "globalsettings.h"
#include "exception.h"

//constants
const int StampContainer::cBHDclassWidth=4;
const int StampContainer::cBHDclassLow = 4; ///< bhd classes start with 4: class 0 = 4-8, class1 = 8..112
const int StampContainer::cBHDclassCount = 70; ///< class count, see getKey(): for lower dbhs classes are smaller
const int StampContainer::cHDclassWidth=10;
const int StampContainer::cHDclassLow = 35; ///< hd classes offset is 35: class 0 = 35-45, class 1 = 45-55
const int StampContainer::cHDclassCount = 16; ///< class count. highest class:  185-195

// static values
Grid<float> StampContainer::m_distance;

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
/// getKey: decodes a floating point piar of dbh and hd-ratio to indices for the
/// lookup table containing pointers to the actual stamps.
void StampContainer::getKey(const float dbh, const float hd_value, int &dbh_class, int &hd_class) const
{
    hd_class = int(hd_value - cHDclassLow) / cHDclassWidth;
    dbh_class = int(dbh - cBHDclassLow) / cBHDclassWidth;
    // fixed scheme: smallest classification scheme for tree-diameters:
    // 1cm width from 4 up to 9cm,
    // 2cm bins from 10 to 18cm
    // 4cm bins starting from 20cm
    if (dbh < 10.f) {
        dbh_class = qMax(0, int(dbh-4.)); // classes from 0..5
    } else if (dbh<20.f) {
        dbh_class = 6 + int((dbh-10.f) / 2.f); // 10-12cm has index 6
    } else {
        dbh_class = 11 + int((dbh-20.f) / 4.f); // 20-24cm has index 11
    }
}

/** fill up the NULLs in the lookup map */
void StampContainer::finalizeSetup()
{
    if (!m_useLookup)
        return;
    Stamp *s;
    int h;
    int max_size=0;
    for (int b=0;b<cBHDclassCount;b++) {
        // find lowest value...
        for (h=0;h<cHDclassCount;h++) {
            s=m_lookup.valueAtIndex(b,h);
            if (s) {
                // fill up values left from this value
                for (int hfill=0;hfill<h;hfill++)
                    m_lookup.valueAtIndex(b,hfill) = s;
                break;
            }
        }
        // go to last filled cell...
        for (;h<cHDclassCount;h++) {
            if (m_lookup.valueAtIndex(b,h)==0)
                break;
            s=m_lookup.valueAtIndex(b,h);
        }
        // fill up the rest...
        for (;h<cHDclassCount;h++) {
            m_lookup.valueAtIndex(b,h)=s;
        }
        if(s)
            max_size = std::max(max_size, s->dataSize());
    }
    // distance grid
    if (m_distance.sizeX()<max_size) {
        setupDistanceGrid(max_size);
    }
    if (GlobalSettings::instance()->settings().paramValueBool("debugDumpStamps", false) )
        qDebug() << dump();
}

void StampContainer::setupDistanceGrid(const int size)
{
    const float px_size = cPxSize;
    m_distance.setup(px_size, size, size);
    float *p=m_distance.begin();
    QPoint idx;
    for (;p!=m_distance.end();++p) {
        idx = m_distance.indexOf(p);
        *p = sqrt(double(idx.x()*idx.x()) + double(idx.y()*idx.y()))*px_size;
    }
}

 void StampContainer::addStamp(Stamp* stamp, const int cls_dbh, const int cls_hd, const float crown_radius_m, const float dbh, const float hd_value)
 {
    if (m_useLookup) {
        if (cls_dbh<0 || cls_dbh>=cBHDclassCount || cls_hd<0 || cls_hd>=cHDclassCount)
            throw IException(QString("StampContainer::addStamp: Stamp out of range. dbh=%1 hd=%2.").arg(dbh).arg(hd_value));
        m_lookup.valueAtIndex(cls_dbh, cls_hd) = stamp; // save address in look up table
    } // if (useLookup)
    stamp->setCrownRadius(crown_radius_m);
    StampItem si;
    si.dbh = dbh;
    si.hd = hd_value;
    si.crown_radius = crown_radius_m;
    si.stamp = stamp;
    m_stamps.append(si); // store entry in list of stamps

 }


/** add a stamp to the internal storage.
    After loading the function finalizeSetup() must be called to ensure that gaps in the matrix get filled. */
void  StampContainer::addStamp(Stamp* stamp, const float dbh, const float hd_value, const float crown_radius)
{
    int cls_dbh, cls_hd;
    getKey(dbh, hd_value, cls_dbh, cls_hd); // decode dbh/hd-value
    addStamp(stamp, cls_dbh, cls_hd, crown_radius, dbh, hd_value); // dont set crownradius
}

void StampContainer::addReaderStamp(Stamp *stamp, const float crown_radius_m)
{
    double rest = fmod(crown_radius_m, 1.)+0.0001;
    int cls_hd = int( rest * 10 ); // 0 .. 9.99999999
    if (cls_hd>=cHDclassCount)
        cls_hd=cHDclassCount-1;
    int cls_dbh = int(crown_radius_m);
    //qDebug() << "Readerstamp r="<< crown_radius_m<<" index dbh hd:" << cls_dbh << cls_hd;
    stamp->setCrownRadius(crown_radius_m);

    // prepare special keys for reader stamps
    addStamp(stamp,cls_dbh, cls_hd, crown_radius_m, 0., 0.); // set crownradius, but not dbh/hd
}


/** retrieve a read-out-stamp. Readers depend solely on a crown radius.
Internally, readers are stored in the same lookup-table, but using a encoding/decoding trick.*/
const Stamp* StampContainer::readerStamp(const float crown_radius_m) const
{
    // Readers: from 0..10m in 50 steps???
    int cls_hd = int( (fmod(crown_radius_m, 1.)+0.0001) * 10 ); // 0 .. 9.99999999
    if (cls_hd>=cHDclassCount)
        cls_hd=cHDclassCount-1;
    int cls_bhd = int(crown_radius_m);
    const Stamp* stamp = m_lookup(cls_bhd, cls_hd);
    if (!stamp)
        qDebug() << "Stamp::readerStamp(): no stamp found for radius" << crown_radius_m;
    return stamp;
}

/** fast access for an individual stamp using a lookup table.
    the dimensions of the lookup table are defined by class-constants.
    If stamp is not found there, the more complete list of stamps is searched. */
const Stamp* StampContainer::stamp(const float bhd_cm, const float height_m) const
{

    float hd_value = 100.f * height_m / bhd_cm;

    int cls_bhd, cls_hd;
    getKey(bhd_cm, hd_value, cls_bhd, cls_hd);

    // check loopup table
    if (cls_bhd<cBHDclassCount && cls_bhd>=0 && cls_hd < cHDclassCount && cls_hd>=0) {
        const Stamp* stamp = m_lookup(cls_bhd, cls_hd);
        if (stamp)
            return stamp;
        qDebug() << "StampContainer::stamp(): not in list: bhd height:" << bhd_cm << height_m << "in" << m_fileName;
        return m_stamps.first().stamp; // default value: the first stamp in the list....
    }

    // extra work: search in list...
    //if (bhd_cm > m_maxBhd)
    if (cls_bhd<cBHDclassCount && cls_bhd>=0) {
        qDebug() << "HD for stamp out of range bhd " << bhd_cm << "and h="<< height_m << "(using smallest/largeset HD)";
        if (cls_hd>=cHDclassCount)
            return m_lookup(cls_bhd, cHDclassCount-1); // highest
        return m_lookup(cls_bhd, 0); // smallest
    }
    qDebug() << "No stamp defined for bhd " << bhd_cm << "and h="<< height_m;
    return NULL;

}

/// static factory function to create stamps with a given size
/// @param type indicates type of stamp
Stamp* StampContainer::newStamp(const Stamp::StampType type)
{
    Stamp *stamp;
    switch (type) {
       case Stamp::est4x4:  //qDebug() << "4x4stamp";
           stamp=new Stamp(4); break;

    default:
          stamp = new Stamp(int(type));
    }
    return stamp;
}


void StampContainer::attachReaderStamps(const StampContainer &source)
{
    int found=0, total=0;
    foreach (StampItem si, m_stamps) {
        const Stamp *s = source.readerStamp(si.crown_radius);
        si.stamp->setReader(const_cast<Stamp*>(s));
        if (s) found++;
        total++;
        //si.crown_radius
    }
    qDebug() << "attachReaderStamps: found" << found << "stamps of" << total;
}

void StampContainer::invert()
{
    StampItem  si;
    foreach(si, m_stamps) {
        Stamp *s =si.stamp;
        float *p = s->data();
        while (p!=s->end()) {
            *p = 1. - *p;
            p++;
        }
    }
}
/// convenience function that loads stamps directly from a single file.
void StampContainer::load(const QString &fileName)
{
    QFile readerfile(fileName);
    if (!readerfile.exists())
        throw IException(QString("The LIP stampfile %1 cannot be found!").arg(fileName));
    m_fileName = fileName;
    readerfile.open(QIODevice::ReadOnly);
    QDataStream rin(&readerfile);
    qDebug() << "loading stamp file" << fileName;
    load(rin);
    readerfile.close();
}

void StampContainer::load(QDataStream &in)
{
    qint32 type;
    qint32 count;
    float bhd, hdvalue, crownradius;
    quint32 magic;
    in >> magic;
    if (magic!=0xFEED0001)
        throw IException("StampContainer: invalid file type!");
    quint16 version;
    in >> version;
    if (version != 100)
        throw IException(QString("StampContainer: invalid file version: %1").arg(version));
    in.setVersion(QDataStream::Qt_4_5);

    in >> count; // read count of stamps
    qDebug() << count << "stamps to read";
    QString desc;
    in >> desc; // read textual description of stamp
    qDebug() << "Stamp notes:" << desc;
    m_desc = desc;
    for (int i=0;i<count;i++) {
        in >> type; // read type
        in >> bhd;
        in >> hdvalue;
        in >> crownradius;
        //qDebug() << "stamp bhd hdvalue type readsum dominance type" << bhd << hdvalue << type << readsum << domvalue << type;

        Stamp *stamp = newStamp( Stamp::StampType(type) );
        stamp->load(in);

        if (bhd > 0.f)
            addStamp(stamp, bhd, hdvalue, crownradius);
        else
            addReaderStamp(stamp, crownradius);
    }
    finalizeSetup(); // fill up lookup grid
    if (count==0)
        throw IException("no stamps loaded!");
}
/** Saves all stamps of the container to a binary stream.
  Format: * count of stamps (int32)
          * a string containing a description (free text) (QString)
      for each stamp:
      - type (enum Stamp::StampType, 4, 8, 12, 16, ...)
      - bhd of the stamp (float)
      - hd-value of the tree (float)
      - crownradius of the stamp (float) in [m]
      - the sum of values in the center of the stamp (used for read out)
      - the dominance value of the stamp
      - individual data values (Stamp::save() / Stamp::load())
      -- offset (int) no. of pixels away from center
      -- list of data items (type*type items)
      see also stamp creation (FonStudio application, MainWindow.cpp).
*/
void StampContainer::save(QDataStream &out)
{
    qint32 type;
    qint32 size = m_stamps.count();
    out << (quint32)0xFEED0001; // magic number
    out << (quint16)100; // version
    out.setVersion(QDataStream::Qt_4_5);

    out << size; // count of stamps...
    out << m_desc; // text...
    foreach(StampItem si, m_stamps) {
        type = si.stamp->dataSize();
        out << type;
        out << si.dbh;
        out << si.hd;
        out << si.crown_radius;
        si.stamp->save(out);
    }

}

QString StampContainer::dump()
{
    QString res;
    QString line;
    int x,y;
    int maxidx;
    res = QString("****** Dump of StampContainer %1 **********\r\n").arg(m_fileName);
    foreach (StampItem si, m_stamps) {
        line = QString("%5 -> size: %1 offset: %2 dbh: %3 hd-ratio: %4\r\n")
               .arg(sqrt(si.stamp->count())).arg(si.stamp->offset())
               .arg(si.dbh).arg(si.hd).arg((int)si.stamp, 0, 16);
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
