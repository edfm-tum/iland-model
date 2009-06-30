#ifndef LIGHTROOM_H
#define LIGHTROOM_H

#include "core/grid.h"
#include "core/solarradiation.h"
#include "core/hemigrid.h"
/** virtual room to do some light-experiments.
  The basic use of this class is to derive the size/pattern of the light-influence FON for a single tree.
  It uses SolarRadiation for the calculation of global radiation and HemiGrid to calculate and store the results.
  This calculation is done for each node of a 3D space and afterwards accumulated into a 2D pattern. */
class LightRoom
{
public:
    LightRoom();
    /// setup the spatial grid.
    void setup(const int size_x, const int size_y, const int size_z,
               const double cellsize, const double hemigridsize,
               const double latitude=48., const double diffus_frac=0.5);
private:
    Grid< QVector<float> > m_3dvalues; ///< storage for resulting 3d light values
    FloatGrid m_2dvalues; ///< resulting 2d pattern (derived from 3dvalues)
    int m_countX; ///< size of the grid in x-direction
    int m_countY;
    int m_countZ;
    double m_cellsize; ///< length of the side of one cell (equal for all 3 directions)
    HemiGrid m_solarGrid; ///< grid used for solar radiation (direct + diffus)
    HemiGrid m_shadowGrid; ///< grid used for shadow calculations
};

#endif // LIGHTROOM_H
