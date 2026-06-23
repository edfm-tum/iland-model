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

    GlobalSettings::instance()->controller()->addLayers(&mLayers, "flow");

    // load settings from the XML file
    loadParameters();

}


void FlowModule::loadParameters(bool do_reset)
{
    const XmlHelper xml = GlobalSettings::instance()->settings().node("modules.barkbeetle");
    params.cohortsPerGeneration = xml.valueInt(".cohortsPerGeneration", params.cohortsPerGeneration);

}


void FlowModule::run()
{
}

void FlowModule::treeDeath(const Tree *tree, const int removal_type)
{

}

void FlowModule::yearBegin()
{

}



//*********************************************************************************
//************************************ BarkBeetleLayers ***************************
//*********************************************************************************


double FlowLayers::value(const FlowCell &data, const int param_index) const
{
    switch(param_index){
    case 0: return data.test;

    default: throw IException(QString("invalid variable index for a FlowCell: %1").arg(param_index));
    }
}


const QVector<LayeredGridBase::LayerElement> &FlowLayers::names()
{
    if (mNames.isEmpty())
        mNames = QVector<LayeredGridBase::LayerElement>()
                 << LayeredGridBase::LayerElement(QStringLiteral("test"), QStringLiteral("grid value of the pixel"), GridViewTurbo);
    return mNames;

}

bool FlowLayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click
}







