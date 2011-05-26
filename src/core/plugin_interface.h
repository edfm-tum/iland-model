#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QtPlugin>

/** This file contains the interfaces for iLand disturbance modules.
    It uses the plugin mechanism provided by Qt; the destructor is needed by some compilers (see Qt-doc).
    The interface contains several parts:
    * General information: used to identify plugins

*/

class DisturbanceInterface
{
public:
    virtual ~DisturbanceInterface() {}

    // general information / properties
    virtual QString name()=0; ///< a unique name of the plugin
    virtual QString version()=0; ///< a version identification
    virtual QString description()=0; ///< some additional description. This info is shown in the GUI and is printed to the log file.

};

/** WaterInterface allows accessing intermediate water variables (e.g. interception)

 */
class WaterCycleData; // forward
class ResourceUnit; // forward
class WaterInterface
{
public:
    virtual ~WaterInterface() {}

    virtual void calculateWater(const ResourceUnit *resource_unit, const WaterCycleData *water_data)=0;
};

Q_DECLARE_INTERFACE(DisturbanceInterface, "at.ac.boku.iland.DisturbanceInterface/1.0")

Q_DECLARE_INTERFACE(WaterInterface, "at.ac.boku.iland.WaterInterface/1.0")

#endif // PLUGIN_INTERFACE_H
