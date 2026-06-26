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

#include "flowplugin.h"
#include "flowscript.h"
#include "flowmoduleout.h"
#include "outputmanager.h"

#include <QObject>
#include <QJSValue>
#include <QJSEngine>


QString FlowPlugin::name()
{
    return "flow";
}

QString FlowPlugin::version()
{
    return "1.0";
}

QString FlowPlugin::description()
{
    return "flow - Natural Hazards for iLand.";
}


FlowPlugin::FlowPlugin()
{
    qDebug() << "Flow beetle plugin created";

}

void FlowPlugin::setup()
{
    mFlow.setup();

    // setup of the fire related outputs: note: here the fire module is passed directly to the output
    FlowModuleOut *output = new FlowModuleOut();
    output->setFlowModel(&mFlow.flowModel());
    GlobalSettings::instance()->outputManager()->removeOutput(output->tableName());
    GlobalSettings::instance()->outputManager()->addOutput(output);
    // setup of the fire module: load parameters from project file, etc.
}

//Q_SCRIPT_DECLARE_QMETAOBJECT(FireScript, QObject*)

// add the fire script interface
void FlowPlugin::setupScripting(QJSEngine *engine)
{
    FlowScript *script = new FlowScript();
    script->setFlowModule(&mFlow);
    QJSValue obj = engine->newQObject(script);
    engine->globalObject().setProperty("Flow", obj);


    qDebug() << "setup scripting of FlowPlugin called...";
}







