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
#include "spatialanalysis.h"

#include "globalsettings.h"
#include "model.h"
#include "helper.h"

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
void SpatialAnalysis::addToScriptEngine()
{
    SpatialAnalysis *spati = new SpatialAnalysis;
    QScriptValue v = GlobalSettings::instance()->scriptEngine()->newQObject(spati, QScriptEngine::ScriptOwnership);
    GlobalSettings::instance()->scriptEngine()->globalObject().setProperty("SpatialAnalysis", v);
}

SpatialAnalysis::~SpatialAnalysis()
{
    if (mRumple)
        delete mRumple;
}

double SpatialAnalysis::rumpleIndexFullArea()
{
    if (!mRumple)
        mRumple = new RumpleIndex;
    double rum = mRumple->value();
    return rum;
}

void SpatialAnalysis::saveRumpleGrid(QString fileName)
{
    if (!mRumple)
        mRumple = new RumpleIndex;

    Helper::saveToTextFile(fileName, gridToESRIRaster(mRumple->rumpleGrid()) );

}


/****************************************************************************************/
/********************************* RumpleIndex class ************************************/
/****************************************************************************************/


void RumpleIndex::setup()
{
    mRumpleGrid.clear();
    if (!GlobalSettings::instance()->model()) return;

    // the rumple grid hast the same dimensions as the resource unit grid (i.e. 100 meters)
    mRumpleGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
                    GlobalSettings::instance()->model()->RUgrid().cellsize());

}

void RumpleIndex::calculate()
{
    if (mRumpleGrid.isEmpty())
        setup();

    mRumpleGrid.initialize(0.f);
    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();

    // iterate over the resource units and calculate the rumple index / surface area for each resource unit
    HeightGridValue* hgv_8[8]; // array holding pointers to height grid values (neighborhood)
    float heights[9];  // array holding heights (8er neighborhood + center pixel)
    int total_valid_pixels = 0;
    float total_surface_area = 0.f;
    for (float *rg = mRumpleGrid.begin(); rg!=mRumpleGrid.end();++rg) {
        int valid_pixels = 0;
        float surface_area_sum = 0.f;
        GridRunner<HeightGridValue> runner(hg, mRumpleGrid.cellRect(mRumpleGrid.indexOf(rg)));
        while (runner.next()) {
            if (runner.current()->isValid()) {
                runner.neighbors8(hgv_8);
                bool valid = true;
                float *hp = heights;
                *hp++ = runner.current()->height;
                // retrieve height values from the grid
                for (int i=0;i<8;++i) {
                    *hp++ = hgv_8[i] ? hgv_8[i]->height: 0;
                    if (hgv_8[i] && !hgv_8[i]->isValid()) valid = false;
                    if (!hgv_8[i]) valid = false;
                }
                // calculate surface area only for cells which are (a) within the project area, and (b) all neighboring pixels are inside the forest area
                if (valid) {
                   valid_pixels++;
                   float surface_area = calculateSurfaceArea(heights, hg->cellsize());
                   surface_area_sum += surface_area;
                }
            }
        }
        if (valid_pixels>0) {
            float rumple_index = surface_area_sum / ( (float) valid_pixels * hg->cellsize()*hg->cellsize());
            *rg = rumple_index;
            total_valid_pixels += valid_pixels;
            total_surface_area += surface_area_sum;
        }
    }
    mRumpleIndex = 0.;
    if (total_valid_pixels>0) {
        float rumple_index = total_surface_area / ( (float) total_valid_pixels * hg->cellsize()*hg->cellsize());
        mRumpleIndex = rumple_index;
    }
    mLastYear = GlobalSettings::instance()->currentYear();
}

double RumpleIndex::value(const bool force_recalculate)
{
    if (force_recalculate ||  mLastYear != GlobalSettings::instance()->currentYear())
        calculate();
    return mRumpleIndex;
}

