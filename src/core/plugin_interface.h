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

    // setup
    virtual void setup()=0; ///< setup after general iLand model frame is created.
    virtual void yearBegin()=0; ///< function executes at the beginning of a year (e.g., cleanup)
    virtual void run()=0; ///< main function that once a year (after growth)
};

class SetupResourceUnitInterface
{
public:
    virtual ~SetupResourceUnitInterface() {}
    /// setup of parameters specific for resource unit.
    /// this allows using spatially explicit parmater values.
    virtual void setupResourceUnit(const ResourceUnit *ru)=0;

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

Q_DECLARE_INTERFACE(SetupResourceUnitInterface, "at.ac.boku.iland.SetupResourceUnitInterface/1.0")

#endif // PLUGIN_INTERFACE_H
