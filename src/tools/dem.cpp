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

#include "global.h"
#include "dem.h"

#include "globalsettings.h"
#include "model.h"

#include "gisgrid.h"


/// loads a DEM from a ESRI style text file.
/// internally, the DEM has always a resolution of 10m
bool DEM::loadFromFile(const QString &fileName)
{
    if (!GlobalSettings::instance()->model())
        throw IException("DEM::create10mGrid: no valid model to retrieve height grid.");

    HeightGrid *h_grid = GlobalSettings::instance()->model()->heightGrid();
    if (!h_grid || h_grid->isEmpty())
        throw IException("GisGrid::create10mGrid: no valid height grid to copy grid size.");

    GisGrid gis_grid;
    if (!gis_grid.loadFromFile(fileName))
        throw IException(QString("Unable to load DEM file %1").arg(fileName));
    // create a grid with the same size as the height grid
    // (height-grid: 10m size, covering the full extent)
    clear();
    aspect_grid.clear();
    slope_grid.clear();
    view_grid.clear();

    setup(h_grid->metricRect(),h_grid->cellsize());

    const QRectF &world = GlobalSettings::instance()->model()->extent();

    QPointF p;
    // copy the data
    for (int i=0;i<count();i++) {
        p = cellCenterPoint(indexOf(i));
        if (gis_grid.value(p) != gis_grid.noDataValue() && world.contains(p) )
            valueAtIndex(i) = gis_grid.value(p);
        else
            valueAtIndex(i) = -1;
    }

    return true;
}

/// calculate slope and aspect at a given point.
/// results are params per reference.
/// returns the height at point (x/y)
/// calculation follows: Burrough, P. A. and McDonell, R.A., 1998.Principles of Geographical Information Systems.(Oxford University Press, New York), p. 190.
/// http://uqu.edu.sa/files2/tiny_mce/plugins/filemanager/files/4280125/Principles%20of%20Geographical%20Information%20Systems.pdf
/// @param point metric coordinates of point to derive orientation
/// @param rslope_angle RESULTING (passed by reference) slope angle as percentage (i.e: 1:=45°)
/// @param rslope_aspect RESULTING slope direction in degrees (0: North, 90: east, 180: south, 270: west)
float DEM::orientation(const QPointF &point, float &rslope_angle, float &rslope_aspect)
{
    QPoint pt = indexAt(point);
    if (pt.x()>0 && pt.x()<sizeX()+1 && pt.y()>0 && pt.y()<sizeY()-1) {
        float *p = ptr(pt.x(), pt.y());
        float z2 = *(p-sizeX());
        float z4 = *(p-1);
        float z6 = *(p+1);
        float z8 = *(p+sizeX());
        float g = (-z4 + z6) / (2*cellsize());
        float h = (z2 - z8) / (2*cellsize());

        if (z2<=0. || z4<=0. || z6<=0. || z8<=0) {
            rslope_angle = 0.;
            rslope_aspect = 0.;
            return *p;
        }
        rslope_angle = sqrt(g*g + h*h);
        // atan2: returns -pi : +pi
        // North: -pi/2, east: 0, south: +pi/2, west: -pi/+pi
        float aspect = atan2(-h, -g);
        // transform to degree:
        // north: 0, east: 90, south: 180, west: 270
        aspect = aspect * 180. / M_PI + 360. + 90.;
        aspect = fmod(aspect, 360.f);

        rslope_aspect = aspect;
        return *p;
    } else {
        rslope_angle = 0.;
        rslope_aspect = 0.;
        return 0.;
    }
}

void DEM::createSlopeGrid()
{
    if (slope_grid.isEmpty()) {
        // setup custom grids with the same size as this DEM
        slope_grid.setup(*this);
        view_grid.setup(*this);
        aspect_grid.setup(*this);
    } else {
        return;
    }
    float *slope = slope_grid.begin();
    float *view = view_grid.begin();
    float *aspect = aspect_grid.begin();

    // use fixed values for azimuth (315) and angle (45°) and calculate
    // norm vectors
    float sun_x = cos(315. * M_PI/180.) * cos(45.*M_PI/180.);
    float sun_y = sin(315. * M_PI/180.) * cos(45.*M_PI/180.);
    float sun_z = sin(45.*M_PI/180.);

    float a_x, a_y, a_z;
    for (float *p = begin(); p!=end(); ++p, ++slope, ++view, ++aspect) {
        QPointF pt = cellCenterPoint(indexOf(p));
        float height = orientation(pt, *slope, *aspect);
        // calculate the view value:
        if (height>0) {
            float h = atan(*slope);
            a_x = cos(*aspect * M_PI/180.) * cos(h);
            a_y = sin(*aspect * M_PI/180.) * cos(h);
            a_z = sin(h);

            // use the scalar product to calculate the angle, and then
            // transform from [-1,1] to [0,1]
            *view = (a_x*sun_x + a_y*sun_y + a_z*sun_z + 1.)/2.;
        } else {
            *view = 0.;
        }
    }

}
