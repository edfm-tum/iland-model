#include "stamp.h"
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
   for (int i=0;i<m_size; i++) {
       in >> data;
       m_data[i]=data;
   }
}

void Stamp::save(QDataStream &out)
{
    // see StampContainer doc for file stamp binary format
   out << (qint32) m_offset;
   for (int i=0;i<m_size; i++) {
       out << m_data[i];
   }
}
