#include "lightroom.h"

LightRoom::LightRoom()
{
}

void LightRoom::setup(const int size_x, const int size_y, const int size_z, const double cellsize)
{
    m_countX = size_x; m_countY=size_y; m_countZ=size_z;
    m_cellsize = cellsize;
    m_2dvalues.setup(cellsize, size_x, size_y);
    m_3dvalues.setup(cellsize, size_x, size_y);
    //
    int x,y;
    for (x=0;x<size_x;x++)
        for (y=0;y<size_y;y++)
            m_3dvalues[x,y].resize(size_z);

}
