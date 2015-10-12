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
#ifndef GRASSCOVER_H
#define GRASSCOVER_H

#include "expression.h"
#include "grid.h"
#include "layeredgrid.h"

class GrassCoverLayers; // forwared

// the number of steps used internally
const int GRASSCOVERSTEPS = 64000;
// define the data type that is used to store the grass-levels
// use unsigned char for 1 byte (or quint8), unsigned short int (quint16) for two bytes per pixel
#define grid_type quint16

/**
 * @brief The GrassCover class specifies the limiting effect of ground vegetation (grasses, herbs)
 * on the regeneration success of the tree species.
 * The GrassCover model is very simple and operates on a 2x2m grain.
 */
class GrassCover
{
public:
    GrassCover();
    ~GrassCover();
    void setup();
    /// set for all the pixels (LIFPixels) the corresponding grass value (in percent: 0-100)
    void setInitialValues(const QVector<float*> &LIFpixels, const int percent);

    /// main function (growth/die-off of grass cover)
    void execute();

    // access
    /// returns 'true' if the module is enabled
    bool enabled() const { return mEnabled; }
    ///
    double effect(grid_type level) const { return mEffect[level]; }
    double regenerationInhibition(QPoint &lif_index) const { return mEnabled?effect(mGrid.constValueAtIndex(lif_index)) : 0.; }
    /// retrieve the grid of current grass cover
    const Grid<grid_type> &grid() { return mGrid; }
private:
    bool mEnabled; ///< is module enabled?
    Expression mGrassPotential; ///< function defining max. grass cover [0..1] as function of the LIF pixel value
    Expression mGrassEffect; ///< equation giving probability of *prohibiting* regeneration as a function of grass level [0..1]
    int mMaxTimeLag; ///< maximum duration (years) from 0 to full cover
    double mEffect[GRASSCOVERSTEPS]; ///< effect lookup table
    Grid<grid_type> mGrid; ///< grid covering state of grass cover (in integer steps)
    int mGrowthRate; ///< max. annual growth rate of herbs and grasses (in 1/256th)
    grid_type mMaxState; ///< potential at lif=1
    GrassCoverLayers *mLayers; // visualization
};

/** Helper class manage and visualize data layers.

*/
class GrassCoverLayers: public LayeredGrid<grid_type> {
  public:
    void setGrid(const Grid<grid_type> &grid, const GrassCover *gc) { mGrid = &grid; mGrassCover=gc; }
    double value(const grid_type &data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> &names();
private:
    QVector<LayeredGridBase::LayerElement> mNames;
    const GrassCover *mGrassCover;
};

#endif // GRASSCOVER_H
