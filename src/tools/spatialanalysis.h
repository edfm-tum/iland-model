#ifndef SPATIALANALYSIS_H
#define SPATIALANALYSIS_H

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
#include "grid.h"

#include <QObject>
class RumpleIndex; // forward
/**
 * @brief The SpatialAnalysis class is the scripting class related to extra spatial analysis functions.
 *
 */
class SpatialAnalysis: public QObject
{
    Q_OBJECT

public:
    SpatialAnalysis(QObject *parent=0): QObject(parent), mRumple(0) {}
public slots:
    // API for Javascript
    double rumpleIndex();
private:
    RumpleIndex *mRumple;

};

/** The RumpleIndex is a spatial index relating surface area to ground area.
 *  In forestry, it is a indicator of vertical heterogeneity. In iLand, the Rumple Index is
 *  the variability of the maximum tree height on 10m level (i.e. the "Height"-Grid).
 *  The RumpleIndex is calculated for each resource unit and also for the full project area.
 **/
class RumpleIndex
{
public:
    RumpleIndex(): mLastYear(-1) {}
    void setup(); ///< sets up the grid
    void calculate(); ///< calculates (or re-calculates) index values
    double value(const bool force_recalculate=false); ///< calculates rumple index for the full project area
    const FloatGrid &rumpleGrid() const { return mRumpleGrid; } ///< return the rumple index for the full project area
    //
    double test_triangle_area();
private:
    double calculateSurfaceArea(const float *heights, const float cellsize);
    FloatGrid mRumpleGrid;
    double mRumpleIndex;
    int mLastYear;
};

#endif // SPATIALANALYSIS_H
