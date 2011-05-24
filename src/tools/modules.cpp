#include "global.h"
#include "modules.h"
#include "plugin_interface.h"

#include "globalsettings.h"
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
            }
        }
    }

}
