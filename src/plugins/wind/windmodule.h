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
#ifndef WINDMODULE_H
#define WINDMODULE_H
#include "grid.h"
#include "layeredgrid.h"

#include <QHash>

class Tree; // forward
class Species; // forward

/** data structure for a single wind cell (usually 10x10m).
  @ingroup windmodule

*/
class WindCell {
public:
    WindCell() { clear(); }
    void clear() { height = edge = 0.f; }
    bool isValid() const { return height<9999.f; } ///< returns true if the pixel is on the valid project area
    float height; ///< top height (m).
    float edge; ///< maximum difference to neighboring cells (m)
};

/** Helper class manage and visualize data layers related to fire.
  @ingroup firemodule
*/
class WindLayers: public LayeredGrid<WindCell> {
  public:
    void setGrid(const Grid<WindCell> &grid) { mGrid = &grid; }
    double value(const WindCell& data, const int index) const;
    const QStringList names() const;
};

/** Species parameters that are specific to the wind module
  */
struct WindSpeciesParameters
{
    WindSpeciesParameters():  crown_area_factor(0.5), crown_length(0.5), Creg(111), MOR(30.6), wet_biomass_factor(1.86) {}
    double crown_area_factor; // empirical factor related to the crown shape (fraction of crown shape compared to rectangle)
    double crown_length; // crown length of the tree (fraction of tree height)
    double Creg; // Nm/kg, critical turning coefficient from tree pulling
    double MOR; // MPa, modulus of rupture
    double wet_biomass_factor; // conversion factor between dry and wet biomass (wet = dry*factor)
};

/** @class WindModule
    @ingroup windmodule
    The WindModule is the disturbance module for simulation wind and windthrow in iLand.  */
class WindModule
{
public:
    WindModule();
    static double cellsize() { return 10.; }
    /// the general setup routine after starting iland
    void setup();

    /// main function of the disturbance module
    void run();

    // test functions
    void testFetch(double degree_direction);
    void testEffect();
private:
    void initWindGrid(); ///< load state from iland main module
    void detectEdges(); ///< detect all pixels that are higher than the surrounding and therefore are likely candidates for damage
    /// find distance to the next pixels that give shelter
    bool checkFetch(const int startx, const int starty, const double direction, const double max_distance, const double threshold) ;
    /// perform the wind effect calculations for a given grid cell
    bool calculateEffect(const QPoint position, WindCell *cell);
    ///
    double calculateWindSpeed(const Tree *tree, const int n_trees, const double wind_speed_10);
    // helping functions
    void loadSpeciesParameter(const QString &table_name);
    const WindSpeciesParameters &speciesParameter(const Species *s);
    Grid<WindCell> mGrid; ///< wind grid (10x10m)
    WindLayers mWindLayers; ///< helping structure
    // species parameters for the wind module
    QHash<const Species*, WindSpeciesParameters> mSpeciesParameters;
};



#endif // WINDMODULE_H
