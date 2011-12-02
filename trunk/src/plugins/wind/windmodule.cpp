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
#include "windmodule.h"
#include "globalsettings.h"
#include "model.h"
#include "modelcontroller.h"
#include "helper.h"

/** @defgroup windmodule iLand windmodule
  The wind module is a disturbance module within the iLand framework.

  FIXME  !!
   See http://iland.boku.ac.at/wind for the science behind the module,
  and http://iland.boku.ac.at/fire+module for the implementation/ using side.
 */


/** @class WindModule

    FIXME some more details
  */

double WindLayers::value(const WindCell &data, const int param_index) const
{
    switch(param_index){
    case 0: return data.height==9999.f?0:data.height; // height
    case 1: return data.edge; // maximum difference to neighboring cells
    default: throw IException(QString("invalid variable index for a WindCell: %1").arg(param_index));
    }
}


const QStringList WindLayers::names() const
{
    return QStringList() <<  "height" << "edge";

}

WindModule::WindModule()
{

}


void WindModule::setup()
{
    // setup the grid (using the size/resolution)
    //mRUGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(),
    //              GlobalSettings::instance()->model()->RUgrid().cellsize());
    // setup the fire spread grid
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());
    //mFireId = 0;

    // set some global settings
    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.wind"));
//    mWindSpeedMin = xml.valueDouble(".wind.speedMin", 5.);
//    mWindSpeedMax = xml.valueDouble(".wind.speedMax", 10.);
//    mWindDirection = xml.valueDouble(".wind.direction", 270.); // defaults to "west"
    //    mFireSizeSigma = xml.valueDouble(".fireSizeSigma", 0.25);

    mWindLayers.setGrid(mGrid);
    GlobalSettings::instance()->controller()->addLayers(&mWindLayers, "wind");


}

/// the run() function executes the fire events
void WindModule::run()
{
    // (1) do we have a wind event this year?

    // (2) do the actual wind calculations
    // (2.1) setup the wind data
    initWindGrid();

    // check for edges in the stand
    DebugTimer tEdge("wind:detectEdges");
    detectEdges();
}

void WindModule::initWindGrid()
{
    // as long as we have 10m -> easy!
    if (cellsize()==cHeightPerRU) {
        WindCell *p = mGrid.begin();
        for (const HeightGridValue *hgv=GlobalSettings::instance()->model()->heightGrid()->begin(); hgv!=GlobalSettings::instance()->model()->heightGrid()->end(); ++hgv, ++p)
            p->height = hgv->isValid()?hgv->height:9999.f;
        return;
    }
    throw IException("WindModule::initWindGrid: not 10m of windpixels...");
}


void WindModule::detectEdges()
{
    WindCell *p_above, *p, *p_below;
    int dy = mGrid.sizeY();
    int dx = mGrid.sizeX();
    int x,y;
    const float threshold = 10.f;
    for (y=1;y<dy-1;++y){
        p = mGrid.ptr(1,y);
        p_above = p - dx; // one line above
        p_below = p + dx; // one line below
        for (x=1;x<dx-1;++x,++p,++p_below, ++p_above) {
            p->edge = 0.f; // no edge
            if (p->isValid()) {
                float max_h = p->height - threshold; // max_h: if a surrounding pixel is higher than this value, then there is no edge here
                if (max_h > 0) {
                    // check 8-neighborhood. if one of those pixels exceed the threshold, then the current
                    // pixel is not an "edge".
                    if (((p_above-1)->height < max_h) ||
                            ((p_above)->height < max_h) ||
                            ((p_above+1)->height < max_h) ||
                            ((p-1)->height < max_h)  ||
                            ((p+1)->height < max_h) ||
                            ((p_below-1)->height < max_h) ||
                            ((p_below)->height < max_h) ||
                            ((p_below+1)->height < max_h) ) {
                        // edge found:
                        p->edge = 1.f;
                    }
                }
            }
        }
    }
}
