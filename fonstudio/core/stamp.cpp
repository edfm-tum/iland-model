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
