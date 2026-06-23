/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
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

#include "flowmodule.h"

#include "globalsettings.h"
#include "model.h"
#include "dem.h"
#include "modelcontroller.h"
#include "species.h"
#include "abe/forestmanagementengine.h"
#include "abe/fmstand.h"



FlowModule::FlowModule()
{
    mLayers.setGrid(mGrid);

}

FlowModule::~FlowModule()
{
    GlobalSettings::instance()->controller()->removeLayers(&mLayers);
}

void FlowModule::setup()
{
    // setup the bark beetle grid (10m default size)
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());
    mFSI.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());

    mLayers.setModel(&mFlow);
    GlobalSettings::instance()->controller()->addLayers(&mLayers, "flow");

    // load settings from the XML file
    loadParameters();


    // set up the Flow Module
    const Grid<float>* dem_ptr = static_cast<const Grid<float>*>(GlobalSettings::instance()->model()->dem());
    mFlow.setup(dem_ptr, mFSI);

}


void FlowModule::loadParameters(bool do_reset)
{
    const XmlHelper xml = GlobalSettings::instance()->settings().node("modules.barkbeetle");
    params.cohortsPerGeneration = xml.valueInt(".cohortsPerGeneration", params.cohortsPerGeneration);

}


void FlowModule::run()
{
    // get current vegetation from iLand
    calculateFSI();

    // run the flow algorithm
    mFlow.run();
}

void FlowModule::treeDeath(const Tree *tree, const int removal_type)
{

}

void FlowModule::yearBegin()
{

}

void FlowModule::calculateFSI()
{
    const auto &hgrid = GlobalSettings::instance()->model()->heightGrid();
    for (int index=0;index<hgrid->count(); ++index) {
        if ((*hgrid)[index].isValid()) {
            float height = (*hgrid)[index].height;
            // dummy function for now - a simple parabola with max value at 30m
            float FSI = 1 - ( (height - 30)*(height - 30) / (30*30) );
            mFSI[index] = FSI;
        } else {
            mFSI[index] = 0.;
        }
    }
}



//*********************************************************************************
//************************************ FlowLayers ***************************
//*********************************************************************************


double FlowLayers::value(const FlowCell &data, const int param_index) const
{
    size_t index = &data - mGrid->begin();
    switch(param_index){
    case 0: return mFlow->dem()[index];
    case 1: return mFlow->fsi()[index];

    default: throw IException(QString("invalid variable index for a FlowCell: %1").arg(param_index));
    }
}


const QVector<LayeredGridBase::LayerElement> &FlowLayers::names()
{
    if (mNames.isEmpty())
        mNames = QVector<LayeredGridBase::LayerElement>()
                 << LayeredGridBase::LayerElement(QStringLiteral("elevation"), QStringLiteral("elevation from DEM"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("FSI"), QStringLiteral("forest structure index [0..1]"), GridViewTurbo);
    return mNames;

}

bool FlowLayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click
}







