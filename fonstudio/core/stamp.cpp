#include "stamp.h"
#include "core/grid.h"
#include "core/stampcontainer.h"
#include "../tools/helper.h"
Stamp::Stamp()
{
    m_data=0;
}
Stamp::~Stamp()
{
   if( m_data)
       delete m_data;
}

void Stamp::setup(const int size)
{
    int c=size*size;
    m_size=size;
    m_offset=0;
    if (m_data)
        delete[] m_data;
    m_data=new float[c];
    for (int i=0;i<c;i++)
        m_data[i]=0.;
}

void Stamp::loadFromFile(const QString &fileName)
{
    QString txt = Helper::loadTextFile(fileName);
    QStringList lines = txt.split("\n");

    setup(lines.count());
    int l=0;
    foreach(QString line, lines) {
        QStringList cols=line.split(";");
        if (cols.count() != lines.count())
            MSGRETURN("Stamp::loadFromFile: invalid count of rows/cols.");
        for (int i=0;i<cols.count();i++)
            *data(i,l)=cols[i].toFloat();
        l++;
    }
}

// load from stream....
void Stamp::load(QDataStream &in)
{
   // see StampContainer doc for file stamp binary format
   qint32 offset;
   in >> offset;
   m_offset = offset;
   // load data
   float data;
   for (int i=0;i<count(); i++) {
       in >> data;
       m_data[i]=data;
   }
}

void Stamp::save(QDataStream &out)
{
    // see StampContainer doc for file stamp binary format
   out << (qint32) m_offset;
   for (int i=0;i<count(); i++) {
       out << m_data[i];
   }
}


Stamp *stampFromGrid(const FloatGrid& grid, const int width)
{
    Stamp::StampType type=Stamp::est4x4;
    int c = grid.sizeX(); // total size of input grid
    if (c%2==0 || width%2==0) {
        qDebug() << "both grid and width should be uneven!!! returning NULL.";
        return NULL;
    }

    if (width<=4) type = Stamp::est4x4;
    else if (width<=8) type = Stamp::est8x8;
    else if (width<=12) type = Stamp::est12x12;
    else if (width<=16) type = Stamp::est16x16;
    else if (width<=24) type = Stamp::est24x24;
    else if (width<=32) type = Stamp::est32x32;
    else type = Stamp::est48x48;

    Stamp *stamp = StampContainer::newStamp(type);
    stamp->setOffset(width/2);
    int coff = c/2 - width/2; // e.g.: grid=25, width=7 -> coff = 12 - 3 = 9
    int x,y;
    for (x=0;x<width; x++)
        for (y=0; y<width; y++)
            stamp->setData(x,y, grid(coff+x, coff+y) ); // copy data (from a different rectangle)
    return stamp;

}
