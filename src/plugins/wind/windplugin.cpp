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
#include <QtScript>
#include "global.h"
#include "resourceunit.h"

#include "windplugin.h"
#include "windmodule.h"
#include "windscript.h"

Q_EXPORT_PLUGIN2(iland_wind, WindPlugin)

WindPlugin::WindPlugin()
{
    qDebug() << "Wind plugin created";
    DBGMODE( qDebug("(Wind plugin in debug mode)"););
    mWind = 0;
}

WindPlugin::~WindPlugin()
{
    if (mWind)
        delete mWind;
    mWind = 0;
    qDebug() << "wind plugin destroyed.";
}

QString WindPlugin::name()
{
    return "wind";
}

QString WindPlugin::version()
{
    return "0.1";
}

QString WindPlugin::description()
{
    return "Wind disturbance module for iLand. " \
            "Designed and written by by Rupert Seidl/Werner Rammer.";
}

void WindPlugin::setup()
{
    mWind = new WindModule;
    mWind->setup();
}

void WindPlugin::setupResourceUnit(const ResourceUnit *ru)
{
}

void WindPlugin::setupScripting(QScriptEngine *engine)
{
    WindScript *wind_script = new WindScript();
    wind_script->setModule(mWind);
    QScriptValue obj = engine->newQObject(wind_script, QScriptEngine::AutoOwnership);
    engine->globalObject().setProperty("Wind", obj);

    qDebug() << "setup scripting for windmodule called...";
}

void WindPlugin::yearBegin()
{
}

void WindPlugin::run()
{
    mWind->run();
}