double RumpleIndex::test_triangle_area()
{
    // check calculation: numbers for Jenness paper
    float hs[9]={165, 170, 145, 160, 183, 155,122,175,190};
    double area = calculateSurfaceArea(hs, 100);
    return area;
}

inline double surface_length(float h1, float h2, float l)
{
    return sqrt((h1-h2)*(h1-h2) + l*l);
}
inline double heron_triangle_area(float a, float b, float c)
{
    float s=(a+b+c)/2.f;
    return sqrt(s * (s-a) * (s-b) * (s-c) );
}

/// calculate the surface area of a pixel given its height value, the height of the 8 neigboring pixels, and the cellsize
/// the algorithm is based on http://www.jennessent.com/downloads/WSB_32_3_Jenness.pdf
double RumpleIndex::calculateSurfaceArea(const float *heights, const float cellsize)
{
    // values in the height array [0..8]: own height / north/east/west/south/ NE/NW/SE/SW
    // step 1: calculate length on 3d surface between all edges
    //   8(A) * 1(B) * 5(C)       <- 0: center cell, indices in the "heights" grid, A..I: codes used by Jenness
    //   4(D) * 0(E) * 2(F)
    //   7(G) * 3(H) * 6(I)

    float slen[16]; // surface lengths (divided by 2)
    // horizontal
    slen[0] = surface_length(heights[8], heights[1], cellsize)/2.f;
    slen[1] = surface_length(heights[1], heights[5], cellsize)/2.f;
    slen[2] = surface_length(heights[4], heights[0], cellsize)/2.f;
    slen[3] = surface_length(heights[0], heights[2], cellsize)/2.f;
    slen[4] = surface_length(heights[7], heights[3], cellsize)/2.f;
    slen[5] = surface_length(heights[3], heights[6], cellsize)/2.f;
    // vertical
    slen[6] = surface_length(heights[8], heights[4], cellsize)/2.f;
    slen[7] = surface_length(heights[1], heights[0], cellsize)/2.f;
    slen[8] = surface_length(heights[5], heights[2], cellsize)/2.f;
    slen[9] = surface_length(heights[4], heights[7], cellsize)/2.f;
    slen[10] = surface_length(heights[0], heights[3], cellsize)/2.f;
    slen[11] = surface_length(heights[2], heights[6], cellsize)/2.f;
    // diagonal
    float cellsize_diag = cellsize * M_SQRT2;
    slen[12] = surface_length(heights[0], heights[8], cellsize_diag)/2.f;
    slen[13] = surface_length(heights[0], heights[5], cellsize_diag)/2.f;
    slen[14] = surface_length(heights[0], heights[7], cellsize_diag)/2.f;
    slen[15] = surface_length(heights[0], heights[6], cellsize_diag)/2.f;

    // step 2: combine the three sides of all the 8 sub triangles using Heron's formula
    double surface_area = 0.;
    surface_area += heron_triangle_area(slen[12], slen[0], slen[7]); // i
    surface_area += heron_triangle_area(slen[7], slen[1], slen[13]); // ii
    surface_area += heron_triangle_area(slen[6], slen[2], slen[12]); // iii
    surface_area += heron_triangle_area(slen[13], slen[8], slen[3]); // iv
    surface_area += heron_triangle_area(slen[2], slen[9], slen[14]); // v
    surface_area += heron_triangle_area(slen[3], slen[11], slen[15]); // vi
    surface_area += heron_triangle_area(slen[14], slen[10], slen[4]); // vii
    surface_area += heron_triangle_area(slen[10], slen[15], slen[5]); // viii

    return surface_area;
}

/* *************************************************************************************** */
/* ******************************* Spatial Layers **************************************** */
/* *************************************************************************************** */
void SpatialLayeredGrid::setup()
{
    addGrid("rumple", 0);
}

void SpatialLayeredGrid::createGrid(const int grid_index)
{
}


int SpatialLayeredGrid::addGrid(const QString name, FloatGrid *grid)
{
    mGridNames.append(name);
    mGrids.append(grid);
    return mGrids.size();
}



