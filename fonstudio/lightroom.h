#ifndef LIGHTROOM_H
#define LIGHTROOM_H

#include "core/grid.h"
#include "core/solarradiation.h"
#include "core/hemigrid.h"
#include "tools/expression.h"

class LightRoomObject
{
public:
    LightRoomObject(): m_radiusFormula(0) {}
    ~LightRoomObject();
    /** Test if the ray starting at "p" hits the object.
        the ray has direction azimuth/elevation and starts from the position denoted by p_x, p_y and p_z
        @return true if the object is hit.*/
    bool hittest(const double p_x, const double p_y, const double p_z,
                 const double azimuth_rad, const double elevation_rad);
    /** sets up a tree as the obstacle.
        @param height treehight in meter
        @param crownheight height of the start of crown (above ground) in meter
        @param formula (as string representation) that yields the radius as f(relativeheight).
              the variable of the formula is 0 for ground 1 for tree top. */
    void setuptree(const double height, const double crownheight, const QString &formula);
private:
    Expression *m_radiusFormula;
    double m_baseradius; ///< maximum radius of the crown (at the bottom of the crown) [m]
    double m_height; ///< treehight [m]
    double m_crownheight; ///< height of the beginning of the living crown [m]
};

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
