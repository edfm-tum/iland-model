#include "lightroom.h"

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
    m_countX = size_x; m_countY=size_y; m_countZ=size_z;
    m_cellsize = cellsize;
    m_2dvalues.setup(cellsize, size_x, size_y);
    m_2dvalues.initialize(0.);
    m_3dvalues.setup(cellsize, size_x, size_y);
    // setup room
    int x,y;
    for (x=0;x<size_x;x++)
        for (y=0;y<size_y;y++)
            m_3dvalues.valueAtIndex(QPoint(x,y)).resize(size_z);
    // setup hemigrids
    SolarRadiation solar;
    solar.setLatidude(latitude); // austria
    solar.setVegetationPeriod(0,367); // no. veg. period
    solar.setDiffusRadFraction(diffus_frac); // 50% diffuse radiation
    // calculate global radiation values
    solar.calculateRadMatrix(hemigridsize, m_solarGrid);
    m_shadowGrid.setup(hemigridsize); // setup size
}
