#ifndef FIREPLUGIN_H
#define FIREPLUGIN_H

#include <QObject>
#include "firemodule.h"

#include "plugin_interface.h"
class FirePlugin: public QObject,
        public DisturbanceInterface,
        public WaterInterface,
        public SetupResourceUnitInterface
{
    Q_OBJECT
    Q_INTERFACES(DisturbanceInterface WaterInterface SetupResourceUnitInterface)

public:
    FirePlugin();
    // general interface details
    QString name(); ///< a unique name of the plugin
    QString version(); ///< a version identification
    QString description(); ///< some additional description. This info is shown in the GUI and is printed to the log file.

    // setup
    /// setup after the main iLand model frame is created
    void setup() { mFire.setup(); }
    /// setup resource unit specific parameters
    void setupResourceUnit(const ResourceUnit *ru) { mFire.setup(ru);}

    // access to water data
    void calculateWater(const ResourceUnit *resource_unit, const WaterCycleData *water_data) {mFire.calculateDroughtIndex(resource_unit, water_data); }

private:
    FireModule mFire;
};

#endif // FIREPLUGIN_H
