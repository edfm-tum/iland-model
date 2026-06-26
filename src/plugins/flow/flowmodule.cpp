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
#include "outputmanager.h"
#include "dem.h"
#include "modelcontroller.h"
#include "species.h"
#include <QJSEngine>



FlowModule::FlowModule()
{
    mLayers.setGrid(mGrid);

}

FlowModule::~FlowModule()
{
    GlobalSettings::instance()->controller()->removeLayers(&mLayers);
}

namespace {
void loadParamsHelper(const XmlHelper &xml, FlowParameters &p) {
    p.alpha = xml.valueDouble(".alpha", p.alpha);
    p.exp = xml.valueDouble(".exp", p.exp);
    p.flux_threshold = xml.valueDouble(".fluxThreshold", p.flux_threshold);
    p.max_z_delta = xml.valueDouble(".maxZDelta", p.max_z_delta);

    p.forest_friction_enabled = xml.valueBool(".forestFriction.enabled", p.forest_friction_enabled);
    p.max_added_friction_forest = xml.valueDouble(".forestFriction.maxAddedFriction", p.max_added_friction_forest);
    p.min_added_friction_forest = xml.valueDouble(".forestFriction.minAddedFriction", p.min_added_friction_forest);
    p.no_friction_effect_v = xml.valueDouble(".forestFriction.noFrictionEffectVelocity", p.no_friction_effect_v);

    p.forest_detrainment_enabled = xml.valueBool(".forestDetrainment.enabled", p.forest_detrainment_enabled);
    p.max_added_detrainment_forest = xml.valueDouble(".forestDetrainment.maxAddedDetrainment", p.max_added_detrainment_forest);
    p.min_added_detrainment_forest = xml.valueDouble(".forestDetrainment.minAddedDetrainment", p.min_added_detrainment_forest);
    p.no_detrainment_effect_v = xml.valueDouble(".forestDetrainment.noDetrainmentEffectVelocity", p.no_detrainment_effect_v);
}
}

void FlowModule::setup()
{
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());
    mFSI.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());

    mLayers.setModel(&mFlow);
    GlobalSettings::instance()->controller()->addLayers(&mLayers, "flow");

    // load settings from the XML file
    loadParameters();

    // set up the Flow Module
    const Grid<float>* dem_ptr = static_cast<const Grid<float>*>(GlobalSettings::instance()->model()->dem());
    mFlow.setup(dem_ptr, mFSI);
    mFlow.parameters() = mParamsAvalanche;
}


void FlowModule::loadParameters(bool do_reset)
{
    Q_UNUSED(do_reset);
    const XmlHelper xmlGlobal = GlobalSettings::instance()->settings().node("modules.flow");

    mRunFunction = xmlGlobal.value(".executeJS", "");

    FlowParameters defaultParams;
    loadParamsHelper(xmlGlobal, defaultParams);

    mParamsAvalanche = defaultParams;
    mParamsRockfall = defaultParams;
    mParamsMudflow = defaultParams;

    XmlHelper xmlAvalanche = GlobalSettings::instance()->settings().node("modules.flow.avalanche");
    if (!xmlAvalanche.isValid()) {
        xmlAvalanche = GlobalSettings::instance()->settings().node("modules.flow.avalance");
    }
    if (xmlAvalanche.isValid()) {
        loadParamsHelper(xmlAvalanche, mParamsAvalanche);
    }

    const XmlHelper xmlRockfall = GlobalSettings::instance()->settings().node("modules.flow.rockfall");
    if (xmlRockfall.isValid()) {
        loadParamsHelper(xmlRockfall, mParamsRockfall);
    }

    const XmlHelper xmlMudflow = GlobalSettings::instance()->settings().node("modules.flow.mudflow");
    if (xmlMudflow.isValid()) {
        loadParamsHelper(xmlMudflow, mParamsMudflow);
    }
}

void FlowModule::run()
{
    // for now, only JS based triggering
    if (!mRunFunction.isEmpty()) {
        qDebug() << "Flow: running Javascript function" << mRunFunction;
        GlobalSettings::instance()->executeJavascript(mRunFunction);
    }
}


void FlowModule::runFlow(QString type, int experimentID)
{
    // Apply process-specific parameters
    if (type == "avalanche") {
        mFlow.parameters() = mParamsAvalanche;
    } else if (type == "rockfall") {
        mFlow.parameters() = mParamsRockfall;
    } else if (type == "mudflow") {
        mFlow.parameters() = mParamsMudflow;
    } else {
        mFlow.parameters() = mParamsAvalanche;
    }

    // get current vegetation from iLand
    calculateFSI(type);

    // run the flow algorithm
    mFlow.run(type, experimentID);

    // create outputs
    GlobalSettings::instance()->outputManager()->execute("flow");
}

void FlowModule::treeDeath(const Tree *tree, const int removal_type)
{
    Q_UNUSED(tree);
    Q_UNUSED(removal_type);
}

void FlowModule::yearBegin()
{

}

void FlowModule::calculateFSI(const QString &type)
{
    const auto &hgrid = GlobalSettings::instance()->model()->heightGrid();
    for (int index=0;index<hgrid->count(); ++index) {
        if ((*hgrid)[index].isValid()) {
            float height = (*hgrid)[index].height;
            // dummy function for now - a simple parabola with max value at 30m
            float FSI = 1 - ( (height - 30)*(height - 30) / (30*30) );
            mFSI[index] = std::max(0.0f, std::min(1.0f, FSI));
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
    case 2: return mFlow->energy()[index];
    case 3: return mFlow->flux()[index];
    case 4: return mFlow->energySum()[index];
    case 5: return mFlow->fpTravelAngle()[index];
    case 6: return mFlow->slTravelAngle()[index];
    case 7: return mFlow->backCalc()[index];
    case 8: return mFlow->count()[index];

    default: throw IException(QString("invalid variable index for a FlowCell: %1").arg(param_index));
    }
}


const QVector<LayeredGridBase::LayerElement> &FlowLayers::names()
{
    if (mNames.isEmpty())
        mNames = QVector<LayeredGridBase::LayerElement>()
                 << LayeredGridBase::LayerElement(QStringLiteral("elevation"), QStringLiteral("elevation from DEM"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("FSI"), QStringLiteral("forest structure index [0..1]"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("energy"), QStringLiteral("maximum energy height [m]"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("flux"), QStringLiteral("maximum cumulative flux [0..1]"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("energySum"), QStringLiteral("sum of energy heights across paths"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("fpAngle"), QStringLiteral("flow path travel angle [deg]"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("slAngle"), QStringLiteral("straight line travel angle [deg]"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("backCalc"), QStringLiteral("back calculation indicator"), GridViewTurbo)
                 << LayeredGridBase::LayerElement(QStringLiteral("count"), QStringLiteral("number of hits from all starts"), GridViewTurbo);
    return mNames;
}

bool FlowLayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click
}







