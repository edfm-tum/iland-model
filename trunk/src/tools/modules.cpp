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


