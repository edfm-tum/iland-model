#include "lightroom.h"
#include "tools/helper.h"

LightRoom::LightRoom()
{
}

/** setup routine.
  sets up datastructures (3d space, hemi grids)
  @param size_x count of cells in x direction
  @param size_y count of cells in y direction
  @param size_z count of cells in z direction
  @param cellsize metric length of cells (used for all 3 dimensions).
  @param hemigridsize size of hemigrid-cells (in degrees).
  @param latitude lat. in degrees.
  @param diffus_frac fraction [0..1] of diffus radiation of global radiation. */
void LightRoom::setup(const int size_x, const int size_y, const int size_z,
                      const double cellsize, const double hemigridsize,
                      const double latitude, const double diffus_frac)
{
    DebugTimer t1("setup of lightroom");
    m_countX = size_x; m_countY=size_y; m_countZ=size_z;
    m_cellsize = cellsize;
    if (m_countX%2==0) m_countX++; // make uneven
    if (m_countY%2==0) m_countY++;
    QRectF rect(-m_countX/2*cellsize, -m_countY/2*cellsize, m_countX*cellsize, m_countY*cellsize);
    m_2dvalues.setup(rect, cellsize);
    m_2dvalues.initialize(0.);
    m_3dvalues.setup(rect, cellsize);
    // setup room
    int x,y;
    for (x=0;x<m_countX;x++)
        for (y=0;y<m_countY;y++)
            m_3dvalues.valueAtIndex(QPoint(x,y)).resize(size_z);
    // setup hemigrids
    SolarRadiation solar;
    solar.setLatidude(latitude); // austria
    solar.setVegetationPeriod(0,367); // no. veg. period
    solar.setDiffusRadFraction(diffus_frac); // 50% diffuse radiation
    // calculate global radiation values
    DebugTimer t2("calculate solar radiation matrix");
    solar.calculateRadMatrix(hemigridsize, m_solarGrid);
    t2.showElapsed();
    m_shadowGrid.setup(hemigridsize); // setup size
}


//////////////////////////////////////////////////////////
// Lightroom Object
//////////////////////////////////////////////////////////

void LightRoomObject::setuptree(const double height, const double crownheight, const QString &formula)
{

}

bool LightRoomObject::hittest(const double p_x, const double p_y, const double p_z,
                              const double azimuth_rad, const double elevation_rad)
{

}
