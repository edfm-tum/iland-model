#include "stamp.h"

Stamp::Stamp()
{
    m_data=0;
}
Stamp::~Stamp()
{
   if( m_data)
       delete m_data;
}

void Stamp::setup(const int sx, const int sy)
{
    int c=sx*sy;
    m_sizex=sx;
    m_sizey=sy;
    m_data=new float[c];
    for (int i=0;i<c;i++)
        m_data[i]=0.;
}
