#ifndef FIREPLUGIN_H
#define FIREPLUGIN_H

#include <QObject>

#include "plugin_interface.h"

class FirePlugin: public QObject, public DisturbanceInterface
{
    Q_OBJECT
    Q_INTERFACES(DisturbanceInterface)

public:
    FirePlugin();

    QString name(); ///< a unique name of the plugin
    QString version(); ///< a version identification
    QString description(); ///< some additional description. This info is shown in the GUI and is printed to the log file.

};

#endif // FIREPLUGIN_H
