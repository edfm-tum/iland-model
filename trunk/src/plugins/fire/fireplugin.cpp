#include "global.h"
#include "fireplugin.h"


Q_EXPORT_PLUGIN2(iland_fire, FirePlugin)

QString FirePlugin::name()
{
    return "fire";
}

QString FirePlugin::version()
{
    return "0.1";
}

QString FirePlugin::description()
{
    return "Fire disturbance module for iLand. The fire ignition and fire spread follows the FireBGC v2 model (Keane et al 2011), " \
            "the estimation of severity and fire effects Schumacher et al (2006). See http://iland.boku.ac.at/wildfire for details.\n" \
            "Designed and written by by Rupert Seidl/Werner Rammer.";
}


FirePlugin::FirePlugin()
{
    qDebug() << "Fire plugin created";
//    foreach (const ResourceUnit *ru, GlobalSettings::instance()->model()->ruList())
    //        qDebug() << ru->boundingBox() << ru->constTrees().count();
}





