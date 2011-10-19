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
#include "modules.h"
#include "plugin_interface.h"

#include "globalsettings.h"
#include "helper.h"
#include <QPluginLoader>

// include the static modules here in the code:
Q_IMPORT_PLUGIN(iland_fire)


Modules::Modules()
{
    init();
}

// load the static plugins
void Modules::init()
{
    foreach (QObject *plugin, QPluginLoader::staticInstances()) {
        DisturbanceInterface *di = qobject_cast<DisturbanceInterface *>(plugin);
        if (di) {
            qDebug() << di->name();
            // check xml file
            if (GlobalSettings::instance()->settings().valueBool(QString("modules.%1.enabled").arg(di->name()))) {
                // plugin is enabled: store in list of active modules
                mInterfaces.append(di);
                // check for other interfaces
                SetupResourceUnitInterface *si=qobject_cast<SetupResourceUnitInterface *>(plugin);
                if (si)
                    mSetupRUs.append(si);
                WaterInterface *wi = qobject_cast<WaterInterface *>(plugin);
                if (wi)
                    mWater.append(wi);
            }
        }
    }

}

DisturbanceInterface * Modules::module(const QString &module_name)
{
    foreach(DisturbanceInterface *di, mInterfaces)
        if (di->name() == module_name)
            return di;
    return 0;
}

void Modules::setupResourceUnit(const ResourceUnit *ru)
{
    foreach(SetupResourceUnitInterface *si, mSetupRUs)
        si->setupResourceUnit(ru);
}

void Modules::setup()
{
    foreach(DisturbanceInterface *di, mInterfaces)
        di->setup();
}

void Modules::calculateWater(const ResourceUnit *resource_unit, const WaterCycleData *water_data)
{
    foreach(WaterInterface *wi, mWater)
        wi->calculateWater(resource_unit, water_data);
}

void Modules::run()
{
    DebugTimer t("modules");
    QList<DisturbanceInterface*> run_list = mInterfaces;

    // execute modules in random order
    for (int i=0;i < run_list.size(); ++i) {
        int idx = irandom(0, run_list.size()-1);
        if (logLevelDebug())
            qDebug() << "executing disturbance module: " << run_list[idx]->name();
        run_list[idx]->run();
        // remove from list
        run_list.removeAt(idx);
    }
}

void Modules::yearBegin()
{
    foreach(DisturbanceInterface *di, mInterfaces)
        di->yearBegin();

}


