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
#include "barkbeetleplugin.h"
#include "outputmanager.h"

#include <QObject>
#include <QJSValue>
#include <QJSEngine>

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(iland_barkbeetle, BarkBeetlePlugin)
#endif

QString BarkBeetlePlugin::name()
{
    return "barkbeetle";
}

QString BarkBeetlePlugin::version()
{
    return "1.0";
}

QString BarkBeetlePlugin::description()
{
    return "bark beetle module for iLand.";
}


BarkBeetlePlugin::BarkBeetlePlugin()
{
    qDebug() << "Bark beetle plugin created";
    DBGMODE( qDebug("(bark beetle plugin in debug mode)"););

//    foreach (const ResourceUnit *ru, GlobalSettings::instance()->model()->ruList())
    //        qDebug() << ru->boundingBox() << ru->constTrees().count();
}

void BarkBeetlePlugin::setup()
{
//    // setup of the fire related outputs: note: here the fire module is passed directly to the output
//    FireOut *fire_output = new FireOut();
//    fire_output->setFireModule(&mFire);
//    GlobalSettings::instance()->outputManager()->addOutput(fire_output);
//    // setup of the fire module: load parameters from project file, etc.
//    mFire.setup();
    mBeetle.setup();
}

//Q_SCRIPT_DECLARE_QMETAOBJECT(FireScript, QObject*)

// add the fire script interface
void BarkBeetlePlugin::setupScripting(QJSEngine *engine)
{
//    FireScript *fire_script = new FireScript();
//    fire_script->setFireModule(&mFire);
//    QJSValue obj = engine->newQObject(fire_script);
//    engine->globalObject().setProperty("Fire", obj);

//    qDebug() << "setup scripting called...";
}





